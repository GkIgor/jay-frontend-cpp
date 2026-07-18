module;
#include <raylib.h>
#include <string>
#include <vector>
#include <algorithm>
export module chat_input;

import theme;
import text_input;

export namespace jay {

class ChatInput {
public:
  static void Draw(Font font, TextInput& txtInput, int screenWidth, int screenHeight, int iy, int dynamicInputHeight, int inputLines, int visibleInputLines, bool blockShortcuts, Vector2 mousePos, bool& triggerSend, bool ctrlPressed, bool isSendBlocked, float inputScrollOffset) {
    DrawRectangle(0, iy, screenWidth, dynamicInputHeight, Theme::Panel);
    DrawLine(0, iy, screenWidth, iy, Theme::Border);

    const float stepY = 18.0f; // Altura exata da linha para a fonte 18.0f
    float inputFieldHeight = 42.0f + (visibleInputLines - 1) * stepY;
    
    // Subtrai uma pequena largura à caixa de texto se a barra de rolagem for exibida
    float scrollBarWidth = (inputLines > 10) ? 12.0f : 0.0f;
    Rectangle inputField = {40.0f, (float)(iy + 19), (float)(screenWidth - 220), inputFieldHeight};
    DrawRectangleRec(inputField, Theme::Background);
    DrawRectangleLinesEx(inputField, 1.0f, Theme::Border);

    txtInput.Update(blockShortcuts);

    std::string currentText = txtInput.GetText();
    float inputTextSize = 18.0f;

    // Divide o texto em linhas usando as quebras reais '\n'
    std::vector<std::string> linesVec;
    std::string tmp = "";
    for (char c : currentText) {
      if (c == '\n') {
        linesVec.push_back(tmp);
        tmp = "";
      } else {
        tmp += c;
      }
    }
    linesVec.push_back(tmp);

    BeginScissorMode((int)inputField.x + 2, (int)inputField.y + 2, (int)inputField.width - 4 - (int)scrollBarWidth, (int)inputField.height - 4);

    // Desenha destaques Ctrl+A linha por linha para manter 100% de consistência
    if (txtInput.IsSelectedAll() && !currentText.empty()) {
      for (size_t l = 0; l < linesVec.size(); ++l) {
        Vector2 lineDim = MeasureTextEx(font, linesVec[l].c_str(), inputTextSize, 1.0f);
        Rectangle selectionRect = {
          inputField.x + 10,
          inputField.y + 9 + l * stepY - inputScrollOffset,
          lineDim.x + 4,
          stepY + 2
        };
        DrawRectangleRec(selectionRect, GetColor(0x1F6FEB88));
      }
    }

    // Desenha o texto digitado linha por linha
    for (size_t l = 0; l < linesVec.size(); ++l) {
      Vector2 linePos = {inputField.x + 12, inputField.y + 12 + l * stepY - inputScrollOffset};
      DrawTextEx(font, linesVec[l].c_str(), linePos, inputTextSize, 1.0f, Theme::TextMain);
    }

    // Cursor piscante posicionado na linha correta sem acumular erros
    if (((int)(GetTime() * 2) % 2) == 0) {
      int cursorX = 14;
      int cursorY = 10;
      std::string lastLine = linesVec.back();
      Vector2 textDim = MeasureTextEx(font, lastLine.c_str(), inputTextSize, 1.0f);
      cursorX += (int)textDim.x;
      cursorY += (linesVec.size() - 1) * stepY;

      DrawRectangle(inputField.x + cursorX, inputField.y + cursorY - inputScrollOffset, 2, 22, Theme::Glow);
    }

    EndScissorMode();

    // Desenha barra de rolagem (mini Scrollbar) dentro da caixa de texto
    if (inputLines > 10) {
      Rectangle track = { inputField.x + inputField.width - 10.0f, inputField.y + 4.0f, 4.0f, inputField.height - 8.0f };
      DrawRectangleRounded(track, 1.0f, 4, GetColor(0x1F293733));

      float thumbHeight = (10.0f / inputLines) * track.height;
      if (thumbHeight < 15.0f) thumbHeight = 15.0f;

      float maxScroll = (inputLines - 10) * stepY;
      float scrollPercent = (maxScroll != 0.0f) ? (inputScrollOffset / maxScroll) : 0.0f;
      float thumbY = track.y + scrollPercent * (track.height - thumbHeight);

      Rectangle thumb = { track.x, thumbY, track.width, thumbHeight };
      DrawRectangleRounded(thumb, 1.0f, 4, Theme::Glow);
    }

    // Botão Enviar
    Rectangle sendBtn = {(float)(screenWidth - 150), (float)(iy + dynamicInputHeight - 61), 110.0f, 42.0f};
    bool sendHovered = CheckCollisionPointRec(mousePos, sendBtn) && !isSendBlocked;

    Color btnColor;
    Color btnTextColor;
    if (isSendBlocked) {
      btnColor = Theme::Border;
      btnTextColor = Theme::TextSec;
    } else {
      btnColor = sendHovered ? Theme::Glow : Theme::UserBubble;
      btnTextColor = WHITE;
    }

    DrawRectangleRounded(sendBtn, 0.2f, 4, btnColor);
    
    float sendBtnFontSize = 14.0f;
    Vector2 sendTextDim = MeasureTextEx(font, "ENVIAR", sendBtnFontSize, 1.0f);
    Vector2 sendTextPos = {
      sendBtn.x + (sendBtn.width / 2) - (sendTextDim.x / 2),
      sendBtn.y + (sendBtn.height / 2) - (sendTextDim.y / 2)
    };
    DrawTextEx(font, "ENVIAR", sendTextPos, sendBtnFontSize, 1.0f, btnTextColor);

    // Bloqueia envios se m_state indica processamento ativo
    if (IsKeyPressed(KEY_ENTER)) {
      if (ctrlPressed) {
        txtInput.AppendNewline();
      } else if (!isSendBlocked) {
        triggerSend = true;
      }
    }
    if (sendHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      triggerSend = true;
    }
  }
};

} // namespace jay
