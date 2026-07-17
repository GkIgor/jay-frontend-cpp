module;
#include <raylib.h>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <nlohmann/json.hpp>
export module chat_renderer;

import app_state;
import theme;
import text_input;
import ipc_client;

using json = nlohmann::json;

export namespace jay {

class ChatRenderer {
public:
  ChatRenderer() : m_cmdCounter(1), m_scrollOffset(0.0f) {}

  void Draw(const std::shared_ptr<ApplicationState>& state, const std::shared_ptr<IPCClient>& ipcClient, Font font, int screenWidth, int screenHeight) {
    const int tabHeight = 60;   // Aumentado proporcionalmente para a nova tela
    const int inputHeight = 80;  // Altura maior para a barra de input
    const int chatAreaHeight = screenHeight - tabHeight - inputHeight;

    // 1. Trata Scroll por rodinha do mouse e teclado (Setas)
    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
      m_scrollOffset += wheel * 36.0f;
    }
    if (IsKeyDown(KEY_UP)) {
      m_scrollOffset += 6.0f;
    }
    if (IsKeyDown(KEY_DOWN)) {
      m_scrollOffset -= 6.0f;
    }

    // 2. Calcula as posições dos balões de mensagens
    std::vector<ChatMessage> messages = state->GetChatFeed();
    float fontSize = 18.0f; // Fonte bem maior e nítida
    float labelFontSize = 12.0f;
    int maxBubbleWidth = 640; // Muito mais espaço horizontal na tela dupla
    int spacingBetweenBubbles = 20;

    struct RenderBubble {
      Rectangle rect;
      Color bubbleColor;
      Color textColor;
      std::vector<std::string> lines;
      bool isUser;
      std::string label;
    };

    std::vector<RenderBubble> renderList;
    int currentY = tabHeight + 25;

    for (const auto& msg : messages) {
      std::vector<std::string> wrappedLines = WrapText(font, msg.text, maxBubbleWidth - 32, fontSize);
      int bubbleHeight = wrappedLines.size() * 24 + 20;
      int bubbleWidth = 0;

      for (const auto& line : wrappedLines) {
        Vector2 sz = MeasureTextEx(font, line.c_str(), fontSize, 1.0f);
        if ((int)sz.x > bubbleWidth) {
          bubbleWidth = (int)sz.x;
        }
      }
      bubbleWidth += 32; // Padding lateral extra

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
        label
      });

