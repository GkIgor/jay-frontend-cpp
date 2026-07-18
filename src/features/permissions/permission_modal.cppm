module;
#include <raylib.h>
#include <string>
#include <memory>
export module permission_modal;

import app_state;
import permission_state;
import theme;
import ipc_client;

export namespace jay {

class PermissionModal {
public:
  PermissionModal() = default;

  static void Update(const std::shared_ptr<ApplicationState>& state, const std::shared_ptr<IPCClient>& ipcClient) {
    if (!state->permission.IsPromptingPermission()) return;

    bool resolved = false;
    bool allowed = false;
    std::string modality = "";

    // Atalhos de teclado
    if (IsKeyPressed(KEY_Y)) {
      resolved = true;
      allowed = true;
      modality = "keyboard";
    } else if (IsKeyPressed(KEY_N)) {
      resolved = true;
      allowed = false;
      modality = "keyboard";
    }

    // Clique de mouse nos botões
    if (!resolved && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      Vector2 mousePos = GetMousePosition();
      const int mw = 460, mh = 160;
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
      std::string refId = state->permission.GetPendingRefId();
      std::string allowedStr = allowed ? "true" : "false";
      std::string msg = "{\"type\": \"permission.response\", \"payload\": {\"ref_id\": \"" + refId +
                        "\", \"allowed\": " + allowedStr + ", \"modality\": \"" + modality + "\"}}";
      ipcClient->SendMessage(msg);
      state->permission.ClearPermissionPrompt();
    }
  }

  static void Draw(const std::shared_ptr<ApplicationState>& state, Font font) {
    if (!state->permission.IsPromptingPermission()) return;

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
    DrawTextEx(font, "Solicitação de Permissão", titlePos, titleSize, 1.0f, Theme::TextMain);

    // Prompt explicativo
    float promptSize = 14.0f;
    Vector2 promptPos = {(float)(mx + 20), (float)(my + 55)};
    DrawTextEx(font, state->permission.GetPendingPrompt().c_str(), promptPos, promptSize, 1.0f, Theme::TextSec);

    // Botão Permitir (Verde / Allow)
    Rectangle btnAllow = {(float)(mx + 40), (float)(my + 100), 160.0f, 36.0f};
    DrawRectangleRec(btnAllow, Theme::AllowBtn);
    Vector2 allowTextDim = MeasureTextEx(font, "Permitir [Y]", 14.0f, 1.0f);
    Vector2 allowTextPos = {
      btnAllow.x + (btnAllow.width / 2) - (allowTextDim.x / 2),
      btnAllow.y + (btnAllow.height / 2) - (allowTextDim.y / 2)
    };
    DrawTextEx(font, "Permitir [Y]", allowTextPos, 14.0f, 1.0f, WHITE);

    // Botão Negar (Vermelho / Deny)
    Rectangle btnDeny = {(float)(mx + mw - 200), (float)(my + 100), 160.0f, 36.0f};
    DrawRectangleRec(btnDeny, Theme::DenyBtn);
    Vector2 denyTextDim = MeasureTextEx(font, "Negar [N]", 14.0f, 1.0f);
    Vector2 denyTextPos = {
      btnDeny.x + (btnDeny.width / 2) - (denyTextDim.x / 2),
      btnDeny.y + (btnDeny.height / 2) - (denyTextDim.y / 2)
    };
    DrawTextEx(font, "Negar [N]", denyTextPos, 14.0f, 1.0f, WHITE);
  }
};

} // namespace jay
