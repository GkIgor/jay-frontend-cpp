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
        if (IsKeyPressed(KEY_Y)) {
          std::string refId = m_avatar->GetPendingRefId();
          std::string msg =
            "{\"type\": \"permission.response\", \"payload\": {\"ref_id\": \"" + refId + "\", \"allowed\": true}}";
          m_ipcClient->SendMessage(msg);
          m_avatar->ClearPermissionPrompt();
        } else if (IsKeyPressed(KEY_N)) {
          std::string refId = m_avatar->GetPendingRefId();
          std::string msg =
            "{\"type\": \"permission.response\", \"payload\": {\"ref_id\": \"" + refId + "\", \"allowed\": false}}";
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

    // Desenha modal simples por cima se houver pedido de permissão ativo
    if (m_avatar->IsPromptingPermission()) {
      const int mw = 320, mh = 120;
      const int mx = cx - mw / 2, my = cy - mh / 2;
      DrawRectangle(mx, my, mw, mh, LIGHTGRAY);
      DrawRectangleLines(mx, my, mw, mh, DARKGRAY);
      DrawText("Permitir Execucao?", mx + 20, my + 15, 20, BLACK);
      DrawText(m_avatar->GetPendingPermission().c_str(), mx + 20, my + 45, 16, MAROON);
      DrawText("[Y] Permitir  [N] Negar", mx + 20, my + 80, 16, DARKGRAY);
    }
  }
};
}  // namespace jay
