module;
#include <raylib.h>
#include <string>
#include <variant>
export module tool_group_bubble;

import chat_state;
import message_bubble;
import theme;

export namespace jay {

class ToolGroupBubble {
public:
  static constexpr float HEADER_HEIGHT    = 36.0f;
  static constexpr float ACTION_ROW_HEIGHT = 26.0f;
  static constexpr float PADDING_BOTTOM   = 8.0f;

  static float CalculateHeight(const ToolGroupPayload& payload, bool expanded) {
    if (!expanded) return HEADER_HEIGHT;
    return HEADER_HEIGHT + payload.actions.size() * ACTION_ROW_HEIGHT + PADDING_BOTTOM;
  }

  static bool Draw(Font font, const ChatBubbleData& bubble, float scrollOffset,
                   float fontSize, Vector2 mousePos, bool expanded) {
    const auto* payload = std::get_if<ToolGroupPayload>(&bubble.payload);
    if (!payload) return false;

    Rectangle scrolledRect = bubble.rect;
    scrolledRect.y += scrollOffset;

    DrawRectangleRounded(scrolledRect, 0.15f, 4, Theme::Surface);
    DrawRectangleLinesEx(scrolledRect, 1.0f, Theme::Border);

    Rectangle headerRect = {scrolledRect.x, scrolledRect.y, scrolledRect.width, HEADER_HEIGHT};
    bool headerHovered = CheckCollisionPointRec(mousePos, headerRect);

    if (headerHovered) {
      DrawRectangleRounded(headerRect, 0.15f, 4, GetColor(0xFFFFFF0A));
    }

    float arrowX = scrolledRect.x + 14.0f;
    float arrowY = scrolledRect.y + HEADER_HEIGHT / 2.0f;
    Color arrowColor = headerHovered ? Theme::TextMain : Theme::TextSec;

    if (expanded) {
      DrawTriangle(
        {arrowX,        arrowY - 5.0f},
        {arrowX + 9.0f, arrowY - 5.0f},
        {arrowX + 4.5f, arrowY + 4.0f},
        arrowColor
      );
    } else {
      DrawTriangle(
        {arrowX,        arrowY - 5.0f},
        {arrowX + 9.0f, arrowY},
        {arrowX,        arrowY + 5.0f},
        arrowColor
      );
    }

    int nActions = (int)payload->actions.size();
    std::string label = std::to_string(nActions) + (nActions == 1 ? " ação executada" : " ações executadas");
    float labelFontSize = 13.0f;
    Vector2 labelPos = {scrolledRect.x + 30.0f, scrolledRect.y + (HEADER_HEIGHT / 2.0f) - 7.0f};
    DrawTextEx(font, label.c_str(), labelPos, labelFontSize, 1.0f,
               headerHovered ? Theme::TextMain : Theme::TextSec);

    if (expanded) {
      DrawLine(scrolledRect.x + 8, scrolledRect.y + HEADER_HEIGHT,
               scrolledRect.x + scrolledRect.width - 8,
               scrolledRect.y + HEADER_HEIGHT, Theme::Border);

      float rowY = scrolledRect.y + HEADER_HEIGHT + 4.0f;
      float actionFontSize = 13.0f;

      for (const auto& action : payload->actions) {
        float iconX = scrolledRect.x + 14.0f;
        float iconY = rowY + ACTION_ROW_HEIGHT / 2.0f;

        if (action.success) {
          DrawLineEx({iconX,      iconY + 2.0f}, {iconX + 4.0f, iconY + 6.0f}, 1.8f, Theme::Success);
          DrawLineEx({iconX + 4.0f, iconY + 6.0f}, {iconX + 10.0f, iconY - 3.0f}, 1.8f, Theme::Success);
        } else {
          DrawLineEx({iconX,      iconY - 4.0f}, {iconX + 10.0f, iconY + 4.0f}, 1.8f, Theme::Danger);
          DrawLineEx({iconX,      iconY + 4.0f}, {iconX + 10.0f, iconY - 4.0f}, 1.8f, Theme::Danger);
        }

        Vector2 namePos = {scrolledRect.x + 30.0f, rowY + (ACTION_ROW_HEIGHT / 2.0f) - 7.0f};
        Color nameColor = action.success ? Theme::TextSec : Theme::Danger;
        DrawTextEx(font, action.name.c_str(), namePos, actionFontSize, 1.0f, nameColor);

        if (!action.error.empty()) {
          std::string errLabel = "  — " + action.error;
          float errX = namePos.x + MeasureTextEx(font, action.name.c_str(), actionFontSize, 1.0f).x;
          DrawTextEx(font, errLabel.c_str(), {errX, namePos.y}, actionFontSize - 1.0f, 1.0f, Theme::Danger);
        }

        rowY += ACTION_ROW_HEIGHT;
      }
    }

    return headerHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
  }
};

} // namespace jay
