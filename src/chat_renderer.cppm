module;
#include <raylib.h>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <cmath>
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
  ChatRenderer() : m_cmdCounter(1), m_scrollOffset(0.0f), m_selectionBubbleIdx(-1), m_selectionStartChar(-1), m_selectionEndChar(-1), m_copiedBubbleIdx(-1), m_copiedTimer(0.0f) {}

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
    float fontSize = 18.0f;
    float labelFontSize = 12.0f;
    int maxBubbleWidth = 640;
    int spacingBetweenBubbles = 20;

    struct RenderBubble {
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

    std::vector<RenderBubble> renderList;
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

      // Deixa espaço extra abaixo do balão para o botão "Copiar"
      currentY += bubbleHeight + 22 + spacingBetweenBubbles;
    }

    // Feedback visual se estiver esperando por resposta (Thinking ou Executing)
    bool isThinkingOrExecuting = (state->GetState() == State::Thinking || state->GetState() == State::Executing);
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

    // Calcula o scroll automático para manter no fundo se não houver rolagem manual
    int totalFeedHeight = currentY - (tabHeight + 25);
    float minScroll = 0.0f;
    if (totalFeedHeight > chatAreaHeight - 50) {
      minScroll = (float)(chatAreaHeight - 50 - totalFeedHeight);
    }
    if (m_scrollOffset > 0.0f) m_scrollOffset = 0.0f;
    if (m_scrollOffset < minScroll) m_scrollOffset = minScroll;

    if (wheel == 0 && !IsKeyDown(KEY_UP) && !IsKeyDown(KEY_DOWN)) {
      m_scrollOffset = minScroll;
    }

    // Gerencia cliques de mouse e drag para seleção e cópia de texto
    Vector2 mousePos = GetMousePosition();
    bool isMouseDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    bool isMousePressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    if (m_copiedTimer > 0.0f) {
      m_copiedTimer -= GetFrameTime();
    }

    if (isMousePressed) {
      m_selectionBubbleIdx = -1;
      m_selectionStartChar = -1;
      m_selectionEndChar = -1;

      for (const auto& bubble : renderList) {
        if (bubble.isLoading) continue;

        Rectangle scrolledRect = bubble.rect;
        scrolledRect.y += m_scrollOffset;

        // Bounding box do botão de copiar
        Rectangle copyBtnRect;
        if (bubble.isUser) {
          copyBtnRect = { scrolledRect.x + scrolledRect.width - 80, scrolledRect.y + scrolledRect.height + 4, 80.0f, 16.0f };
        } else {
          copyBtnRect = { scrolledRect.x, scrolledRect.y + scrolledRect.height + 4, 80.0f, 16.0f };
        }

        if (CheckCollisionPointRec(mousePos, copyBtnRect)) {
          SetClipboardText(bubble.originalText.c_str());
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
      // Atualiza o fim da seleção
      for (const auto& bubble : renderList) {
        if (bubble.originalIndex == m_selectionBubbleIdx) {
          Rectangle scrolledRect = bubble.rect;
          scrolledRect.y += m_scrollOffset;
          m_selectionEndChar = GetCharIndexAtMouse(font, bubble.originalText, bubble.lines, scrolledRect, fontSize, mousePos);
          break;
        }
      }
    }

    // Atalho de cópia da seleção Ctrl+C
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
              if (l + 1 < bubble.lines.size()) dispText += " ";
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

    // 3. Renderiza a área de chat recortando conteúdos fora do limite (Scissor)
    BeginScissorMode(0, tabHeight, screenWidth, chatAreaHeight);

    for (const auto& bubble : renderList) {
      Rectangle scrolledRect = bubble.rect;
      scrolledRect.y += m_scrollOffset;

      if (scrolledRect.y + scrolledRect.height > tabHeight && scrolledRect.y < screenHeight - inputHeight) {
        if (bubble.isLoading) {
          // Balão animado de resposta/carregamento
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
          // Balão de mensagem comum
          DrawRectangleRounded(scrolledRect, 0.2f, 4, bubble.bubbleColor);
          Vector2 labelPos = {scrolledRect.x + 10, scrolledRect.y - 15};
          DrawTextEx(font, bubble.label.c_str(), labelPos, labelFontSize, 1.0f, Theme::TextSec);

          // Destaque de seleção (Ctrl+A / mouse drag) caractere por caractere
          if (m_selectionBubbleIdx == bubble.originalIndex && m_selectionStartChar != m_selectionEndChar) {
            int startChar = std::min(m_selectionStartChar, m_selectionEndChar);
            int endChar = std::max(m_selectionStartChar, m_selectionEndChar);

            int charAbsIdx = 0;
            for (size_t l = 0; l < bubble.lines.size(); ++l) {
              const auto& line = bubble.lines[l];
              int lineY = scrolledRect.y + 10 + l * 24;

              for (size_t i = 0; i < line.length(); ++i) {
                if (charAbsIdx >= startChar && charAbsIdx < endChar) {
                  float x1 = MeasureTextEx(font, line.substr(0, i).c_str(), fontSize, 1.0f).x;
                  float x2 = MeasureTextEx(font, line.substr(0, i + 1).c_str(), fontSize, 1.0f).x;
                  Rectangle hlRect = { scrolledRect.x + 16 + x1, (float)lineY, x2 - x1, 24.0f };
                  DrawRectangleRec(hlRect, GetColor(0x1F6FEB88)); // Fundo de destaque azul/cinza translúcido
                }
                charAbsIdx++;
              }
              charAbsIdx++; // Pula o espaço implícito entre linhas
            }
          }

          // Texto interno
          int lineY = scrolledRect.y + 10;
          for (const auto& line : bubble.lines) {
            Vector2 textPos = {scrolledRect.x + 16, (float)lineY};
            DrawTextEx(font, line.c_str(), textPos, fontSize, 1.0f, bubble.textColor);
            lineY += 24;
          }

          // Botão de copiar abaixo do balão
          Rectangle copyBtnRect;
          if (bubble.isUser) {
            copyBtnRect = { scrolledRect.x + scrolledRect.width - 80, scrolledRect.y + scrolledRect.height + 4, 80.0f, 16.0f };
          } else {
            copyBtnRect = { scrolledRect.x, scrolledRect.y + scrolledRect.height + 4, 80.0f, 16.0f };
          }

          bool isCopyHovered = CheckCollisionPointRec(mousePos, copyBtnRect);
          std::string copyLabel = "Copiar";
          Color btnColor = isCopyHovered ? Theme::Glow : Theme::TextSec;

          if (m_copiedBubbleIdx == bubble.originalIndex && m_copiedTimer > 0.0f) {
            copyLabel = "Copiado!";
            btnColor = Theme::Executing; // Verde visual de sucesso
          }

          Vector2 labelDim = MeasureTextEx(font, copyLabel.c_str(), 11.0f, 1.0f);
          Vector2 btnTextPos;
          if (bubble.isUser) {
            btnTextPos = { copyBtnRect.x + copyBtnRect.width - labelDim.x, copyBtnRect.y };
          } else {
            btnTextPos = { copyBtnRect.x, copyBtnRect.y };
          }

          DrawTextEx(font, copyLabel.c_str(), btnTextPos, 11.0f, 1.0f, btnColor);
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
    m_textInput.Update(m_selectionBubbleIdx != -1 && m_selectionStartChar != m_selectionEndChar);

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

    // Cursor piscante com suporte a quebra de linha (Ctrl+Enter)
    if (((int)(GetTime() * 2) % 2) == 0) {
      int cursorX = 14;
      int cursorY = 10;
      size_t lastNL = currentText.find_last_of('\n');
      if (lastNL == std::string::npos) {
        Vector2 textDim = MeasureTextEx(font, currentText.c_str(), inputTextSize, 1.0f);
        cursorX += (int)textDim.x;
      } else {
        std::string lastLine = currentText.substr(lastNL + 1);
        Vector2 textDim = MeasureTextEx(font, lastLine.c_str(), inputTextSize, 1.0f);
        cursorX += (int)textDim.x;

        int numLines = 1;
        for (char c : currentText) if (c == '\n') numLines++;
        cursorY += (numLines - 1) * 22; // 22px de avanço por linha
      }
      DrawRectangle(inputField.x + cursorX, inputField.y + cursorY, 2, 22, Theme::Glow);
    }

    // Botão Enviar
    Rectangle sendBtn = {(float)(screenWidth - 150), (float)(iy + 19), 110.0f, 42.0f};
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
      if (ctrlPressed) {
        m_textInput.AppendNewline();
      } else {
        triggerSend = true;
      }
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

  // Variáveis para seleção e cópia
  int m_selectionBubbleIdx;
  int m_selectionStartChar;
  int m_selectionEndChar;

  int m_copiedBubbleIdx;
  float m_copiedTimer;

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

    int absIdx = 0;
    for (int l = 0; l < lineIdx; ++l) {
      absIdx += (int)wrappedLines[l].length() + 1;
    }
    return absIdx + colIdx;
  }

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
