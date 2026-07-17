module;
#include <raylib.h>
#include <iostream>
#include <memory>
#include <vector>
#include <string>
export module renderer;

import app_state;
import ipc_client;
import theme;
import tab_bar_renderer;
import avatar_renderer;
import chat_renderer;

export namespace jay {

class Renderer {
public:
  Renderer(std::shared_ptr<ApplicationState> state, std::shared_ptr<IPCClient> ipcClient)
    : m_state(std::move(state)), m_ipcClient(std::move(ipcClient)), m_activeTab(0) {}

  ~Renderer() {
    if (m_font.texture.id != 0) {
      UnloadFont(m_font);
    }
    if (IsWindowReady()) CloseWindow();
  }

  void Run() {
    // Janela dobrada em tamanho: 1200 x 850
    InitWindow(1200, 850, "Jay Frontend");
    SetTargetFPS(60);

    // Gera lista de codepoints Unicode Latin-1 (ASCII + acentos brasileiros)
    std::vector<int> codepoints;
    for (int i = 32; i < 256; ++i) {
      codepoints.push_back(i);
    }

    // Carrega fonte do sistema operacional (Liberation Sans) com 32px de tamanho base
    m_font = LoadFontEx("/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf", 32, codepoints.data(), codepoints.size());
    if (m_font.texture.id == 0) {
      std::cout << "Aviso: Não foi possível carregar a fonte LiberationSans. Usando fonte padrão.\n";
      m_font = GetFontDefault();
    } else {
      // Habilita filtro bilinear para anti-aliasing perfeito na renderização do texto
      SetTextureFilter(m_font.texture, TEXTURE_FILTER_BILINEAR);
    }

    while (!WindowShouldClose()) {
      // 1. Atalhos globais de teclado (Ctrl + Tab para alternar abas)
      bool ctrlPressed = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
      if (ctrlPressed && IsKeyPressed(KEY_TAB)) {
        m_activeTab = (m_activeTab + 1) % 2;
      }

      // 2. Processa input de bloqueio do Modal de Permissão se estiver ativo
      if (m_state->IsPromptingPermission()) {
        bool resolved = false;
        bool allowed = false;
        std::string modality = "";

        // Teclado
        if (IsKeyPressed(KEY_Y)) {
          resolved = true;
          allowed = true;
          modality = "keyboard";
        } else if (IsKeyPressed(KEY_N)) {
          resolved = true;
          allowed = false;
          modality = "keyboard";
        }

        // Mouse click nos botões do modal (adaptado para a nova resolução de 1200x850)
        if (!resolved && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
          Vector2 mousePos = GetMousePosition();
          const int mw = 460, mh = 160; // Modal ligeiramente maior
          const int mx = (GetScreenWidth() / 2) - mw / 2;
          const int my = (GetScreenHeight() / 2) - mh / 2;

          Rectangle btnAllow = {(float)(mx + 40), (float)(my + 100), 160.0f, 36.0f};
          Rectangle btnDeny = {(float)(mx + mw - 200), (float)(my + 100), 160.0f, 36.0f};

          if (CheckCollisionPointRec(mousePos, btnAllow)) {
            resolved = true;
            allowed = true;
            modality = "click";
          } else if (CheckCollisionPointRec(mousePos, btnDeny)) {
            resolved = true;
            allowed = false;
            modality = "click";
          }
        }

        if (resolved) {
          std::string refId = m_state->GetPendingRefId();
          std::string allowedStr = allowed ? "true" : "false";
          std::string msg = "{\"type\": \"permission.response\", \"payload\": {\"ref_id\": \"" + refId +
                            "\", \"allowed\": " + allowedStr + ", \"modality\": \"" + modality + "\"}}";
          m_ipcClient->SendMessage(msg);
          m_state->ClearPermissionPrompt();
        }
      }

      // 3. Renderização
      BeginDrawing();
      ClearBackground(Theme::Background);

      // Renderiza e atualiza barra de abas superiores
      m_activeTab = m_tabBarRenderer.UpdateAndDraw(m_activeTab, GetScreenWidth(), m_font);

      // Renderiza aba correspondente
      if (m_activeTab == 0) {
        m_avatarRenderer.Draw(m_state, m_font, GetScreenWidth(), GetScreenHeight());
      } else {
        m_chatRenderer.Draw(m_state, m_ipcClient, m_font, GetScreenWidth(), GetScreenHeight());
      }

      // Renderiza o Modal de Permissão se estiver ativo por cima de tudo
      if (m_state->IsPromptingPermission()) {
        DrawPermissionModal();
      }

      EndDrawing();
    }
  }

private:
  std::shared_ptr<ApplicationState> m_state;
  std::shared_ptr<IPCClient> m_ipcClient;
  int m_activeTab; // 0 = Avatar, 1 = Chat
  Font m_font;

  TabBarRenderer m_tabBarRenderer;
  AvatarRenderer m_avatarRenderer;
  ChatRenderer m_chatRenderer;

  void DrawPermissionModal() {
    const int cx = GetScreenWidth() / 2;
    const int cy = GetScreenHeight() / 2;
    const int mw = 460, mh = 160;
    const int mx = cx - mw / 2, my = cy - mh / 2;

    // Fundo e bordas do modal
    DrawRectangle(mx, my, mw, mh, Theme::Panel);
    DrawRectangleLines(mx, my, mw, mh, Theme::Border);

    // Título do modal
    float titleSize = 18.0f;
    Vector2 titlePos = {(float)(mx + 20), (float)(my + 20)};
    DrawTextEx(m_font, "Solicitação de Permissão", titlePos, titleSize, 1.0f, Theme::TextMain);

    // Ajusta visualização do prompt explicativo
    float promptSize = 14.0f;
    Vector2 promptPos = {(float)(mx + 20), (float)(my + 55)};
    DrawTextEx(m_font, m_state->GetPendingPrompt().c_str(), promptPos, promptSize, 1.0f, Theme::TextSec);

    // Botão Permitir (Verde / Allow)
    Rectangle btnAllow = {(float)(mx + 40), (float)(my + 100), 160.0f, 36.0f};
    DrawRectangleRec(btnAllow, Theme::AllowBtn);
    Vector2 allowTextDim = MeasureTextEx(m_font, "Permitir [Y]", 14.0f, 1.0f);
    Vector2 allowTextPos = {
      btnAllow.x + (btnAllow.width / 2) - (allowTextDim.x / 2),
      btnAllow.y + (btnAllow.height / 2) - (allowTextDim.y / 2)
    };
    DrawTextEx(m_font, "Permitir [Y]", allowTextPos, 14.0f, 1.0f, WHITE);

    // Botão Negar (Vermelho / Deny)
    Rectangle btnDeny = {(float)(mx + mw - 200), (float)(my + 100), 160.0f, 36.0f};
    DrawRectangleRec(btnDeny, Theme::DenyBtn);
    Vector2 denyTextDim = MeasureTextEx(m_font, "Negar [N]", 14.0f, 1.0f);
    Vector2 denyTextPos = {
      btnDeny.x + (btnDeny.width / 2) - (denyTextDim.x / 2),
      btnDeny.y + (btnDeny.height / 2) - (denyTextDim.y / 2)
    };
    DrawTextEx(m_font, "Negar [N]", denyTextPos, 14.0f, 1.0f, WHITE);
  }
};

} // namespace jay
