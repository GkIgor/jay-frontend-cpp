module;
#include <raylib.h>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <algorithm>
#include <unordered_map>
#include <variant>
#include <nlohmann/json.hpp>
export module chat_renderer;

import app_state;
import avatar_state;
import chat_state;
import theme;
import text_input;
import ipc_client;
import scroll_view;
import message_bubble;
import chat_input;
import chat_events;
import bubble_registry;

using json = nlohmann::json;

export namespace jay {

class ChatRenderer {
public:
  ChatRenderer() : m_cmdCounter(1), m_prevMinScroll(0.0f), m_prevInputText("") {}

  void Draw(const std::shared_ptr<ApplicationState>& state,
            const std::shared_ptr<IPCClient>& ipcClient,
            Font font, int screenWidth, int screenHeight) {
    const int tabHeight = 60;
    m_events.Update(GetFrameTime());

    std::string currentText = m_textInput.GetText();
    int inputLines = 1;
    for (char c : currentText) if (c == '\n') inputLines++;
    int visibleInputLines = std::min(inputLines, 10);

    const float stepY = 18.0f;
    int dynamicInputHeight = 80 + (visibleInputLines - 1) * stepY;
    int chatAreaHeight = screenHeight - tabHeight - dynamicInputHeight;

    float wheel     = GetMouseWheelMove();
    Vector2 mousePos = GetMousePosition();

    int iy = screenHeight - dynamicInputHeight;
    float inputFieldHeight = 42.0f + (visibleInputLines - 1) * stepY;
    Rectangle inputField = {40.0f, (float)(iy + 19), (float)(screenWidth - 220), inputFieldHeight};
    bool isMouseOverInput = CheckCollisionPointRec(mousePos, inputField);

    if (wheel != 0) {
      if (isMouseOverInput && inputLines > 10) {
        m_events.m_inputScrollOffset -= wheel * 18.0f;
      } else {
        m_events.m_scrollOffset += wheel * 48.0f;
      }
    }
    if (IsKeyDown(KEY_UP))   m_events.m_scrollOffset += 6.0f;
    if (IsKeyDown(KEY_DOWN)) m_events.m_scrollOffset -= 6.0f;

    float maxInputScroll = std::max(0.0f, (inputLines - 10) * stepY);
    if (currentText != m_prevInputText) {
      m_events.m_inputScrollOffset = std::min(m_events.m_inputScrollOffset, maxInputScroll);
      if (inputLines > 10) {
        float cursorY = (inputLines - 1) * stepY;
        if (cursorY >= m_events.m_inputScrollOffset + 10 * stepY) {
          m_events.m_inputScrollOffset = (inputLines - 10) * stepY;
        }
      }
      m_prevInputText = currentText;
    }
    m_events.m_inputScrollOffset = std::clamp(m_events.m_inputScrollOffset, 0.0f, maxInputScroll);
    m_events.HandleInputScrollbarDrag(inputLines, stepY, inputField, mousePos);

    std::vector<ChatMessage> messages = state->chat.GetChatFeed();
    float fontSize      = 18.0f;
    float labelFontSize = 12.0f;
    int maxBubbleWidth  = 640;
    int spacing         = 20;

    std::vector<ChatBubbleData> renderList;
    int currentY = tabHeight + 25;

    for (int i = 0; i < (int)messages.size(); ++i) {
      const auto& msg = messages[i];
      bool expanded = GetExpanded(msg.id);

      ChatBubbleData bubble;
      bubble.id            = msg.id;
      bubble.originalIndex = i;
      bubble.kind          = msg.kind;
      bubble.payload       = msg.payload;
      bubble.isLoading     = false;
      bubble.originalText  = msg.text;

      switch (msg.kind) {
        case ChatKind::User:
          bubble.bubbleColor = Theme::UserBubble;
          bubble.textColor   = WHITE;
          bubble.isUser      = true;
          bubble.label       = "VOCÊ";
          bubble.lines       = WrapText(font, Trim(msg.text), maxBubbleWidth - 32, fontSize);
          break;

        case ChatKind::Assistant:
          bubble.bubbleColor = Theme::JayBubble;
          bubble.textColor   = Theme::TextMain;
          bubble.isUser      = false;
          bubble.label       = "JAY";
          bubble.lines       = WrapText(font, Trim(msg.text), maxBubbleWidth - 32, fontSize);
          break;

        case ChatKind::Error:
          bubble.bubbleColor = Theme::DenyBtn;
          bubble.textColor   = WHITE;
          bubble.isUser      = false;
          bubble.label       = "ERRO";
          bubble.lines       = WrapText(font, Trim(msg.text), maxBubbleWidth - 32, fontSize);
          break;

        case ChatKind::ToolGroup:
          bubble.bubbleColor = Theme::Surface;
          bubble.textColor   = Theme::TextSec;
          bubble.isUser      = false;
          bubble.label       = "";
          bubble.lines       = {};
          break;

        default:
          bubble.bubbleColor = Theme::Border;
          bubble.textColor   = Theme::TextSec;
          bubble.isUser      = false;
          bubble.label       = "SISTEMA";
          bubble.lines       = WrapText(font, Trim(msg.text), maxBubbleWidth - 32, fontSize);
          break;
      }

      float bubbleWidth, bubbleHeight;
      if (msg.kind == ChatKind::ToolGroup) {
        bubbleWidth  = 340.0f;
        bubbleHeight = m_registry.CalculateHeight(bubble, expanded);
      } else {
        int measuredWidth = 0;
        for (const auto& line : bubble.lines) {
          Vector2 sz = MeasureTextEx(font, line.c_str(), fontSize, 1.0f);
          if ((int)sz.x > measuredWidth) measuredWidth = (int)sz.x;
        }
        bubbleWidth  = measuredWidth + 32;
        bubbleHeight = m_registry.CalculateHeight(bubble, expanded);
      }

      float x = bubble.isUser ? (float)(screenWidth - bubbleWidth - 40) : 40.0f;
      bubble.rect = {x, (float)currentY, bubbleWidth, bubbleHeight};

      renderList.push_back(std::move(bubble));
      currentY += (int)bubbleHeight + 22 + spacing;
    }

    bool isWaiting = (state->avatar.GetState() == State::Thinking ||
                      state->avatar.GetState() == State::Executing);
    if (isWaiting) {
      ChatBubbleData loading;
      loading.id            = "__loading__";
      loading.originalIndex = -1;
      loading.kind          = ChatKind::Assistant;
      loading.isLoading     = true;
      loading.isUser        = false;
      loading.label         = "JAY ESTÁ RESPONDENDO...";
      loading.bubbleColor   = Theme::JayBubble;
      loading.textColor     = Theme::TextMain;
      loading.lines         = {"..."};
      loading.payload       = std::monostate{};
      int loadH = 42;
      loading.rect = {40.0f, (float)currentY, 90.0f, (float)loadH};
      renderList.push_back(std::move(loading));
      currentY += loadH + 22 + spacing;
    }

    int totalFeedHeight = currentY - (tabHeight + 25);
    float minScroll = 0.0f;
    if (totalFeedHeight > chatAreaHeight - 50) {
      minScroll = (float)(chatAreaHeight - 50 - totalFeedHeight);
    }

    if (messages.size() > m_events.m_prevMsgCount) {
      bool lastIsUser   = (!messages.empty() && messages.back().sender == "user");
      bool wasNearBottom = (m_events.m_scrollOffset <= m_prevMinScroll + 50.0f);
      if (lastIsUser || wasNearBottom) {
        m_events.m_scrollOffset = minScroll;
      }
      m_events.m_prevMsgCount = messages.size();
    }
    if (m_events.m_scrollOffset > 0.0f)     m_events.m_scrollOffset = 0.0f;
    if (m_events.m_scrollOffset < minScroll) m_events.m_scrollOffset = minScroll;

    bool isMouseDown    = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    bool isMousePressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    m_events.HandleMouseClicks(font, fontSize, renderList, mousePos, isMousePressed, isMouseDown);
    m_events.HandleKeyboardCopy(renderList);
    m_events.HandleScrollbarDrag((float)totalFeedHeight, (float)chatAreaHeight - 50,
                                  (float)tabHeight, screenWidth, mousePos);

    BeginScissorMode(0, tabHeight, screenWidth, chatAreaHeight);

    for (auto& bubble : renderList) {
      bool expanded = GetExpanded(bubble.id);
      bool toggled  = m_registry.Draw(
        font, bubble, m_events.m_scrollOffset, fontSize, labelFontSize, mousePos, expanded,
        m_events.m_selectionBubbleIdx, m_events.m_selectionStartChar, m_events.m_selectionEndChar,
        m_events.m_copiedBubbleIdx, m_events.m_copiedTimer
      );
      if (toggled) {
        m_expandedBubbles[bubble.id] = !expanded;
      }
    }

    EndScissorMode();

    ScrollView::DrawScrollbar(m_events.m_scrollOffset, (float)totalFeedHeight,
                              (float)chatAreaHeight - 50, (float)tabHeight, screenWidth);
    m_prevMinScroll = minScroll;

    bool triggerSend  = false;
    bool ctrlPressed  = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    bool blockShorts  = (m_events.m_selectionBubbleIdx != -1 &&
                         m_events.m_selectionStartChar != m_events.m_selectionEndChar);

    ChatInput::Draw(font, m_textInput, screenWidth, screenHeight, iy,
                    dynamicInputHeight, inputLines, visibleInputLines,
                    blockShorts, mousePos, triggerSend, ctrlPressed,
                    isWaiting, m_events.m_inputScrollOffset);

    if (triggerSend && !m_textInput.IsEmpty() && !isWaiting) {
      std::string text = m_textInput.GetText();
      m_textInput.Clear();
      m_events.m_inputScrollOffset = 0.0f;

      long long ts = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
      state->chat.AddChatMessage("user-msg-" + std::to_string(ts), "user", text, ts,
                                 ChatKind::User);

      json cmd = {
        {"type", "command"},
        {"payload", {
          {"id",     "gui-cmd-" + std::to_string(m_cmdCounter++)},
          {"action", "chat"},
          {"data",   text}
        }}
      };
      ipcClient->SendMessage(cmd.dump());
    }
  }

private:
  TextInput    m_textInput;
  ChatEvents   m_events;
  BubbleRegistry m_registry;
  int          m_cmdCounter;
  float        m_prevMinScroll;
  std::string  m_prevInputText;

