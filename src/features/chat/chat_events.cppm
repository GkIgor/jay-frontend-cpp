module;
#include <raylib.h>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
export module chat_events;

import message_bubble;
import chat_state;
import scroll_controller;
import scrollbar;
import theme;

export namespace jay {

class ChatEvents {
public:
  int m_selectionBubbleIdx = -1;
  int m_selectionStartChar = -1;
  int m_selectionEndChar = -1;

  int m_copiedBubbleIdx = -1;
  float m_copiedTimer = 0.0f;

  ScrollController m_chatScroll{Theme::WheelSensChat};
  ScrollController m_inputScroll{Theme::WheelSensInput};
  size_t m_prevMsgCount = 0;

  void Update(float frameTime) {
    if (m_copiedTimer > 0.0f) {
      m_copiedTimer -= frameTime;
    }
  }

  void HandleChatScrollbarDrag(const Scrollbar& scrollbar, Rectangle chatBounds,
                                float contentHeight, float visibleHeight) {
    if (!scrollbar.IsVisible(contentHeight, visibleHeight)) return;

    float maxScroll = visibleHeight - contentHeight; // negativo quando content > visible

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      Rectangle thumbRect = scrollbar.GetThumbRect(chatBounds, -m_chatScroll.GetOffset(),
        contentHeight, visibleHeight);
      m_chatScroll.TryBeginDrag(GetMousePosition(), thumbRect);
    }

    Rectangle track = scrollbar.GetTrackRect(chatBounds);
    float thumbH = scrollbar.GetThumbRect(chatBounds, -m_chatScroll.GetOffset(),
      contentHeight, visibleHeight).height;

    m_chatScroll.UpdateDrag(GetMousePosition().y, track.y, track.height, thumbH, maxScroll);
    m_chatScroll.EndDragIfReleased();
  }

  void HandleInputScrollbarDrag(const Scrollbar& scrollbar, Rectangle inputField,
                                 int inputLines, float stepY) {
    if (inputLines <= 10) return;

    float contentHeight = inputLines * stepY;
    float visibleHeight = inputField.height - 8.0f;
    float maxScroll = std::max(0.0f, (inputLines - 10) * stepY);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      Rectangle thumbRect = scrollbar.GetThumbRect(inputField, m_inputScroll.GetOffset(),
        contentHeight, visibleHeight);
      m_inputScroll.TryBeginDrag(GetMousePosition(), thumbRect);
    }

    Rectangle track = scrollbar.GetTrackRect(inputField);
    float thumbH = scrollbar.GetThumbRect(inputField, m_inputScroll.GetOffset(),
      contentHeight, visibleHeight).height;

    m_inputScroll.UpdateDrag(GetMousePosition().y, track.y, track.height, thumbH, maxScroll);
    m_inputScroll.EndDragIfReleased();
  }

  int GetCharIndexAtMouse(Font font, const std::string& text, const std::vector<std::string>& wrappedLines, Rectangle scrolledRect, float fontSize, Vector2 mousePos) {
    if (wrappedLines.empty()) return 0;

    int lineIdx = (int)((mousePos.y - (scrolledRect.y + 10)) / Theme::BubbleStepY);
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

    int absIdx = 0;
    for (int l = 0; l < lineIdx; ++l) {
      absIdx += (int)wrappedLines[l].length() + 1;
    }
    return absIdx + colIdx;
  }

  void HandleMouseClicks(Font font, float fontSize, const std::vector<ChatBubbleData>& renderList, Vector2 mousePos, bool isMousePressed, bool isMouseDown) {
    if (isMousePressed) {
      m_selectionBubbleIdx = -1;
      m_selectionStartChar = -1;
      m_selectionEndChar = -1;

      for (const auto& bubble : renderList) {
        if (bubble.isLoading || bubble.kind == ChatKind::ToolGroup || bubble.lines.empty()) continue;

        Rectangle scrolledRect = bubble.rect;
        scrolledRect.y += m_chatScroll.GetOffset();

        Rectangle copyBtnRect;
        if (bubble.isUser) {
          copyBtnRect = { scrolledRect.x + scrolledRect.width - 32, scrolledRect.y + scrolledRect.height + 4, 24.0f, 24.0f };
        } else {
          copyBtnRect = { scrolledRect.x, scrolledRect.y + scrolledRect.height + 4, 24.0f, 24.0f };
        }

        if (CheckCollisionPointRec(mousePos, copyBtnRect)) {
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
          scrolledRect.y += m_chatScroll.GetOffset();
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
