module;
#include <raylib.h>
#include <string>
#include <vector>
#include <algorithm>
export module chat_input;

import theme;
import text_input;
import text_editor;
import text_layout;
import unicode;
import scrollbar;

export namespace jay {

class ChatInput {
public:
  static void Draw(Font font, TextInput& txtInput, int screenWidth, int screenHeight, int iy,
                   int dynamicInputHeight, int inputLines, int visibleInputLines,
                   bool blockShortcuts, Vector2 mousePos, bool& triggerSend,
                   bool ctrlPressed, bool isSendBlocked, float inputScrollOffset) {
    DrawRectangle(0, iy, screenWidth, dynamicInputHeight, Theme::Panel);
    DrawLine(0, iy, screenWidth, iy, Theme::Border);

    const float stepY = Theme::StepY;
    float inputFieldHeight = 42.0f + (visibleInputLines - 1) * stepY;

    Scrollbar scrollbar(ScrollbarStyle{});
    float scrollBarWidth = (inputLines > 10) ? (scrollbar.GetWidth() + 8.0f) : 0.0f;
    Rectangle inputField = {40.0f, (float)(iy + 19), (float)(screenWidth - 220), inputFieldHeight};
    DrawRectangleRec(inputField, Theme::Background);
    DrawRectangleLinesEx(inputField, 1.0f, Theme::Border);

    float inputTextSize = 18.0f;
    float maxWidth = inputField.width - 24.0f - scrollBarWidth;

    // 1. Reconstrói o layout de texto via TextEditor (única fonte de verdade)
    TextLayoutConfig layoutConfig;
    layoutConfig.font = font;
    layoutConfig.fontSize = inputTextSize;
    layoutConfig.maxWidth = maxWidth;
    txtInput.RebuildLayout(layoutConfig);

    // 2. Processa cliques de mouse e arrasto
    txtInput.UpdateMouseSelection(font, inputTextSize, mousePos, inputField, inputScrollOffset);

    // 3. Processa atalhos e digitação do teclado
    txtInput.Update(blockShortcuts);

    // 4. Regenera o layout caso o texto tenha mudado no ciclo atual do frame
    txtInput.RebuildLayout(layoutConfig);

    const auto& physicalLines = txtInput.GetLayout().GetLines();
    const TextEditor& editor = txtInput.GetEditor();
    const Selection& sel = editor.GetSelection();

    BeginScissorMode((int)inputField.x + 2, (int)inputField.y + 2,
                     (int)inputField.width - 4 - (int)scrollBarWidth, (int)inputField.height - 4);

    // 5. Desenha destaque de seleção multi-linha dinâmica se ativa
    if (sel.IsActive()) {
      size_t selStart = sel.GetStart();
      size_t selEnd = sel.GetEnd();
      size_t absCounter = 0;

      for (size_t l = 0; l < physicalLines.size(); ++l) {
        std::vector<char32_t> lineCps = unicode::Utf8ToCodepoints(physicalLines[l].text);
        size_t lineStartAbs = absCounter;
        size_t lineEndAbs = lineStartAbs + lineCps.size();

        size_t interStart = std::max(lineStartAbs, selStart);
        size_t interEnd = std::min(lineEndAbs, selEnd);

        if (interStart < interEnd) {
          int colStart = (int)(interStart - lineStartAbs);
          int colEnd = (int)(interEnd - lineStartAbs);

          std::vector<char32_t> prefixSlice(lineCps.begin(), lineCps.begin() + colStart);
          float xStart = MeasureTextEx(font, unicode::CodepointsToUtf8(prefixSlice).c_str(),
                                       inputTextSize, 1.0f).x;

          std::vector<char32_t> selectSlice(lineCps.begin() + colStart, lineCps.begin() + colEnd);
          float widthSel = MeasureTextEx(font, unicode::CodepointsToUtf8(selectSlice).c_str(),
                                        inputTextSize, 1.0f).x;

          Rectangle selectionRect = {
            inputField.x + 12 + xStart,
            inputField.y + 9 + l * stepY - inputScrollOffset,
            widthSel,
            stepY + 2
          };
          DrawRectangleRec(selectionRect, GetColor(0x1F6FEB88));
        }
        absCounter += lineCps.size() + (physicalLines[l].hasNewLine ? 1 : 0);
      }
    }

    // 6. Desenha o texto digitado linha por linha
    for (size_t l = 0; l < physicalLines.size(); ++l) {
      Vector2 linePos = {inputField.x + 12, inputField.y + 12 + l * stepY - inputScrollOffset};
      DrawTextEx(font, physicalLines[l].text.c_str(), linePos, inputTextSize, 1.0f, Theme::TextMain);
    }

    // 7. Desenha o cursor piscante posicionado de forma dinâmica
    if (((int)(GetTime() * 2) % 2) == 0) {
      size_t caretIdx = editor.GetCaret().logicalIndex;
      auto [cursorLine, cursorCol] = txtInput.GetLayout().IndexToLineCol(caretIdx);

      std::vector<char32_t> lineCps = unicode::Utf8ToCodepoints(physicalLines[cursorLine].text);
      std::vector<char32_t> prefixSlice(lineCps.begin(), lineCps.begin() + cursorCol);
      float prefixWidth = MeasureTextEx(font, unicode::CodepointsToUtf8(prefixSlice).c_str(),
                                       inputTextSize, 1.0f).x;

      int cursorX = 12 + (int)prefixWidth;
      int cursorY = 10 + cursorLine * stepY;

      DrawRectangle(inputField.x + cursorX, inputField.y + cursorY - inputScrollOffset,
                    2, 22, Theme::Glow);
    }

    EndScissorMode();

    // 8. Desenha scrollbar reutilizável
    if (inputLines > 10) {
      float contentHeight = physicalLines.size() * stepY;
      float visibleHeight = inputField.height - 8.0f;
      scrollbar.Draw(inputField, inputScrollOffset, contentHeight, visibleHeight);
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
