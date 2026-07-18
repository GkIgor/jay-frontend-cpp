module;
#include <raylib.h>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
export module message_bubble;

import theme;

export namespace jay {

struct ChatBubbleData {
  Rectangle rect;
  Color bubbleColor;
  Color textColor;
  std::vector<std::string> lines;
  bool isUser;
  std::string label;
  std::string originalText;
  bool isLoading;
  int originalIndex;
};

class MessageBubble {
public:
  static void Draw(Font font, const ChatBubbleData& bubble, float scrollOffset, float fontSize, float labelFontSize, Vector2 mousePos, int selectionBubbleIdx, int selectionStartChar, int selectionEndChar, int copiedBubbleIdx, float copiedTimer) {
    Rectangle scrolledRect = bubble.rect;
    scrolledRect.y += scrollOffset;

    if (bubble.isLoading) {
      DrawRectangleRounded(scrolledRect, 0.2f, 4, bubble.bubbleColor);
      Vector2 labelPos = {scrolledRect.x + 10, scrolledRect.y - 15};
      DrawTextEx(font, bubble.label.c_str(), labelPos, labelFontSize, 1.0f, Theme::TextSec);

      float cy = scrolledRect.y + 21;
      float t = (float)GetTime() * 8.0f;
      float dot1Y = cy + sinf(t + 0.0f) * 5.0f;
      float dot2Y = cy + sinf(t + 1.2f) * 5.0f;
      float dot3Y = cy + sinf(t + 2.4f) * 5.0f;

      DrawCircle(scrolledRect.x + 25, dot1Y, 4.0f, Theme::TextMain);
      DrawCircle(scrolledRect.x + 45, dot2Y, 4.0f, Theme::TextMain);
      DrawCircle(scrolledRect.x + 65, dot3Y, 4.0f, Theme::TextMain);
    } else {
      DrawRectangleRounded(scrolledRect, 0.2f, 4, bubble.bubbleColor);
      Vector2 labelPos = {scrolledRect.x + 10, scrolledRect.y - 15};
      DrawTextEx(font, bubble.label.c_str(), labelPos, labelFontSize, 1.0f, Theme::TextSec);

      // Desenha destaques de seleção caractere por caractere (com contraste ajustado)
      if (selectionBubbleIdx == bubble.originalIndex && selectionStartChar != selectionEndChar) {
        int startChar = std::min(selectionStartChar, selectionEndChar);
        int endChar = std::max(selectionStartChar, selectionEndChar);

        int charAbsIdx = 0;
        for (size_t l = 0; l < bubble.lines.size(); ++l) {
          const auto& line = bubble.lines[l];
          int lineY = scrolledRect.y + 10 + l * 24;

          for (size_t i = 0; i < line.length(); ++i) {
            if (charAbsIdx >= startChar && charAbsIdx < endChar) {
              float x1 = MeasureTextEx(font, line.substr(0, i).c_str(), fontSize, 1.0f).x;
              float x2 = MeasureTextEx(font, line.substr(0, i + 1).c_str(), fontSize, 1.0f).x;
              Rectangle hlRect = { scrolledRect.x + 16 + x1, (float)lineY, x2 - x1, 24.0f };
              
              // Ajusta a cor do highlight dependendo se a bolha é do usuário para manter contraste
              Color hlColor = bubble.isUser ? GetColor(0xFFFFFF44) : GetColor(0x1F6FEB88);
              DrawRectangleRec(hlRect, hlColor);
            }
            charAbsIdx++;
          }
          charAbsIdx++; // +1 pelo caractere de quebra implícito (\n)
        }
      }

      // Texto interno
      int lineY = scrolledRect.y + 10;
      for (const auto& line : bubble.lines) {
        Vector2 textPos = {scrolledRect.x + 16, (float)lineY};
        DrawTextEx(font, line.c_str(), textPos, fontSize, 1.0f, bubble.textColor);
        lineY += 24;
      }

      // Ícone de copiar abaixo do balão
      Rectangle copyBtnRect;
      if (bubble.isUser) {
        copyBtnRect = { scrolledRect.x + scrolledRect.width - 32, scrolledRect.y + scrolledRect.height + 4, 24.0f, 24.0f };
      } else {
        copyBtnRect = { scrolledRect.x, scrolledRect.y + scrolledRect.height + 4, 24.0f, 24.0f };
      }

      bool isCopyHovered = CheckCollisionPointRec(mousePos, copyBtnRect);
      Color iconColor = isCopyHovered ? Theme::Glow : Theme::TextSec;

      float cx = copyBtnRect.x;
      float cy = copyBtnRect.y;

      if (copiedBubbleIdx == bubble.originalIndex && copiedTimer > 0.0f) {
        // Desenha um checkmark (✓) de sucesso
        DrawLineEx({cx + 4, cy + 12}, {cx + 9, cy + 17}, 2.0f, Theme::Executing);
        DrawLineEx({cx + 9, cy + 17}, {cx + 18, cy + 7}, 2.0f, Theme::Executing);
      } else {
        // Desenha duas folhas sobrepostas (ícone de copiar)
        Rectangle r1 = {cx + 4, cy + 4, 10, 12};
        DrawRectangleLinesEx(r1, 1.5f, iconColor);

        Rectangle r2 = {cx + 8, cy + 8, 10, 12};
        DrawRectangleRec(r2, Theme::Background); // Esconde a linha inferior por baixo
        DrawRectangleLinesEx(r2, 1.5f, iconColor);
      }
    }
  }
};

} // namespace jay
