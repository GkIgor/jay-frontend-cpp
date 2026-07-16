module;
#include <raylib.h>

#include <iostream>
#include <memory>
export module renderer;

import avatar;
import ipc_client;

export namespace jay {
class Renderer {
public:
  Renderer(std::shared_ptr<Avatar> avatar, std::shared_ptr<IPCClient> ipcClient)
    : m_avatar(std::move(avatar)), m_ipcClient(std::move(ipcClient)) {}
  ~Renderer() {
    if (IsWindowReady()) CloseWindow();
  }
  void Run() {
    InitWindow(400, 400, "Jay Frontend");
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
      // Captura input do modal de permissão caso esteja ativo
      if (m_avatar->IsPromptingPermission()) {
        bool resolved = false;
        bool allowed = false;
        std::string modality = "";

        // 1. Teclado
        if (IsKeyPressed(KEY_Y)) {
          resolved = true;
          allowed = true;
          modality = "keyboard";
        } else if (IsKeyPressed(KEY_N)) {
          resolved = true;
          allowed = false;
          modality = "keyboard";
        }

        // 2. Clique do mouse nos botões do modal
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
          std::string refId = m_avatar->GetPendingRefId();
          std::string allowedStr = allowed ? "true" : "false";
          std::string msg = "{\"type\": \"permission.response\", \"payload\": {\"ref_id\": \"" + refId +
                            "\", \"allowed\": " + allowedStr + ", \"modality\": \"" + modality + "\"}}";
          m_ipcClient->SendMessage(msg);
          m_avatar->ClearPermissionPrompt();
        }
      }

      BeginDrawing();
      ClearBackground(RAYWHITE);
      DrawAvatarState();
      EndDrawing();
    }
  }

private:
  std::shared_ptr<Avatar> m_avatar;
  std::shared_ptr<IPCClient> m_ipcClient;

  void DrawAvatarState() {
    State currentState = m_avatar->GetState();
    const int cx = GetScreenWidth() / 2, cy = GetScreenHeight() / 2;
    switch (currentState) {
      case State::Idle:
        DrawCircle(cx, cy, 50.0f, LIGHTGRAY);
        DrawText("Jay (Idle)", cx - 40, cy + 70, 20, DARKGRAY);
        break;
      case State::Thinking:
        DrawCircle(cx, cy, 50.0f, ORANGE);
        DrawText("Jay (Thinking...)", cx - 70, cy + 70, 20, DARKGRAY);
        break;
      case State::Executing:
        DrawCircle(cx, cy, 50.0f, GREEN);
        DrawText("Jay (Executing)", cx - 60, cy + 70, 20, DARKGRAY);
        break;
      case State::Sleeping:
        DrawCircle(cx, cy, 50.0f, DARKBLUE);
        DrawText("Jay (Sleeping)", cx - 60, cy + 70, 20, LIGHTGRAY);
        break;
    }
    if (auto anim = m_avatar->ConsumeNextAnimation()) std::cout << "Anim: " << *anim << "\n";

    // Desenha modal por cima se houver pedido de permissão ativo
    if (m_avatar->IsPromptingPermission()) {
      const int mw = 360, mh = 140;
      const int mx = cx - mw / 2, my = cy - mh / 2;

      // Fundo e Borda do Modal
      DrawRectangle(mx, my, mw, mh, LIGHTGRAY);
      DrawRectangleLines(mx, my, mw, mh, DARKGRAY);

      DrawText("Solicitacao de Permissao", mx + 15, my + 15, 18, BLACK);

      // Exibe o Prompt explicativo enviado pelo Core
      DrawText(m_avatar->GetPendingPrompt().c_str(), mx + 15, my + 45, 11, MAROON);

      // Botão Permitir (Verde / Lime)
      Rectangle btnAllow = {(float)(mx + 30), (float)(my + 90), 130.0f, 32.0f};
      DrawRectangleRec(btnAllow, LIME);
      DrawRectangleLinesEx(btnAllow, 1.0f, GREEN);
      DrawText("Permitir [Y]", mx + 50, my + 98, 14, BLACK);

      // Botão Negar (Vermelho)
      Rectangle btnDeny = {(float)(mx + mw - 160), (float)(my + 90), 130.0f, 32.0f};
      DrawRectangleRec(btnDeny, RED);
      DrawRectangleLinesEx(btnDeny, 1.0f, MAROON);
      DrawText("Negar [N]", mx + mw - 120, my + 98, 14, WHITE);
    }
  }
};
}  // namespace jay
