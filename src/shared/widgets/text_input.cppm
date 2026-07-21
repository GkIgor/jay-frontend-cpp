module;
#include <raylib.h>
#include <string>
#include <vector>
#include <algorithm>
export module text_input;

import unicode;
import text_editor;
import text_layout;

export namespace jay {

class TextInput {
public:
  TextInput()
      : m_backspaceTimer(0.0f), m_deleteTimer(0.0f),
        m_lastClickTime(0.0f), m_clickCount(0), m_isDragging(false) {}

  // Reconstrói o layout de texto via TextEditor. Deve ser chamado pelo consumidor
  // antes de Update/UpdateMouseSelection no mesmo frame.
  void RebuildLayout(const TextLayoutConfig& config) {
    m_editor.RebuildLayout(config);
  }

  const TextLayout& GetLayout() const {
    return m_editor.GetLayout();
  }

  void Update(bool blockShortcuts) {
    bool ctrlPressed = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    bool shiftPressed = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

    // 1. Trata atalhos com Ctrl pressionado
    if (ctrlPressed && !blockShortcuts) {
      if (IsKeyPressed(KEY_A)) {
        m_editor.SelectAll();
      }

      if (IsKeyPressed(KEY_C)) {
        std::string text = m_editor.Copy();
        if (!text.empty()) {
          SetClipboardText(text.c_str());
        }
      }

      if (IsKeyPressed(KEY_X)) {
        std::string text = m_editor.Cut();
        if (!text.empty()) {
          SetClipboardText(text.c_str());
        }
      }

      if (IsKeyPressed(KEY_Z)) {
        if (shiftPressed) {
          m_editor.Redo();
        } else {
          m_editor.Undo();
        }
      }

      if (IsKeyPressed(KEY_Y)) {
        m_editor.Redo();
      }

      if (IsKeyPressed(KEY_V)) {
        const char* clipboard = GetClipboardText();
        if (clipboard) {
          m_editor.Paste(clipboard);
        }
      }

      // Deleção por palavra via atalho pressionado uma vez
      if (IsKeyPressed(KEY_BACKSPACE)) {
        m_editor.Delete(Direction::Backward, MoveUnit::Word);
      }
      if (IsKeyPressed(KEY_DELETE)) {
        m_editor.Delete(Direction::Forward, MoveUnit::Word);
      }
    }

    // 2. Trata navegação por teclado (Setas, Home, End, PageUp, PageDown)
    if (!blockShortcuts) {
      if (IsKeyPressed(KEY_LEFT)) {
        m_editor.Move(Direction::Backward, ctrlPressed ? MoveUnit::Word : MoveUnit::Character, shiftPressed);
      }
      if (IsKeyPressed(KEY_RIGHT)) {
        m_editor.Move(Direction::Forward, ctrlPressed ? MoveUnit::Word : MoveUnit::Character, shiftPressed);
      }
      if (IsKeyPressed(KEY_UP)) {
        m_editor.Move(Direction::Up, MoveUnit::Character, shiftPressed);
      }
      if (IsKeyPressed(KEY_DOWN)) {
        m_editor.Move(Direction::Down, MoveUnit::Character, shiftPressed);
      }
      if (IsKeyPressed(KEY_HOME)) {
        m_editor.Move(Direction::Backward, ctrlPressed ? MoveUnit::Document : MoveUnit::Line, shiftPressed);
      }
      if (IsKeyPressed(KEY_END)) {
        m_editor.Move(Direction::Forward, ctrlPressed ? MoveUnit::Document : MoveUnit::Line, shiftPressed);
      }
      if (IsKeyPressed(KEY_PAGE_UP)) {
        m_editor.Move(Direction::Backward, MoveUnit::Document, shiftPressed);
      }
      if (IsKeyPressed(KEY_PAGE_DOWN)) {
        m_editor.Move(Direction::Forward, MoveUnit::Document, shiftPressed);
      }
    }

    // 3. Lê caracteres Unicode ordinários de digitação
    int codepoint = GetCharPressed();
    if (codepoint > 0) {
      std::vector<char32_t> inputQueue;
      while (codepoint > 0) {
        inputQueue.push_back((char32_t)codepoint);
        codepoint = GetCharPressed();
      }
      if (!inputQueue.empty()) {
        m_editor.InsertText(unicode::CodepointsToUtf8(inputQueue));
      }
    }

    // 4. Trata Backspace contínuo sob temporizador (se Ctrl não estiver ativo)
    if (!ctrlPressed) {
      if (IsKeyDown(KEY_BACKSPACE)) {
        if (IsKeyPressed(KEY_BACKSPACE)) {
          m_editor.Delete(Direction::Backward, MoveUnit::Character);
          m_backspaceTimer = 0.0f;
        } else {
          m_backspaceTimer += GetFrameTime();
          if (m_backspaceTimer >= m_backspaceDelay) {
            float elapsed = m_backspaceTimer - m_backspaceDelay;
            int steps = (int)(elapsed / m_backspaceInterval);
            if (steps > 0) {
              for (int s = 0; s < steps; ++s) {
                m_editor.Delete(Direction::Backward, MoveUnit::Character);
              }
              m_backspaceTimer = m_backspaceDelay + (elapsed - steps * m_backspaceInterval);
            }
          }
        }
      } else {
        m_backspaceTimer = 0.0f;
      }

      // Trata Delete contínuo sob temporizador
      if (IsKeyDown(KEY_DELETE)) {
        if (IsKeyPressed(KEY_DELETE)) {
          m_editor.Delete(Direction::Forward, MoveUnit::Character);
          m_deleteTimer = 0.0f;
        } else {
          m_deleteTimer += GetFrameTime();
          if (m_deleteTimer >= m_backspaceDelay) {
            float elapsed = m_deleteTimer - m_backspaceDelay;
            int steps = (int)(elapsed / m_backspaceInterval);
            if (steps > 0) {
              for (int s = 0; s < steps; ++s) {
                m_editor.Delete(Direction::Forward, MoveUnit::Character);
              }
              m_deleteTimer = m_backspaceDelay + (elapsed - steps * m_backspaceInterval);
            }
          }
        }
      } else {
        m_deleteTimer = 0.0f;
      }
    } else {
      // Repetição para deleção por palavra via Ctrl
      if (IsKeyDown(KEY_BACKSPACE)) {
        m_backspaceTimer += GetFrameTime();
        if (m_backspaceTimer >= m_backspaceDelay) {
          float elapsed = m_backspaceTimer - m_backspaceDelay;
          int steps = (int)(elapsed / m_backspaceInterval);
          if (steps > 0) {
            for (int s = 0; s < steps; ++s) {
              m_editor.Delete(Direction::Backward, MoveUnit::Word);
            }
            m_backspaceTimer = m_backspaceDelay + (elapsed - steps * m_backspaceInterval);
          }
        }
      } else {
        m_backspaceTimer = 0.0f;
      }

      if (IsKeyDown(KEY_DELETE)) {
        m_deleteTimer += GetFrameTime();
        if (m_deleteTimer >= m_backspaceDelay) {
          float elapsed = m_deleteTimer - m_backspaceDelay;
          int steps = (int)(elapsed / m_backspaceInterval);
          if (steps > 0) {
            for (int s = 0; s < steps; ++s) {
              m_editor.Delete(Direction::Forward, MoveUnit::Word);
            }
            m_deleteTimer = m_backspaceDelay + (elapsed - steps * m_backspaceInterval);
          }
        }
      } else {
        m_deleteTimer = 0.0f;
      }
    }
  }

