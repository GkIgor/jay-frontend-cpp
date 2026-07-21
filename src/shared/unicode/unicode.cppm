module;
#include <string>
#include <vector>
export module unicode;

export namespace jay::unicode {

inline bool IsWordBoundary(char32_t cp) {
  return cp == U' '  || cp == U'\n' || cp == U'\t' || cp == U'\r' ||
         cp == U'.'  || cp == U','  || cp == U';'  || cp == U':'  ||
         cp == U'('  || cp == U')'  || cp == U'['  || cp == U']'  ||
         cp == U'{'  || cp == U'}'  || cp == U'<'  || cp == U'>'  ||
         cp == U'"'  || cp == U'\'' || cp == U'`'  || cp == U'\\' ||
         cp == U'/'  || cp == U'-'  || cp == U'_'  || cp == U'+'  ||
         cp == U'='  || cp == U'*'  || cp == U'&'  || cp == U'^'  ||
         cp == U'%'  || cp == U'$'  || cp == U'#'  || cp == U'@'  ||
         cp == U'!'  || cp == U'?'  || cp == U'|';
}

std::vector<char32_t> Utf8ToCodepoints(const std::string& str) {
  std::vector<char32_t> codepoints;
  for (size_t i = 0; i < str.length(); ) {
    unsigned char c = str[i];
    char32_t codepoint = 0;
    int extraBytes = 0;
    if (c <= 0x7F) {
      codepoint = c;
      extraBytes = 0;
    } else if ((c & 0xE0) == 0xC0) {
      codepoint = c & 0x1F;
      extraBytes = 1;
    } else if ((c & 0xF0) == 0xE0) {
      codepoint = c & 0x0F;
      extraBytes = 2;
    } else if ((c & 0xF8) == 0xF0) {
      codepoint = c & 0x07;
      extraBytes = 3;
    } else {
      i++;
      continue;
    }

    if (i + extraBytes >= str.length()) {
      break;
    }
    bool valid = true;
    for (int j = 0; j < extraBytes; ++j) {
      unsigned char next = str[i + 1 + j];
      if ((next & 0xC0) != 0x80) {
        valid = false;
        break;
      }
      codepoint = (codepoint << 6) | (next & 0x3F);
    }
    if (valid) {
      codepoints.push_back(codepoint);
      i += 1 + extraBytes;
    } else {
      i++;
    }
  }
  return codepoints;
}

std::string CodepointsToUtf8(const std::vector<char32_t>& codepoints) {
  std::string str;
  for (char32_t cp : codepoints) {
    if (cp <= 0x7F) {
      str += (char)cp;
    } else if (cp <= 0x7FF) {
      str += (char)(0xC0 | ((cp >> 6) & 0x1F));
      str += (char)(0x80 | (cp & 0x3F));
    } else if (cp <= 0xFFFF) {
      str += (char)(0xE0 | ((cp >> 12) & 0x0F));
      str += (char)(0x80 | ((cp >> 6) & 0x3F));
      str += (char)(0x80 | (cp & 0x3F));
    } else if (cp <= 0x10FFFF) {
      str += (char)(0xF0 | ((cp >> 18) & 0x07));
      str += (char)(0x80 | ((cp >> 12) & 0x3F));
      str += (char)(0x80 | ((cp >> 6) & 0x3F));
      str += (char)(0x80 | (cp & 0x3F));
    }
  }
  return str;
}

} // namespace jay::unicode
