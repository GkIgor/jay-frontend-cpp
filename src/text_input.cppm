module;
#include <raylib.h>
#include <string>
export module text_input;

export namespace jay {

class TextInput {
public:
  TextInput() = default;

  void Update() {
    // 1. Lê os caracteres Unicode inseridos no frame
    int codepoint = GetCharPressed();
    while (codepoint > 0) {
      // Limite básico de tamanho para não estourar a tela
      if (m_text.length() < 256) {
        AppendUtf8(m_text, codepoint);
      }
      codepoint = GetCharPressed();
    }

    // 2. Trata Backspace (apagamento seguro UTF-8)
    if (IsKeyPressed(KEY_BACKSPACE)) {
      PopUtf8(m_text);
    }
  }

  std::string GetText() const { return m_text; }
  void Clear() { m_text.clear(); }
  bool IsEmpty() const { return m_text.empty(); }

private:
  std::string m_text;

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
      // Em UTF-8, bytes de continuação começam com bits 10xxxxxx (0x80 a 0xBF).
      // Se o byte não for de continuação, removemos o cabeçalho do caractere e paramos.
      if ((c & 0xC0) != 0x80) {
        break;
      }
    }
  }
};

} // namespace jay
