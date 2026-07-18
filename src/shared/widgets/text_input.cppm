module;
#include <raylib.h>
#include <string>
export module text_input;

export namespace jay {

class TextInput {
public:
  TextInput() : m_isSelectedAll(false) {}

  void Update(bool blockShortcuts = false) {
    // 1. Lê os atalhos de teclado de controle primeiro
    bool ctrlPressed = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    
    if (ctrlPressed && !blockShortcuts) {
      if (IsKeyPressed(KEY_A)) {
        m_isSelectedAll = true;
      }
      if (IsKeyPressed(KEY_C)) {
        SetClipboardText(m_text.c_str());
      }
      if (IsKeyPressed(KEY_X)) {
        SetClipboardText(m_text.c_str());
        m_text.clear();
        m_isSelectedAll = false;
      }
      if (IsKeyPressed(KEY_V)) {
        const char* clipboard = GetClipboardText();
        if (clipboard) {
          if (m_isSelectedAll) {
            m_text.clear();
            m_isSelectedAll = false;
          }
          // Filtra para copiar no máximo até limite
          for (int i = 0; clipboard[i] != '\0' && m_text.length() < 256; ++i) {
            m_text += clipboard[i];
          }
        }
      }
    } else {
      // 2. Lê os caracteres Unicode normais
      int codepoint = GetCharPressed();
      if (codepoint > 0 && m_isSelectedAll) {
        m_text.clear();
        m_isSelectedAll = false;
      }
      while (codepoint > 0) {
        if (m_text.length() < 256) {
          AppendUtf8(m_text, codepoint);
        }
        codepoint = GetCharPressed();
      }

      // 3. Trata Backspace (apagamento seguro UTF-8)
      if (IsKeyPressed(KEY_BACKSPACE)) {
        if (m_isSelectedAll) {
          m_text.clear();
          m_isSelectedAll = false;
        } else {
          PopUtf8(m_text);
        }
      }
    }
  }

  std::string GetText() const { return m_text; }
  void Clear() { m_text.clear(); }
  bool IsEmpty() const { return m_text.empty(); }
  bool IsSelectedAll() const { return m_isSelectedAll; }
  void AppendNewline() {
    if (m_text.length() < 256) {
      m_text += "\n";
    }
  }

private:
  std::string m_text;
  bool m_isSelectedAll;

  // Converte codepoint Unicode para sequência de bytes UTF-8 e anexa à string
  void AppendUtf8(std::string& str, int codepoint) {
    if (codepoint <= 0x7F) {
      str += (char)codepoint;
    } else if (codepoint <= 0x7FF) {
      str += (char)(0xC0 | ((codepoint >> 6) & 0x1F));
      str += (char)(0x80 | (codepoint & 0x3F));
    } else if (codepoint <= 0xFFFF) {
      str += (char)(0xE0 | ((codepoint >> 12) & 0x0F));
      str += (char)(0x80 | ((codepoint >> 6) & 0x3F));
      str += (char)(0x80 | (codepoint & 0x3F));
    } else if (codepoint <= 0x10FFFF) {
      str += (char)(0xF0 | ((codepoint >> 18) & 0x07));
      str += (char)(0x80 | ((codepoint >> 12) & 0x3F));
      str += (char)(0x80 | ((codepoint >> 6) & 0x3F));
      str += (char)(0x80 | (codepoint & 0x3F));
    }
  }

  // Remove exatamente um caractere UTF-8 (multibyte) do final da string
  void PopUtf8(std::string& str) {
    if (str.empty()) return;
    while (!str.empty()) {
      unsigned char c = str.back();
      str.pop_back();
      if ((c & 0xC0) != 0x80) {
        break;
      }
    }
  }
};

} // namespace jay
