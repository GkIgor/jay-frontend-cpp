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
import text_layout;
import ipc_client;
import scrollbar;
import scroll_controller;
import message_bubble;
import chat_input;
import chat_events;
import bubble_registry;

using json = nlohmann::json;

export namespace jay {

struct CachedBubbleLayout {
  TextLayout layout;
  int measuredWidth = 0;
};

// Comentário Arquitetural (BubbleLayout):
// Futuramente, o layout das bolhas de chat (CachedBubbleLayout) poderá reutilizar
// o TextLayout quando ambos suportarem a mesma configuração de layout (stepY, fonte, maxWidth).
// Por enquanto, a unificação está adiada pois as bolhas usam BubbleStepY=24px e o input usa StepY=18px.

class ChatRenderer {
public:
  ChatRenderer()
      : m_cmdCounter(1), m_prevMinScroll(0.0f), m_prevInputText(""),
        m_prevScreenWidth(0), m_prevScreenHeight(0), m_prevMsgCount(0),
        m_prevIsWaiting(false), m_needsRebuild(true), m_totalFeedHeight(0) {}

  void Draw(const std::shared_ptr<ApplicationState>& state,
            const std::shared_ptr<IPCClient>& ipcClient,
            Font font, int screenWidth, int screenHeight) {
    const int tabHeight = 60;
    m_events.Update(GetFrameTime());

    std::string currentText = m_textInput.GetText();

    // Calcula inputLines via TextLayout (única fonte de verdade)
    float inputTextSize = 18.0f;
    float scrollBarWidthReserve = 16.0f; // Reserva para possível scrollbar
    float inputFieldBaseWidth = (float)(screenWidth - 220);
    float maxInputWidth = inputFieldBaseWidth - 24.0f - scrollBarWidthReserve;

    TextLayoutConfig inputLayoutConfig;
    inputLayoutConfig.font = font;
    inputLayoutConfig.fontSize = inputTextSize;
    inputLayoutConfig.maxWidth = maxInputWidth;
    m_textInput.RebuildLayout(inputLayoutConfig);

    int inputLines = m_textInput.GetLayout().GetLineCount();
    int visibleInputLines = std::min(inputLines, 10);

    const float stepY = Theme::StepY;
    int dynamicInputHeight = 80 + (visibleInputLines - 1) * stepY;
    int chatAreaHeight = screenHeight - tabHeight - dynamicInputHeight;

    float wheel = GetMouseWheelMove();
    Vector2 mousePos = GetMousePosition();

    int iy = screenHeight - dynamicInputHeight;
    float inputFieldHeight = 42.0f + (visibleInputLines - 1) * stepY;
    Rectangle inputField = {40.0f, (float)(iy + 19), inputFieldBaseWidth, inputFieldHeight};
    bool isMouseOverInput = CheckCollisionPointRec(mousePos, inputField);

    if (wheel != 0) {
      if (isMouseOverInput && inputLines > 10) {
        m_events.m_inputScroll.ApplyWheel(-wheel);
      } else {
        m_events.m_chatScroll.ApplyWheel(wheel);
      }
    }

    if (IsKeyDown(KEY_UP))   m_events.m_chatScroll.ScrollByKeyboard(6.0f);
    if (IsKeyDown(KEY_DOWN)) m_events.m_chatScroll.ScrollByKeyboard(-6.0f);

    float maxInputScroll = std::max(0.0f, (inputLines - 10) * stepY);
    if (currentText != m_prevInputText) {
      float inputOffset = m_events.m_inputScroll.GetOffset();
      if (inputOffset > maxInputScroll) {
        m_events.m_inputScroll.SetOffset(maxInputScroll);
      }
      if (inputLines > 10) {
        float cursorY = (inputLines - 1) * stepY;
        if (cursorY >= m_events.m_inputScroll.GetOffset() + 10 * stepY) {
          m_events.m_inputScroll.SetOffset((inputLines - 10) * stepY);
        }
      }
      m_prevInputText = currentText;
    }

    m_events.m_inputScroll.Clamp(0.0f, maxInputScroll);
    m_events.HandleInputScrollbarDrag(m_inputScrollbar, inputField, inputLines, stepY);

    std::vector<ChatMessage> messages = state->chat.GetChatFeed();
    if (messages.empty() && !m_layoutCache.empty()) {
      m_layoutCache.clear();
      m_needsRebuild = true;
    }

    bool isWaiting = (state->avatar.GetState() == State::Thinking ||
                      state->avatar.GetState() == State::Executing);

    bool sizeChanged = (screenWidth != m_prevScreenWidth || screenHeight != m_prevScreenHeight);
    bool msgChanged  = (messages.size() != m_prevMsgCount);
    bool waitChanged = (isWaiting != m_prevIsWaiting);

    if (sizeChanged || msgChanged || waitChanged || m_needsRebuild) {
      RebuildLayout(messages, font, screenWidth, screenHeight, isWaiting, tabHeight);
      m_prevScreenWidth  = screenWidth;
      m_prevScreenHeight = screenHeight;
      m_prevMsgCount     = messages.size();
      m_prevIsWaiting    = isWaiting;
      m_needsRebuild     = false;
    }

    float fontSize = 18.0f;
    float labelFontSize = 12.0f;
    int totalFeedHeight = m_totalFeedHeight;
    
    float minScroll = 0.0f;
    if (totalFeedHeight > chatAreaHeight - 50) {
      minScroll = (float)(chatAreaHeight - 50 - totalFeedHeight);
    }

    if (messages.size() > m_events.m_prevMsgCount) {
      bool lastIsUser = (!messages.empty() && messages.back().sender == "user");
      bool wasNearBottom = (m_events.m_chatScroll.GetOffset() <= m_prevMinScroll + 50.0f);
      if (lastIsUser || wasNearBottom) {
        m_events.m_chatScroll.SetOffset(minScroll);
      }
      m_events.m_prevMsgCount = messages.size();
    }

    m_events.m_chatScroll.Clamp(minScroll, 0.0f);

    bool isMouseDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    bool isMousePressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    m_events.HandleMouseClicks(font, fontSize, m_renderList, mousePos, isMousePressed, isMouseDown);
    m_events.HandleKeyboardCopy(m_renderList);

    Rectangle chatBounds = {0.0f, (float)tabHeight, (float)screenWidth, (float)(chatAreaHeight - 50)};
    m_events.HandleChatScrollbarDrag(m_chatScrollbar, chatBounds,
                                      (float)totalFeedHeight, (float)(chatAreaHeight - 50));

    BeginScissorMode(0, tabHeight, screenWidth, chatAreaHeight);

    for (auto& bubble : m_renderList) {
      bool expanded = GetExpanded(bubble.id);
      bool toggled  = m_registry.Draw(
        font, bubble, m_events.m_chatScroll.GetOffset(), fontSize, labelFontSize, mousePos, expanded,
        m_events.m_selectionBubbleIdx, m_events.m_selectionStartChar, m_events.m_selectionEndChar,
        m_events.m_copiedBubbleIdx, m_events.m_copiedTimer
      );
      if (toggled) {
        m_expandedBubbles[bubble.id] = !expanded;
        m_needsRebuild = true;
      }
    }

    EndScissorMode();

    m_chatScrollbar.Draw(chatBounds, -m_events.m_chatScroll.GetOffset(),
                         (float)totalFeedHeight, (float)(chatAreaHeight - 50));
    m_prevMinScroll = minScroll;

    bool triggerSend = false;
    bool ctrlPressed = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    bool blockShorts = (m_events.m_selectionBubbleIdx != -1 &&
                        m_events.m_selectionStartChar != m_events.m_selectionEndChar);

    ChatInput::Draw(font, m_textInput, screenWidth, screenHeight, iy,
                    dynamicInputHeight, inputLines, visibleInputLines,
                    blockShorts, mousePos, triggerSend, ctrlPressed,
                    isWaiting, m_events.m_inputScroll.GetOffset());

    if (triggerSend && !m_textInput.IsEmpty() && !isWaiting) {
      std::string text = m_textInput.GetText();
      m_textInput.Clear();
      m_events.m_inputScroll.SetOffset(0.0f);

      long long ts = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
      
      state->chat.AddChatMessage("user-msg-" + std::to_string(ts), "user", text, ts, ChatKind::User);

      json cmd = {
        {"type", "command"},
        {"payload", {
          {"id",     "gui-cmd-" + std::to_string(m_cmdCounter++)},
          {"action", "chat"},
          {"data",   text}
        }}
      };
      
      ipcClient->SendMessage(cmd.dump());
      m_needsRebuild = true;
    }
  }

private:
  TextInput    m_textInput;
  ChatEvents   m_events;
  BubbleRegistry m_registry;
  Scrollbar    m_chatScrollbar;
  Scrollbar    m_inputScrollbar;
  int          m_cmdCounter;
  float        m_prevMinScroll;
  std::string  m_prevInputText;

