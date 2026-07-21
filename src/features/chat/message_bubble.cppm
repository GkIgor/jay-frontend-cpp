module;
#include <raylib.h>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
export module message_bubble;

import chat_state;
import theme;

export namespace jay {

struct ChatBubbleData {
  std::string              id;
  Rectangle                rect;
  Color                    bubbleColor;
  Color                    textColor;
  std::vector<std::string> lines;
  bool                     isUser;
  std::string              label;
  std::string              originalText;
  bool                     isLoading;
  int                      originalIndex;
  ChatKind                 kind;
  ChatPayload              payload;
};

class MessageBubble {
public:
  static void Draw(Font font, const ChatBubbleData& bubble, float scrollOffset,
                   float fontSize, float labelFontSize, Vector2 mousePos,
                   int selectionBubbleIdx, int selectionStartChar, int selectionEndChar,
                   int copiedBubbleIdx, float copiedTimer) {
    Rectangle scrolledRect = bubble.rect;
    scrolledRect.y += scrollOffset;

    float minSide = std::min(scrolledRect.width, scrolledRect.height);
    float roundness = std::min(1.0f, (minSide > 0.0f) ? (2.0f * Theme::BubbleCornerRadius / minSide) : 0.0f);

    if (bubble.isLoading) {
      DrawRectangleRounded(scrolledRect, roundness, 4, bubble.bubbleColor);
      DrawTextEx(font, bubble.label.c_str(),
                 {scrolledRect.x + 10, scrolledRect.y - 15}, labelFontSize, 1.0f, Theme::TextSec);

      float cy = scrolledRect.y + 21;
      float t = (float)GetTime() * 8.0f;
      DrawCircle(scrolledRect.x + 25, cy + sinf(t + 0.0f) * 5.0f, 4.0f, Theme::TextMain);
      DrawCircle(scrolledRect.x + 45, cy + sinf(t + 1.2f) * 5.0f, 4.0f, Theme::TextMain);
      DrawCircle(scrolledRect.x + 65, cy + sinf(t + 2.4f) * 5.0f, 4.0f, Theme::TextMain);
      return;
    }

    DrawRectangleRounded(scrolledRect, roundness, 4, bubble.bubbleColor);
    DrawTextEx(font, bubble.label.c_str(),
               {scrolledRect.x + 10, scrolledRect.y - 15}, labelFontSize, 1.0f, Theme::TextSec);

    if (selectionBubbleIdx == bubble.originalIndex && selectionStartChar != selectionEndChar) {
      int startChar = std::min(selectionStartChar, selectionEndChar);
      int endChar   = std::max(selectionStartChar, selectionEndChar);
      int charAbsIdx = 0;
      std::string measureBuffer;
      measureBuffer.reserve(256);
      for (size_t l = 0; l < bubble.lines.size(); ++l) {
        const auto& line = bubble.lines[l];
        int lineY = scrolledRect.y + 10 + l * 24;
        for (size_t i = 0; i < line.length(); ++i) {
          if (charAbsIdx >= startChar && charAbsIdx < endChar) {
            measureBuffer.assign(line, 0, i);
            float x1 = MeasureTextEx(font, measureBuffer.c_str(), fontSize, 1.0f).x;
            measureBuffer.assign(line, 0, i + 1);
            float x2 = MeasureTextEx(font, measureBuffer.c_str(), fontSize, 1.0f).x;
            Color hlColor = bubble.isUser ? GetColor(0xFFFFFF44) : GetColor(0x1F6FEB88);
            DrawRectangleRec({scrolledRect.x + 16 + x1, (float)lineY, x2 - x1, 24.0f}, hlColor);
          }
          charAbsIdx++;
        }
        charAbsIdx++;
      }
    }

    int lineY = scrolledRect.y + 10;
    for (const auto& line : bubble.lines) {
      DrawTextEx(font, line.c_str(), {scrolledRect.x + 16, (float)lineY}, fontSize, 1.0f, bubble.textColor);
      lineY += 24;
    }

    Rectangle copyBtnRect = bubble.isUser
      ? Rectangle{scrolledRect.x + scrolledRect.width - 32, scrolledRect.y + scrolledRect.height + 4, 24.0f, 24.0f}
      : Rectangle{scrolledRect.x, scrolledRect.y + scrolledRect.height + 4, 24.0f, 24.0f};

    bool isCopyHovered = CheckCollisionPointRec(mousePos, copyBtnRect);
    Color iconColor = isCopyHovered ? Theme::Glow : Theme::TextSec;
    float cx = copyBtnRect.x;
    float cy = copyBtnRect.y;

    if (copiedBubbleIdx == bubble.originalIndex && copiedTimer > 0.0f) {
      DrawLineEx({cx + 4, cy + 12}, {cx + 9, cy + 17}, 2.0f, Theme::Executing);
      DrawLineEx({cx + 9, cy + 17}, {cx + 18, cy + 7}, 2.0f, Theme::Executing);
    } else {
      DrawRectangleLinesEx({cx + 4, cy + 4, 10, 12}, 1.5f, iconColor);
      DrawRectangleRec({cx + 8, cy + 8, 10, 12}, Theme::Background);
      DrawRectangleLinesEx({cx + 8, cy + 8, 10, 12}, 1.5f, iconColor);
    }
  }
};

} // namespace jay