  std::unordered_map<std::string, bool> m_expandedBubbles;

  bool GetExpanded(const std::string& id) const {
    auto it = m_expandedBubbles.find(id);
    return it != m_expandedBubbles.end() ? it->second : false;
  }

  std::string Trim(const std::string& s) {
    size_t first = s.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = s.find_last_not_of(" \t\n\r");
    return s.substr(first, last - first + 1);
  }

  std::vector<std::string> WrapText(Font font, const std::string& text, int maxWidth, float fontSize) {
    std::vector<std::string> lines;
    std::string currentLine, word;
    for (char c : text) {
      if (c == '\n') {
        currentLine += word; lines.push_back(currentLine);
        currentLine = ""; word = "";
      } else if (c == ' ') {
        std::string testLine = currentLine + word + " ";
        if (MeasureTextEx(font, testLine.c_str(), fontSize, 1.0f).x > maxWidth) {
          lines.push_back(currentLine);
          currentLine = word + " ";
        } else {
          currentLine = testLine;
        }
        word = "";
      } else {
        word += c;
      }
    }
    if (!word.empty() || !currentLine.empty()) {
      std::string testLine = currentLine + word;
      if (MeasureTextEx(font, testLine.c_str(), fontSize, 1.0f).x > maxWidth) {
        lines.push_back(currentLine);
        lines.push_back(word);
      } else {
        lines.push_back(testLine);
      }
    }
    return lines;
  }
};

} // namespace jay