  void UpdateMouseSelection(Font font, float fontSize, Vector2 mousePos, Rectangle bounds, float inputScrollOffset = 0.0f) {
    const auto& physicalLines = m_editor.GetLayout().GetLines();
    bool shiftPressed = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    bool isInside = CheckCollisionPointRec(mousePos, bounds);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      if (isInside) {
        float time = (float)GetTime();
        if (time - m_lastClickTime < 0.3f) {
          m_clickCount++;
        } else {
          m_clickCount = 1;
        }
        m_lastClickTime = time;

        size_t absIdx = GetCharIndexAtMouse(font, fontSize, physicalLines, mousePos, bounds, inputScrollOffset);

        if (m_clickCount == 1) {
          if (shiftPressed) {
            m_editor.MoveCaretTo(absIdx, true);
          } else {
            m_editor.SetCaretAndAnchor(absIdx);
          }
          m_isDragging = true;
        } else if (m_clickCount == 2) {
          m_editor.SelectWordAt(absIdx);
          m_isDragging = false;
        } else if (m_clickCount >= 3) {
          // Triplo clique: seleciona a linha física clicada inteira
          auto [lineIdx, colIdx] = m_editor.GetLayout().IndexToLineCol(absIdx);
          size_t lineStart = m_editor.GetLayout().LineColToIndex(lineIdx, 0);
          // Conta codepoints da linha para obter lineEnd
          size_t lineLen = CountCodepoints(physicalLines[lineIdx].text);
          size_t lineEnd = m_editor.GetLayout().LineColToIndex(lineIdx, (int)lineLen);
          m_editor.SelectLineAt(lineStart, lineEnd);
          m_isDragging = false;
        }
      }
    }

