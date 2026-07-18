module;
#include <raylib.h>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <algorithm>
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

using json = nlohmann::json;

export namespace jay {

class ChatRenderer {
public:
  ChatRenderer() : m_cmdCounter(1), m_prevMinScroll(0.0f), m_prevInputText("") {}

  void Draw(const std::shared_ptr<ApplicationState>& state, const std::shared_ptr<IPCClient>& ipcClient, Font font, int screenWidth, int screenHeight) {
    const int tabHeight = 60;
    
    // Atualiza lógica de timers e seleções de cliques
    m_events.Update(GetFrameTime());

    // 1. Calcula a altura dinâmica do input bar com base nas quebras de linha (Ctrl+Enter)
    std::string currentText = m_textInput.GetText();
    int inputLines = 1;
    for (char c : currentText) if (c == '\n') inputLines++;
    int visibleInputLines = std::min(inputLines, 10);
    
    const float stepY = 18.0f; // Altura de cada linha
    int dynamicInputHeight = 80 + (visibleInputLines - 1) * stepY;
    int chatAreaHeight = screenHeight - tabHeight - dynamicInputHeight;

    // Trata Scroll por rodinha do mouse (aumentada a sensibilidade de 36.0f para 48.0f)
    float wheel = GetMouseWheelMove();
    Vector2 mousePos = GetMousePosition();
    
    // Determina se o mouse está sobre a caixa de texto do input
    int iy = screenHeight - dynamicInputHeight;
    float inputFieldHeight = 42.0f + (visibleInputLines - 1) * stepY;
    Rectangle inputField = {40.0f, (float)(iy + 19), (float)(screenWidth - 220), inputFieldHeight};
    bool isMouseOverInput = CheckCollisionPointRec(mousePos, inputField);

    if (wheel != 0) {
      if (isMouseOverInput && inputLines > 10) {
        // Scroll do input por mouse wheel
        m_events.m_inputScrollOffset -= wheel * 18.0f;
      } else {
        // Scroll do chat por mouse wheel (mais sensível a 48.0f por clique)
        m_events.m_scrollOffset += wheel * 48.0f;
      }
    }
    if (IsKeyDown(KEY_UP)) {
      m_events.m_scrollOffset += 6.0f;
    }
    if (IsKeyDown(KEY_DOWN)) {
      m_events.m_scrollOffset -= 6.0f;
    }

    // Auto-ajusta o scroll do input de forma inteligente (apenas quando o texto mudar, para evitar snap back)
    float maxInputScroll = std::max(0.0f, (inputLines - 10) * stepY);
    if (currentText != m_prevInputText) {
      m_events.m_inputScrollOffset = std::min(m_events.m_inputScrollOffset, maxInputScroll);
      if (inputLines > 10) {
        // Se o cursor estiver abaixo da área visível do input, desce o scroll
        float cursorYOffset = (inputLines - 1) * stepY;
        if (cursorYOffset >= m_events.m_inputScrollOffset + 10 * stepY) {
          m_events.m_inputScrollOffset = (inputLines - 10) * stepY;
        }
      }
      m_prevInputText = currentText;
    }

    // Mantém o scroll do input travado nos limites corretos
    m_events.m_inputScrollOffset = std::clamp(m_events.m_inputScrollOffset, 0.0f, maxInputScroll);

    // Permite arraste do mouse (slide) na barra de rolagem interna do input
    m_events.HandleInputScrollbarDrag(inputLines, stepY, inputField, mousePos);

    // 2. Calcula as posições dos balões de mensagens
    std::vector<ChatMessage> messages = state->chat.GetChatFeed();
    float fontSize = 18.0f;
    float labelFontSize = 12.0f;
    int maxBubbleWidth = 640;
    int spacingBetweenBubbles = 20;

    std::vector<ChatBubbleData> renderList;
    int currentY = tabHeight + 25;

    for (size_t i = 0; i < messages.size(); ++i) {
      const auto& msg = messages[i];
      std::vector<std::string> wrappedLines = WrapText(font, msg.text, maxBubbleWidth - 32, fontSize);
      int bubbleHeight = wrappedLines.size() * 24 + 20;
      int bubbleWidth = 0;

      for (const auto& line : wrappedLines) {
        Vector2 sz = MeasureTextEx(font, line.c_str(), fontSize, 1.0f);
        if ((int)sz.x > bubbleWidth) {
          bubbleWidth = (int)sz.x;
        }
      }
      bubbleWidth += 32;

      bool isUser = (msg.sender == "user");
      int x = isUser ? (screenWidth - bubbleWidth - 40) : 40;

      Color bColor = Theme::JayBubble;
      Color tColor = Theme::TextMain;
      std::string label = "JAY";

      if (isUser) {
        bColor = Theme::UserBubble;
        tColor = WHITE;
        label = "VOCÊ";
      } else if (msg.kind == "error") {
        bColor = Theme::DenyBtn;
        tColor = WHITE;
        label = "ERRO";
      } else if (msg.kind == "tool") {
        bColor = Theme::Border;
        tColor = Theme::TextSec;
        label = "AÇÃO DO CORE";
      }

      renderList.push_back({
        {(float)x, (float)currentY, (float)bubbleWidth, (float)bubbleHeight},
        bColor,
        tColor,
        wrappedLines,
        isUser,
        label,
        msg.text,
        false,
        (int)i
      });

      currentY += bubbleHeight + 22 + spacingBetweenBubbles;
    }

    // Feedback visual se estiver esperando por resposta (Thinking ou Executing)
    bool isThinkingOrExecuting = (state->avatar.GetState() == State::Thinking || state->avatar.GetState() == State::Executing);
    if (isThinkingOrExecuting) {
      std::vector<std::string> loadingLines = {"..."};
      int bubbleHeight = 42;
      int bubbleWidth = 90;
      int x = 40;
      renderList.push_back({
        {(float)x, (float)currentY, (float)bubbleWidth, (float)bubbleHeight},
        Theme::JayBubble,
        Theme::TextMain,
        loadingLines,
        false,
        "JAY ESTÁ RESPONDENDO...",
        "",
        true,
        -1
      });
      currentY += bubbleHeight + 22 + spacingBetweenBubbles;
    }

    // Auto-scroll inteligente
    int totalFeedHeight = currentY - (tabHeight + 25);
    float minScroll = 0.0f;
    if (totalFeedHeight > chatAreaHeight - 50) {
      minScroll = (float)(chatAreaHeight - 50 - totalFeedHeight);
    }
    
    if (messages.size() > m_events.m_prevMsgCount) {
      bool isLastMsgFromUser = (!messages.empty() && messages.back().sender == "user");
      bool wasNearBottom = (m_events.m_scrollOffset <= m_prevMinScroll + 50.0f);

      if (isLastMsgFromUser || wasNearBottom) {
        m_events.m_scrollOffset = minScroll;
      }
      m_events.m_prevMsgCount = messages.size();
    }

    // Limites de scroll manual do chat
    if (m_events.m_scrollOffset > 0.0f) m_events.m_scrollOffset = 0.0f;
    if (m_events.m_scrollOffset < minScroll) m_events.m_scrollOffset = minScroll;

    // Gerencia cliques de mouse, drag e atalhos de teclado de seleção no handler
    bool isMouseDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    bool isMousePressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    m_events.HandleMouseClicks(font, fontSize, renderList, mousePos, isMousePressed, isMouseDown);
    m_events.HandleKeyboardCopy(renderList);

    // Permite arraste do mouse (slide) na barra de rolagem principal
    m_events.HandleScrollbarDrag((float)totalFeedHeight, (float)chatAreaHeight - 50, (float)tabHeight, screenWidth, mousePos);

    // 3. Renderiza a área de chat recortando conteúdos fora do limite (Scissor)
    BeginScissorMode(0, tabHeight, screenWidth, chatAreaHeight);

    for (const auto& bubble : renderList) {
      MessageBubble::Draw(font, bubble, m_events.m_scrollOffset, fontSize, labelFontSize, mousePos, m_events.m_selectionBubbleIdx, m_events.m_selectionStartChar, m_events.m_selectionEndChar, m_events.m_copiedBubbleIdx, m_events.m_copiedTimer);
    }

    EndScissorMode();

    // Desenha a barra de rolagem (ScrollView) se o conteúdo transbordar
    ScrollView::DrawScrollbar(m_events.m_scrollOffset, (float)totalFeedHeight, (float)chatAreaHeight - 50, (float)tabHeight, screenWidth);

    // Salva o minScroll deste frame para validação do próximo auto-scroll
    m_prevMinScroll = minScroll;

    // 4. Renderiza a Caixa de Entrada de Texto (Bottom Input Bar)
    bool triggerSend = false;
    bool ctrlPressed = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    bool blockShortcuts = (m_events.m_selectionBubbleIdx != -1 && m_events.m_selectionStartChar != m_events.m_selectionEndChar);

    ChatInput::Draw(font, m_textInput, screenWidth, screenHeight, iy, dynamicInputHeight, inputLines, visibleInputLines, blockShortcuts, mousePos, triggerSend, ctrlPressed, isThinkingOrExecuting, m_events.m_inputScrollOffset);

    // Bloqueia disparos de comandos se já há processamento ativo
    if (triggerSend && !m_textInput.IsEmpty() && !isThinkingOrExecuting) {
      std::string text = m_textInput.GetText();
      m_textInput.Clear();
      m_events.m_inputScrollOffset = 0.0f; // Reseta scroll do input ao enviar

      // Registra mensagem do usuário localmente
      long long timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch()).count();
      state->chat.AddChatMessage("user-msg", "user", text, timestamp, "user");

      // Dispara JSON via IPC
      json cmd = {
        {"type", "command"},
        {"payload", {
          {"id", "gui-cmd-" + std::to_string(m_cmdCounter++)},
          {"action", "chat"},
          {"data", text}
        }}
      };
      ipcClient->SendMessage(cmd.dump());
    }
  }

private:
  TextInput m_textInput;
  ChatEvents m_events;
  int m_cmdCounter;
  float m_prevMinScroll;
  std::string m_prevInputText;

  // Função utilitária para wrapping de texto Raylib por largura de pixel usando a fonte anti-aliased
  std::vector<std::string> WrapText(Font font, const std::string& text, int maxWidth, float fontSize) {
    std::vector<std::string> lines;
    std::string currentLine = "";
    std::string word = "";

    for (char c : text) {
      if (c == '\n') {
        currentLine += word;
        lines.push_back(currentLine);
        currentLine = "";
        word = "";
        continue;
      }
      if (c == ' ') {
        std::string testLine = currentLine + word + " ";
        Vector2 size = MeasureTextEx(font, testLine.c_str(), fontSize, 1.0f);
        if (size.x > maxWidth) {
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
      Vector2 size = MeasureTextEx(font, testLine.c_str(), fontSize, 1.0f);
      if (size.x > maxWidth) {
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