  std::unordered_map<std::string, bool> m_expandedBubbles;
  std::unordered_map<std::string, CachedBubbleLayout> m_layoutCache;

  std::vector<ChatBubbleData> m_renderList;
  int m_prevScreenWidth;
  int m_prevScreenHeight;
  size_t m_prevMsgCount;
  bool m_prevIsWaiting;
  bool m_needsRebuild;
  int m_totalFeedHeight;

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

  void RebuildLayout(const std::vector<ChatMessage>& messages, Font font,
                     int screenWidth, int screenHeight, bool isWaiting, int tabHeight) {
    m_renderList.clear();

    float fontSize = 18.0f;
    int maxBubbleWidth = 640;
    int spacing = 20;
    int currentY = tabHeight + 25;

    for (int i = 0; i < (int)messages.size(); ++i) {
      const auto& msg = messages[i];
      bool expanded = GetExpanded(msg.id);

      std::vector<std::string> bubbleLines;
      int measuredWidth = 0;

      if (msg.kind != ChatKind::ToolGroup) {
        auto it = m_layoutCache.find(msg.id);
        if (it != m_layoutCache.end()) {
          measuredWidth = it->second.measuredWidth;
          for (const auto& l : it->second.layout.GetLines()) {
            bubbleLines.push_back(l.text);
          }
        } else {
          TextLayoutConfig layoutConfig;
          layoutConfig.font = font;
          layoutConfig.fontSize = fontSize;
          layoutConfig.maxWidth = (float)(maxBubbleWidth - 32);

          TextLayout textLayout;
          textLayout.Build(Trim(msg.text), layoutConfig);

          for (const auto& l : textLayout.GetLines()) {
            Vector2 sz = MeasureTextEx(font, l.text.c_str(), fontSize, 1.0f);
            if ((int)sz.x > measuredWidth) measuredWidth = (int)sz.x;
            bubbleLines.push_back(l.text);
          }
          m_layoutCache[msg.id] = { std::move(textLayout), measuredWidth };
        }
      }

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
          bubble.lines       = bubbleLines;
          break;

        case ChatKind::Assistant:
          bubble.bubbleColor = Theme::JayBubble;
          bubble.textColor   = Theme::TextMain;
          bubble.isUser      = false;
          bubble.label       = "JAY";
          bubble.lines       = bubbleLines;
          break;

        case ChatKind::Error:
          bubble.bubbleColor = Theme::DenyBtn;
          bubble.textColor   = WHITE;
          bubble.isUser      = false;
          bubble.label       = "ERRO";
          bubble.lines       = bubbleLines;
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
          bubble.lines       = bubbleLines;
          break;
      }

      float bubbleWidth, bubbleHeight;
      if (msg.kind == ChatKind::ToolGroup) {
        bubbleWidth  = 340.0f;
        bubbleHeight = m_registry.CalculateHeight(bubble, expanded);
      } else {
        // Garante que a largura da bolha respeite o limite máximo de maxBubbleWidth
        bubbleWidth  = std::min((float)maxBubbleWidth, (float)measuredWidth + 32.0f);
        bubbleHeight = m_registry.CalculateHeight(bubble, expanded);
      }

      float x = bubble.isUser ? (float)(screenWidth - bubbleWidth - 40) : 40.0f;
      bubble.rect = {x, (float)currentY, bubbleWidth, bubbleHeight};

      m_renderList.push_back(std::move(bubble));
      currentY += (int)bubbleHeight + 22 + spacing;
    }

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
      
      m_renderList.push_back(std::move(loading));
      currentY += loadH + 22 + spacing;
    }

    m_totalFeedHeight = currentY - (tabHeight + 25);
  }
};

} // namespace jay