      currentY += bubbleHeight + spacingBetweenBubbles;
    }

    // Calcula o scroll automático para manter no fundo se não houver rolagem manual
    int totalFeedHeight = currentY - (tabHeight + 25);
    float minScroll = 0.0f;
    if (totalFeedHeight > chatAreaHeight - 50) {
      minScroll = (float)(chatAreaHeight - 50 - totalFeedHeight);
    }
    // Impede o usuário de rolar além dos limites
    if (m_scrollOffset > 0.0f) m_scrollOffset = 0.0f;
    if (m_scrollOffset < minScroll) m_scrollOffset = minScroll;

    // Se o usuário não moveu o scroll recentemente, auto-rola para o fundo
    if (wheel == 0 && !IsKeyDown(KEY_UP) && !IsKeyDown(KEY_DOWN)) {
      m_scrollOffset = minScroll;
    }

    // 3. Renderiza a área de chat recortando conteúdos fora do limite (Scissor)
    BeginScissorMode(0, tabHeight, screenWidth, chatAreaHeight);

    for (const auto& bubble : renderList) {
      // Aplica offset de scroll
      Rectangle scrolledRect = bubble.rect;
      scrolledRect.y += m_scrollOffset;

      // Só renderiza se estiver minimamente visível na área de corte
      if (scrolledRect.y + scrolledRect.height > tabHeight && scrolledRect.y < screenHeight - inputHeight) {
        // Fundo do balão
        DrawRectangleRounded(scrolledRect, 0.2f, 4, bubble.bubbleColor);
        // Pequena etiqueta de cabeçalho do balão com fontes anti-aliased
        Vector2 labelPos = {scrolledRect.x + 10, scrolledRect.y - 15};
        DrawTextEx(font, bubble.label.c_str(), labelPos, labelFontSize, 1.0f, Theme::TextSec);

        // Texto interno
        int lineY = scrolledRect.y + 10;
        for (const auto& line : bubble.lines) {
          Vector2 textPos = {scrolledRect.x + 16, (float)lineY};
          DrawTextEx(font, line.c_str(), textPos, fontSize, 1.0f, bubble.textColor);
          lineY += 24;
        }
      }
    }

    EndScissorMode();

    // 4. Renderiza a Caixa de Entrada de Texto (Bottom Input Bar)
    int iy = screenHeight - inputHeight;
    DrawRectangle(0, iy, screenWidth, inputHeight, Theme::Panel);
    DrawLine(0, iy, screenWidth, iy, Theme::Border);

    // Borda da caixa de texto
    Rectangle inputField = {40.0f, (float)(iy + 19), (float)(screenWidth - 220), 42.0f};
    DrawRectangleRec(inputField, Theme::Background);
    DrawRectangleLinesEx(inputField, 1.0f, Theme::Border);

    // Lê o input do teclado UTF-8
    m_textInput.Update();

    // Desenha o texto digitado
    std::string currentText = m_textInput.GetText();
    float inputTextSize = 18.0f;
    Vector2 inputTextPos = {inputField.x + 12, inputField.y + 12};

    // Desenha fundo de seleção cobrindo APENAS a largura ocupada pelo texto
    if (m_textInput.IsSelectedAll() && !currentText.empty()) {
      Vector2 textDim = MeasureTextEx(font, currentText.c_str(), inputTextSize, 1.0f);
      Rectangle selectionRect = {
        inputField.x + 10,
        inputField.y + 9,
        textDim.x + 4,
        24.0f
      };
      DrawRectangleRec(selectionRect, GetColor(0x1F6FEB88)); // Fundo azul de seleção translúcido
    }

    DrawTextEx(font, currentText.c_str(), inputTextPos, inputTextSize, 1.0f, Theme::TextMain);

    // Cursor piscante
    if (((int)(GetTime() * 2) % 2) == 0) {
      Vector2 textDim = MeasureTextEx(font, currentText.c_str(), inputTextSize, 1.0f);
      DrawRectangle(inputField.x + 14 + (int)textDim.x, inputField.y + 10, 2, 22, Theme::Glow);
    }

    // Botão Enviar
    Rectangle sendBtn = {(float)(screenWidth - 150), (float)(iy + 19), 110.0f, 42.0f};
    Vector2 mousePos = GetMousePosition();
    bool sendHovered = CheckCollisionPointRec(mousePos, sendBtn);

    DrawRectangleRounded(sendBtn, 0.2f, 4, sendHovered ? Theme::Glow : Theme::UserBubble);
    
    // Label do botão enviar centralizada
    float sendBtnFontSize = 14.0f;
    Vector2 sendTextDim = MeasureTextEx(font, "ENVIAR", sendBtnFontSize, 1.0f);
    Vector2 sendTextPos = {
      sendBtn.x + (sendBtn.width / 2) - (sendTextDim.x / 2),
      sendBtn.y + (sendBtn.height / 2) - (sendTextDim.y / 2)
    };
    DrawTextEx(font, "ENVIAR", sendTextPos, sendBtnFontSize, 1.0f, WHITE);

    // Envio de mensagem
    bool triggerSend = false;
    if (IsKeyPressed(KEY_ENTER)) {
      triggerSend = true;
    }
    if (sendHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      triggerSend = true;
    }

    if (triggerSend && !m_textInput.IsEmpty()) {
      std::string text = m_textInput.GetText();
      m_textInput.Clear();

      // Registra mensagem do usuário localmente
      long long timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch()).count();
      state->AddChatMessage("user-msg", "user", text, timestamp, "user");

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
  int m_cmdCounter;
  float m_scrollOffset;

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
