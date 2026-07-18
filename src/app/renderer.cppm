module;
#include <raylib.h>
#include <memory>
#include <string>
#include <vector>
#include <iostream>
export module renderer;

import app_state;
import ipc_client;
import theme;
import tab_bar;
import avatar_renderer;
import chat_renderer;
import permission_modal;

export namespace jay {

/**
 * Renderer (Compositor Global de Interfaces)
 * 
 * Papel estrutural (ADR-001):
 * O Renderer principal atua estritamente como compositor das views de cada feature
 * (AvatarRenderer, ChatRenderer) e dos widgets comuns (TabBar, PermissionModal).
 * Ele não deve conter lógica complexa de manipulação de estados ou tratamento direto de inputs de rede.
 */
class Renderer {
public:
  Renderer(std::shared_ptr<ApplicationState> state, std::shared_ptr<IPCClient> ipcClient)
    : m_state(std::move(state)), m_ipcClient(std::move(ipcClient)), m_activeTab(0) {}

  void Init(const std::string& fontPath) {
    // Gera lista de codepoints Unicode Latin-1 (ASCII + acentos brasileiros)
    std::vector<int> codepoints;
    for (int i = 32; i < 256; ++i) {
      codepoints.push_back(i);
    }

    // Carrega a fonte do sistema operacional Liberation Sans com anti-aliasing
    m_font = LoadFontEx(fontPath.c_str(), 32, codepoints.data(), codepoints.size());
    if (m_font.texture.id == 0) {
      std::cout << "Aviso: Não foi possível carregar a fonte. Usando fonte padrão do Raylib.\n";
      m_font = GetFontDefault();
    } else {
      SetTextureFilter(m_font.texture, TEXTURE_FILTER_BILINEAR);
    }
  }

  void CleanUp() {
    if (m_font.texture.id != 0) {
      UnloadFont(m_font);
    }
  }

  void Draw() {
    // 1. Alterna abas via atalho de teclado global (Ctrl + Tab)
    bool ctrlPressed = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    if (ctrlPressed && IsKeyPressed(KEY_TAB)) {
      m_activeTab = (m_activeTab + 1) % 2;
    }

    // 2. Atualiza regras de input do modal de permissão se ativo
    PermissionModal::Update(m_state, m_ipcClient);

    // 3. Renderização
    BeginDrawing();
    ClearBackground(Theme::Background);

    // Renderiza e atualiza a barra de abas superiores (TabBar Widget)
    std::vector<std::string> tabLabels = {"AVATAR", "CHAT"};
    m_activeTab = TabBar::UpdateAndDraw(m_activeTab, GetScreenWidth(), m_font, tabLabels);

    // Renderiza a tela correspondente à aba ativa
    if (m_activeTab == 0) {
      m_avatarRenderer.Draw(m_state, m_font, GetScreenWidth(), GetScreenHeight());
    } else {
      m_chatRenderer.Draw(m_state, m_ipcClient, m_font, GetScreenWidth(), GetScreenHeight());
    }

    // Renderiza o Modal de Permissão se ativo por cima de tudo
    PermissionModal::Draw(m_state, m_font);

    EndDrawing();
  }

private:
  std::shared_ptr<ApplicationState> m_state;
  std::shared_ptr<IPCClient> m_ipcClient;
  int m_activeTab; // 0 = Avatar, 1 = Chat
  Font m_font;

  AvatarRenderer m_avatarRenderer;
  ChatRenderer m_chatRenderer;
};

} // namespace jay