    if (m_isDragging) {
      if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        size_t currentIdx = GetCharIndexAtMouse(font, fontSize, physicalLines, mousePos, bounds, inputScrollOffset);
        m_editor.MoveCaretTo(currentIdx, true);
      } else {
        m_isDragging = false;
      }
    }
  }

  std::string GetText() const { return m_editor.GetText(); }
  void SetText(const std::string& text) { m_editor.SetText(text); }
  void Clear() { m_editor.Clear(); }
  bool IsEmpty() const { return m_editor.GetTextLength() == 0; }
  const TextEditor& GetEditor() const { return m_editor; }

  void AppendNewline() {
    m_editor.InsertText("\n");
  }

private:
  TextEditor m_editor;

  // Temporizadores
  float m_backspaceTimer;
  float m_deleteTimer;
  const float m_backspaceDelay = 0.5f;
  const float m_backspaceInterval = 0.05f;

  // Controle de Mouse
  float m_lastClickTime;
  int m_clickCount;
  bool m_isDragging;

  size_t GetCharIndexAtMouse(Font font, float fontSize, const std::vector<PhysicalLine>& physicalLines, Vector2 mousePos, Rectangle bounds, float inputScrollOffset) {
    if (physicalLines.empty()) return 0;

    float stepY = 18.0f;
    float localY = mousePos.y - (bounds.y + 12.0f) + inputScrollOffset;
    int clickedLine = (int)(localY / stepY);
    if (clickedLine < 0) clickedLine = 0;
    if (clickedLine >= (int)physicalLines.size()) clickedLine = (int)physicalLines.size() - 1;

    float localX = mousePos.x - (bounds.x + 12.0f);
    std::vector<char32_t> lineCps = unicode::Utf8ToCodepoints(physicalLines[clickedLine].text);

    float prevX = 0.0f;
    int charCol = 0;
    for (size_t i = 1; i <= lineCps.size(); ++i) {
      std::vector<char32_t> slice(lineCps.begin(), lineCps.begin() + i);
      std::string prefix = unicode::CodepointsToUtf8(slice);
      float w = MeasureTextEx(font, prefix.c_str(), fontSize, 1.0f).x;
      if (localX < prevX + (w - prevX) / 2.0f) {
        charCol = (int)i - 1;
        break;
      }
      prevX = w;
      charCol = (int)i;
    }

    return m_editor.GetLayout().LineColToIndex(clickedLine, charCol);
  }

  static size_t CountCodepoints(const std::string& str) {
    size_t count = 0;
    for (size_t i = 0; i < str.length(); ) {
      unsigned char c = str[i];
      if (c <= 0x7F) { i += 1; }
      else if ((c & 0xE0) == 0xC0) { i += 2; }
      else if ((c & 0xF0) == 0xE0) { i += 3; }
      else if ((c & 0xF8) == 0xF0) { i += 4; }
      else { i += 1; }
      count++;
    }
    return count;
  }
};

} // namespace jay
