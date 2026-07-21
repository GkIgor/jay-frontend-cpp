module;
#include <raylib.h>
#include <string>
#include <vector>
#include <algorithm>
export module text_layout;

export namespace jay {

struct PhysicalLine {
  std::string text;
  bool hasNewLine = false; // true = quebra \n real; false = wrap automático
};

struct TextLayoutConfig {
  Font font;
  float fontSize = 18.0f;
  float maxWidth = 600.0f;
  float lineSpacing = 1.0f;
  bool wrapWords = true;        // false para layout sem quebra automática
  bool preserveWhitespace = false; // futuro: preservar espaços em branco (ex: blocos de código)
};

class TextLayout {
public:
  // Reconstrói as linhas físicas com base no texto e na configuração.
  // Deve ser chamado quando texto ou configuração muda.
  void Build(const std::string& text, const TextLayoutConfig& config) {
    m_lines.clear();

    // Divide pelas quebras lógicas reais (\n)
    std::vector<std::string> logicalLines;
    std::string tmp;
    for (char c : text) {
      if (c == '\n') {
        logicalLines.push_back(tmp);
        tmp.clear();
      } else {
        tmp += c;
      }
    }
    logicalLines.push_back(tmp);

    for (size_t i = 0; i < logicalLines.size(); ++i) {
      const auto& line = logicalLines[i];
      bool isLastLogical = (i == logicalLines.size() - 1);

      if (line.empty()) {
        m_lines.push_back({"", !isLastLogical});
        continue;
      }

      if (!config.wrapWords) {
        // Sem wrap: uma linha física por linha lógica
        m_lines.push_back({line, !isLastLogical});
        continue;
      }

      // Word wrap por palavras usando medição de fonte real
      std::vector<std::string> wrapped;
      std::string currentLine;
      std::string word;

      for (char c : line) {
        if (c == ' ') {
          std::string testLine = currentLine + word + " ";
          if (MeasureTextEx(config.font, testLine.c_str(), config.fontSize, 1.0f).x > config.maxWidth) {
            if (!currentLine.empty()) {
              wrapped.push_back(currentLine);
              currentLine = "";
            }
            // Tenta colocar a palavra na nova linha vazia.
            std::string testWordSpace = word + " ";
            if (MeasureTextEx(config.font, testWordSpace.c_str(), config.fontSize, 1.0f).x > config.maxWidth) {
              if (MeasureTextEx(config.font, word.c_str(), config.fontSize, 1.0f).x > config.maxWidth) {
                // Token excessivo! Quebra segura por codepoints Unicode.
                auto parts = WrapOversizedToken(word, config.font, config.fontSize, config.maxWidth);
                for (size_t p = 0; p < parts.size() - 1; ++p) {
                  wrapped.push_back(parts[p]);
                }
                currentLine = parts.back() + " ";
              } else {
                wrapped.push_back(word);
                currentLine = " ";
              }
            } else {
              currentLine = testWordSpace;
            }
          } else {
            currentLine = testLine;
          }
          word.clear();
        } else {
          word += c;
        }
      }

      if (!word.empty() || !currentLine.empty()) {
        std::string testLine = currentLine + word;
        if (MeasureTextEx(config.font, testLine.c_str(), config.fontSize, 1.0f).x > config.maxWidth) {
          if (!currentLine.empty()) {
            wrapped.push_back(currentLine);
            currentLine = "";
          }
          if (MeasureTextEx(config.font, word.c_str(), config.fontSize, 1.0f).x > config.maxWidth) {
            auto parts = WrapOversizedToken(word, config.font, config.fontSize, config.maxWidth);
            for (size_t p = 0; p < parts.size(); ++p) {
              wrapped.push_back(parts[p]);
            }
          } else {
            wrapped.push_back(word);
          }
        } else {
          wrapped.push_back(testLine);
        }
      }

      if (wrapped.empty()) {
        wrapped.push_back("");
      }

      for (size_t j = 0; j < wrapped.size(); ++j) {
        bool isLastPhysicalOfLogical = (j == wrapped.size() - 1);
        bool hasNL = isLastPhysicalOfLogical && !isLastLogical;
        m_lines.push_back({wrapped[j], hasNL});
      }
    }
  }

  // Linhas físicas resultantes
  const std::vector<PhysicalLine>& GetLines() const { return m_lines; }

  // Número total de linhas físicas
  int GetLineCount() const { return (int)m_lines.size(); }

  // Altura total de conteúdo em pixels
  float GetContentHeight(float stepY) const {
    return m_lines.empty() ? stepY : m_lines.size() * stepY;
  }

  // Índice lógico → (linha física, coluna)
  std::pair<int, int> IndexToLineCol(size_t logicalIndex) const {
    int remaining = (int)logicalIndex;
    int cursorLine = 0;
    int cursorCol = 0;
    for (size_t l = 0; l < m_lines.size(); ++l) {
      int len = (int)CountCodepoints(m_lines[l].text);
      if (remaining <= len) {
        cursorLine = (int)l;
        cursorCol = remaining;
        return {cursorLine, cursorCol};
      }
      remaining -= (len + (m_lines[l].hasNewLine ? 1 : 0));
      cursorLine = (int)l;
      cursorCol = len;
    }
    return {cursorLine, cursorCol};
  }

  // (linha física, coluna) → índice lógico
  size_t LineColToIndex(int line, int col) const {
    size_t absIdx = 0;
    int targetLine = std::clamp(line, 0, std::max(0, (int)m_lines.size() - 1));
    for (int l = 0; l < targetLine; ++l) {
      absIdx += CountCodepoints(m_lines[l].text) + (m_lines[l].hasNewLine ? 1 : 0);
    }
    absIdx += std::min((size_t)col, CountCodepoints(m_lines[targetLine].text));
    return absIdx;
  }

private:
  std::vector<PhysicalLine> m_lines;

  // Divide um token contínuo e excessivo (como URLs, caminhos,hashes) em partes que caibam no maxWidth
  // Usamos quebra segura baseada em codepoints Unicode.
  // Otimização futura: A busca do ponto de quebra poderá utilizar busca binária sobre
  // o token, reduzindo chamadas repetidas a MeasureTextEx().
  static std::vector<std::string> WrapOversizedToken(const std::string& token, Font font, float fontSize, float maxWidth) {
    std::vector<std::string> parts;
    std::string currentPart = "";

    for (size_t i = 0; i < token.length(); ) {
      size_t charLen = 1;
      unsigned char c = token[i];
      if (c <= 0x7F) { charLen = 1; }
      else if ((c & 0xE0) == 0xC0) { charLen = 2; }
      else if ((c & 0xF0) == 0xE0) { charLen = 3; }
      else if ((c & 0xF8) == 0xF0) { charLen = 4; }

      std::string nextChar = token.substr(i, charLen);
      std::string testPart = currentPart + nextChar;

      if (MeasureTextEx(font, testPart.c_str(), fontSize, 1.0f).x > maxWidth) {
        if (!currentPart.empty()) {
          parts.push_back(currentPart);
          currentPart = nextChar;
        } else {
          // Se um único codepoint Unicode exceder a largura máxima sozinho,
          // forçamos sua inclusão para evitar loop infinito
          parts.push_back(nextChar);
          currentPart = "";
        }
      } else {
        currentPart = testPart;
      }
      i += charLen;
    }

    if (!currentPart.empty()) {
      parts.push_back(currentPart);
    }
    return parts;
  }

  // Conta codepoints UTF-8 de uma string (sem depender do módulo unicode para evitar dependência circular)
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
