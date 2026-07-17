module;
#include <raylib.h>
#include <iostream>
#include <memory>
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
    if (IsWindowReady()) CloseWindow();
  }

  void Run() {
    InitWindow(600, 550, "Jay Frontend");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
      // 1. Processa input de bloqueio do Modal de Permissão se estiver ativo
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

        // Mouse click nos botões do modal
        if (!resolved && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
          Vector2 mousePos = GetMousePosition();
          const int mw = 360, mh = 140;
          const int mx = (GetScreenWidth() / 2) - mw / 2;
          const int my = (GetScreenHeight() / 2) - mh / 2;

          Rectangle btnAllow = {(float)(mx + 30), (float)(my + 90), 130.0f, 32.0f};
          Rectangle btnDeny = {(float)(mx + mw - 160), (float)(my + 90), 130.0f, 32.0f};

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

      // 2. Renderização
      BeginDrawing();
      ClearBackground(Theme::Background);

      // Renderiza e atualiza barra de abas superiores
      m_activeTab = m_tabBarRenderer.UpdateAndDraw(m_activeTab, GetScreenWidth());

      // Renderiza aba correspondente
      if (m_activeTab == 0) {
        m_avatarRenderer.Draw(m_state, GetScreenWidth(), GetScreenHeight());
      } else {
        m_chatRenderer.Draw(m_state, m_ipcClient, GetScreenWidth(), GetScreenHeight());
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

  TabBarRenderer m_tabBarRenderer;
  AvatarRenderer m_avatarRenderer;
  ChatRenderer m_chatRenderer;

  void DrawPermissionModal() {
    const int cx = GetScreenWidth() / 2;
    const int cy = GetScreenHeight() / 2;
    const int mw = 360, mh = 140;
    const int mx = cx - mw / 2, my = cy - mh / 2;

    // Fundo e bordas do modal
    DrawRectangle(mx, my, mw, mh, Theme::Panel);
    DrawRectangleLines(mx, my, mw, mh, Theme::Border);

    DrawText("Solicitação de Permissão", mx + 15, my + 15, 16, Theme::TextMain);

    // Ajusta visualização do prompt
    DrawText(m_state->GetPendingPrompt().c_str(), mx + 15, my + 45, 11, Theme::TextSec);

    // Botão Permitir (Verde / Allow)
    Rectangle btnAllow = {(float)(mx + 30), (float)(my + 90), 130.0f, 32.0f};
    DrawRectangleRec(btnAllow, Theme::AllowBtn);
    DrawText("Permitir [Y]", mx + 54, my + 98, 12, WHITE);

    // Botão Negar (Vermelho / Deny)
    Rectangle btnDeny = {(float)(mx + mw - 160), (float)(my + 90), 130.0f, 32.0f};
    DrawRectangleRec(btnDeny, Theme::DenyBtn);
    DrawText("Negar [N]", mx + mw - 120, my + 98, 12, WHITE);
  }
};

} // namespace jay
