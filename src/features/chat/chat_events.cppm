module;
#include <raylib.h>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
export module chat_events;

import message_bubble;

export namespace jay {

class ChatEvents {
public:
  int m_selectionBubbleIdx = -1;
  int m_selectionStartChar = -1;
  int m_selectionEndChar = -1;

  int m_copiedBubbleIdx = -1;
  float m_copiedTimer = 0.0f;

  float m_scrollOffset = 0.0f;
  size_t m_prevMsgCount = 0;

  // Estados de rolagem e drag (ADR-001 UX)
  bool m_isDraggingScrollbar = false;
  float m_dragStartY = 0.0f;
  float m_inputScrollOffset = 0.0f;

  void Update(float frameTime) {
    if (m_copiedTimer > 0.0f) {
      m_copiedTimer -= frameTime;
    }
  }

  // Manipula o arraste (slide) da barra de rolagem pelo mouse
  void HandleScrollbarDrag(float contentHeight, float visibleHeight, float tabHeight, int screenWidth, Vector2 mousePos) {
    if (contentHeight <= visibleHeight) {
      m_isDraggingScrollbar = false;
      return;
    }

    Rectangle track = { screenWidth - 12.0f, tabHeight + 4.0f, 6.0f, visibleHeight - 8.0f };
    float thumbHeight = (visibleHeight / contentHeight) * track.height;
    if (thumbHeight < 30.0f) thumbHeight = 30.0f;

    float maxScroll = visibleHeight - contentHeight;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      float scrollPercent = (maxScroll != 0.0f) ? (m_scrollOffset / maxScroll) : 0.0f;
      float thumbY = track.y + scrollPercent * (track.height - thumbHeight);
      Rectangle thumb = { track.x, thumbY, track.width, thumbHeight };

      if (CheckCollisionPointRec(mousePos, thumb)) {
        m_isDraggingScrollbar = true;
        m_dragStartY = mousePos.y - thumbY;
      }
    }

    if (m_isDraggingScrollbar) {
      if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        float targetThumbY = mousePos.y - m_dragStartY;
        float percent = (targetThumbY - track.y) / (track.height - thumbHeight);
        if (percent < 0.0f) percent = 0.0f;
        if (percent > 1.0f) percent = 1.0f;
        m_scrollOffset = percent * maxScroll;
      } else {
        m_isDraggingScrollbar = false;
      }
    }
  }

  int GetCharIndexAtMouse(Font font, const std::string& text, const std::vector<std::string>& wrappedLines, Rectangle scrolledRect, float fontSize, Vector2 mousePos) {
    int lineIdx = (int)((mousePos.y - (scrolledRect.y + 10)) / 24);
    if (lineIdx < 0) lineIdx = 0;
    if (lineIdx >= (int)wrappedLines.size()) lineIdx = (int)wrappedLines.size() - 1;

    const auto& line = wrappedLines[lineIdx];
    int colIdx = 0;
    float minDiff = 99999.0f;

    float localX = mousePos.x - (scrolledRect.x + 16);
    for (size_t i = 0; i <= line.length(); ++i) {
      float w = MeasureTextEx(font, line.substr(0, i).c_str(), fontSize, 1.0f).x;
      float diff = fabsf(w - localX);
      if (diff < minDiff) {
        minDiff = diff;
        colIdx = (int)i;
      }
    }

    // Calcula o índice absoluto no texto exibido
    int absIdx = 0;
    for (int l = 0; l < lineIdx; ++l) {
      absIdx += (int)wrappedLines[l].length() + 1; // +1 pelo caractere de quebra implícito
    }
    return absIdx + colIdx;
  }

  void HandleMouseClicks(Font font, float fontSize, const std::vector<ChatBubbleData>& renderList, Vector2 mousePos, bool isMousePressed, bool isMouseDown) {
    if (isMousePressed) {
      m_selectionBubbleIdx = -1;
      m_selectionStartChar = -1;
      m_selectionEndChar = -1;

      for (const auto& bubble : renderList) {
        if (bubble.isLoading) continue;

        Rectangle scrolledRect = bubble.rect;
        scrolledRect.y += m_scrollOffset;

        Rectangle copyBtnRect;
        if (bubble.isUser) {
          copyBtnRect = { scrolledRect.x + scrolledRect.width - 32, scrolledRect.y + scrolledRect.height + 4, 24.0f, 24.0f };
        } else {
          copyBtnRect = { scrolledRect.x, scrolledRect.y + scrolledRect.height + 4, 24.0f, 24.0f };
        }

        if (CheckCollisionPointRec(mousePos, copyBtnRect)) {
          // Copia o texto reconstruindo as quebras originais
          std::string dispText = "";
          for (size_t l = 0; l < bubble.lines.size(); ++l) {
            dispText += bubble.lines[l];
            if (l + 1 < bubble.lines.size()) dispText += "\n";
          }
          SetClipboardText(dispText.c_str());
          m_copiedBubbleIdx = bubble.originalIndex;
          m_copiedTimer = 1.5f;
          break;
        }

        if (CheckCollisionPointRec(mousePos, scrolledRect)) {
          m_selectionBubbleIdx = bubble.originalIndex;
          m_selectionStartChar = GetCharIndexAtMouse(font, bubble.originalText, bubble.lines, scrolledRect, fontSize, mousePos);
          m_selectionEndChar = m_selectionStartChar;
          break;
        }
      }
    }

    if (isMouseDown && m_selectionBubbleIdx != -1) {
      for (const auto& bubble : renderList) {
        if (bubble.originalIndex == m_selectionBubbleIdx) {
          Rectangle scrolledRect = bubble.rect;
          scrolledRect.y += m_scrollOffset;
          m_selectionEndChar = GetCharIndexAtMouse(font, bubble.originalText, bubble.lines, scrolledRect, fontSize, mousePos);
          break;
        }
      }
    }
  }

  void HandleKeyboardCopy(const std::vector<ChatBubbleData>& renderList) {
    bool ctrlPressed = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    if (ctrlPressed && IsKeyPressed(KEY_C)) {
      if (m_selectionBubbleIdx != -1 && m_selectionStartChar != m_selectionEndChar) {
        int start = std::min(m_selectionStartChar, m_selectionEndChar);
        int end = std::max(m_selectionStartChar, m_selectionEndChar);

        std::string targetText = "";
        for (const auto& bubble : renderList) {
          if (bubble.originalIndex == m_selectionBubbleIdx) {
            std::string dispText = "";
            for (size_t l = 0; l < bubble.lines.size(); ++l) {
              dispText += bubble.lines[l];
              if (l + 1 < bubble.lines.size()) dispText += "\n";
            }
            if (start >= 0 && end <= (int)dispText.length()) {
              targetText = dispText.substr(start, end - start);
            }
            break;
          }
        }
        if (!targetText.empty()) {
          SetClipboardText(targetText.c_str());
        }
      }
    }
  }
};

} // namespace jay
