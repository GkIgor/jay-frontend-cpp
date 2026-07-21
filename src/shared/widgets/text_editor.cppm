module;
#include <string>
#include <vector>
#include <algorithm>
export module text_editor;

import unicode;
import text_layout;

export namespace jay {

// Comentário Arquitetural (Grapheme Clusters):
// Atualmente, a manipulação de texto do TextEditor é indexada a nível de codepoints Unicode (char32_t).
// Caso versões futuras necessitem de suporte completo a Grapheme Clusters (ex: ZWJ emoji sequences como 👨‍👩‍👧‍👦,
// acentos combinantes ou ligaduras complexas), a arquitetura do motor de edição está estruturada para
// substituir os índices lógicos por intervalos de grapheme clusters sem alterar o contrato público do TextInput ou TextEditor.

// Comentário Arquitetural (BubbleLayout):
// Futuramente, o layout das bolhas de chat (CachedBubbleLayout em chat_renderer.cppm) poderá reutilizar
// o TextLayout quando ambos suportarem a mesma configuração de layout (stepY, fonte, maxWidth).
// Por enquanto, a unificação está adiada pois as bolhas usam BubbleStepY=24px e o input usa StepY=18px.

enum class Direction { Forward, Backward, Up, Down };
enum class MoveUnit { Character, Word, Line, Document };

struct Caret {
  size_t logicalIndex = 0;
  int preferredColumn = -1; // Mantém a coluna visual preferida durante navegações verticais
};

struct Selection {
  size_t anchor = 0;
  size_t caret = 0;

  bool IsActive() const { return anchor != caret; }
  size_t GetStart() const { return std::min(anchor, caret); }
  size_t GetEnd() const { return std::max(anchor, caret); }
  void Clear(size_t pos) { anchor = pos; caret = pos; }
};

class TextBuffer {
public:
  std::vector<char32_t> codepoints;

  size_t GetSize() const { return codepoints.size(); }
  bool IsEmpty() const { return codepoints.empty(); }
  void Clear() { codepoints.clear(); }

  void Insert(size_t pos, char32_t cp) {
    if (pos <= codepoints.size()) {
      codepoints.insert(codepoints.begin() + pos, cp);
    }
  }

  void Insert(size_t pos, const std::vector<char32_t>& cps) {
    if (pos <= codepoints.size()) {
      codepoints.insert(codepoints.begin() + pos, cps.begin(), cps.end());
    }
  }

  void Erase(size_t start, size_t end) {
    size_t s = std::min(start, end);
    size_t e = std::max(start, end);
    if (s < codepoints.size()) {
      e = std::min(e, codepoints.size());
      codepoints.erase(codepoints.begin() + s, codepoints.begin() + e);
    }
  }

  void Replace(size_t start, size_t end, const std::vector<char32_t>& cps) {
    Erase(start, end);
    Insert(std::min(start, end), cps);
  }
};

struct EditorSnapshot {
  std::vector<char32_t> textState;
  size_t caretPos;
  size_t anchorPos;
};

class TextEditor {
public:
  TextEditor() : m_lastAction(LastAction::None) {}

  void SetText(const std::string& text) {
    m_buffer.codepoints = unicode::Utf8ToCodepoints(text);
    m_caret.logicalIndex = m_buffer.GetSize();
    m_caret.preferredColumn = -1;
    m_selection.Clear(m_caret.logicalIndex);
    m_undoStack.clear();
    m_redoStack.clear();
    m_lastAction = LastAction::None;
  }

  std::string GetText() const {
    return unicode::CodepointsToUtf8(m_buffer.codepoints);
  }

  size_t GetTextLength() const {
    return m_buffer.GetSize();
  }

  void Clear() {
    SaveUndoState(LastAction::Deleting);
    m_buffer.Clear();
    m_caret.logicalIndex = 0;
    m_caret.preferredColumn = -1;
    m_selection.Clear(0);
  }

  const Caret& GetCaret() const { return m_caret; }
  const Selection& GetSelection() const { return m_selection; }

  // --- Layout integrado ---

  void RebuildLayout(const TextLayoutConfig& config) {
    m_layout.Build(GetText(), config);
  }

  const TextLayout& GetLayout() const { return m_layout; }

  // --- Navegação do Cursor e Seleções ---

  void SetCaretAndAnchor(size_t index) {
    index = std::min(index, m_buffer.GetSize());
    m_caret.logicalIndex = index;
    m_caret.preferredColumn = -1;
    m_selection.Clear(index);
  }

  void MoveCaretTo(size_t index, bool select) {
    index = std::min(index, m_buffer.GetSize());
    m_caret.logicalIndex = index;
    m_caret.preferredColumn = -1;
    if (select) {
      m_selection.caret = index;
    } else {
      m_selection.Clear(index);
    }
  }

  void SelectWordAt(size_t index) {
    size_t len = m_buffer.GetSize();
    if (len == 0) return;
    index = std::min(index, len - 1);

    size_t start = index;
    while (start > 0 && !unicode::IsWordBoundary(m_buffer.codepoints[start - 1])) {
      start--;
    }

    size_t end = index;
    while (end < len && !unicode::IsWordBoundary(m_buffer.codepoints[end])) {
      end++;
    }

    m_selection.anchor = start;
    m_selection.caret = end;
    m_caret.logicalIndex = end;
    m_caret.preferredColumn = -1;
  }

  void SelectLineAt(size_t lineStart, size_t lineEnd) {
    m_selection.anchor = std::min(lineStart, m_buffer.GetSize());
    m_selection.caret = std::min(lineEnd, m_buffer.GetSize());
    m_caret.logicalIndex = m_selection.caret;
    m_caret.preferredColumn = -1;
  }

  void SelectAll() {
    m_selection.anchor = 0;
    m_selection.caret = m_buffer.GetSize();
    m_caret.logicalIndex = m_selection.caret;
    m_caret.preferredColumn = -1;
  }

  // Move usa o layout interno (m_layout) — não recebe physicalLines
  void Move(Direction dir, MoveUnit unit, bool select) {
    const auto& physicalLines = m_layout.GetLines();
    size_t prevPos = m_caret.logicalIndex;
    size_t newPos = prevPos;
    size_t len = m_buffer.GetSize();

    if (dir == Direction::Backward) {
      if (unit == MoveUnit::Character) {
        newPos = prevPos > 0 ? prevPos - 1 : 0;
        m_caret.preferredColumn = -1;
      } else if (unit == MoveUnit::Word) {
        newPos = FindWordBoundaryLeft(prevPos);
        m_caret.preferredColumn = -1;
      } else if (unit == MoveUnit::Line) {
        newPos = FindLineBoundaryLeft(prevPos, physicalLines);
        m_caret.preferredColumn = -1;
      } else if (unit == MoveUnit::Document) {
        newPos = 0;
        m_caret.preferredColumn = -1;
      }
    } else if (dir == Direction::Forward) {
      if (unit == MoveUnit::Character) {
        newPos = prevPos < len ? prevPos + 1 : len;
        m_caret.preferredColumn = -1;
      } else if (unit == MoveUnit::Word) {
        newPos = FindWordBoundaryRight(prevPos);
        m_caret.preferredColumn = -1;
      } else if (unit == MoveUnit::Line) {
        newPos = FindLineBoundaryRight(prevPos, physicalLines);
        m_caret.preferredColumn = -1;
      } else if (unit == MoveUnit::Document) {
        newPos = len;
        m_caret.preferredColumn = -1;
      }
    } else if (dir == Direction::Up || dir == Direction::Down) {
      newPos = NavigateUpDown(dir == Direction::Up, physicalLines);
    }

    m_caret.logicalIndex = newPos;
    if (select) {
      m_selection.caret = newPos;
    } else {
      m_selection.Clear(newPos);
    }
  }

  // --- Operações de Deleção e Edição ---

  void Delete(Direction dir, MoveUnit unit) {
    if (m_selection.IsActive()) {
      DeleteSelection();
      return;
    }

    size_t len = m_buffer.GetSize();
    size_t start = m_caret.logicalIndex;
    size_t target = start;

    if (dir == Direction::Backward) {
      if (start == 0) return;
      SaveUndoState(LastAction::Deleting);
      if (unit == MoveUnit::Character) {
        target = start - 1;
      } else if (unit == MoveUnit::Word) {
        target = FindWordBoundaryLeft(start);
      }
      m_buffer.Erase(target, start);
      SetCaretAndAnchor(target);
    } else if (dir == Direction::Forward) {
      if (start == len) return;
      SaveUndoState(LastAction::Deleting);
      if (unit == MoveUnit::Character) {
        target = start + 1;
      } else if (unit == MoveUnit::Word) {
        target = FindWordBoundaryRight(start);
      }
      m_buffer.Erase(start, target);
      SetCaretAndAnchor(start);
    }
  }

  void InsertText(const std::string& utf8Text) {
    if (m_selection.IsActive()) {
      DeleteSelection();
    }

    std::vector<char32_t> newCps = unicode::Utf8ToCodepoints(utf8Text);
    if (newCps.empty()) return;

    SaveUndoState(utf8Text.length() > 1 ? LastAction::Pasting : LastAction::Typing);
    
    // Agrupamento semântico: se for espaço ou nova linha, força a quebra de agrupamento posterior
    bool isWhitespace = (newCps.size() == 1 && (newCps[0] == U' ' || newCps[0] == U'\n'));

    m_buffer.Insert(m_caret.logicalIndex, newCps);
    size_t newPos = m_caret.logicalIndex + newCps.size();
    SetCaretAndAnchor(newPos);

    if (isWhitespace) {
      m_lastAction = LastAction::None;
    }
  }

  void DeleteSelection() {
    if (!m_selection.IsActive()) return;
    SaveUndoState(LastAction::Deleting);

    size_t start = m_selection.GetStart();
    size_t end = m_selection.GetEnd();
    m_buffer.Erase(start, end);
    SetCaretAndAnchor(start);
  }

  // --- Operações de Clipboard Centralizadas ---

  std::string Copy() {
    if (!m_selection.IsActive()) return "";
    size_t s = m_selection.GetStart();
    size_t e = m_selection.GetEnd();
    std::vector<char32_t> slice(m_buffer.codepoints.begin() + s, m_buffer.codepoints.begin() + e);
    return unicode::CodepointsToUtf8(slice);
  }

  std::string Cut() {
    if (!m_selection.IsActive()) return "";
    std::string text = Copy();
    DeleteSelection();
    return text;
  }

  void Paste(const std::string& utf8Text) {
    InsertText(utf8Text);
  }

  // --- Desfazer e Refazer (Undo/Redo) ---

  void Undo() {
    if (!m_undoStack.empty()) {
      m_redoStack.push_back({m_buffer.codepoints, m_caret.logicalIndex, m_selection.anchor});
      auto snapshot = m_undoStack.back();
      m_undoStack.pop_back();

      m_buffer.codepoints = snapshot.textState;
      m_caret.logicalIndex = snapshot.caretPos;
      m_caret.preferredColumn = -1;
      m_selection.anchor = snapshot.anchorPos;
      m_selection.caret = snapshot.caretPos;
      m_lastAction = LastAction::None;
    }
  }

  void Redo() {
    if (!m_redoStack.empty()) {
      m_undoStack.push_back({m_buffer.codepoints, m_caret.logicalIndex, m_selection.anchor});
      auto snapshot = m_redoStack.back();
      m_redoStack.pop_back();

      m_buffer.codepoints = snapshot.textState;
      m_caret.logicalIndex = snapshot.caretPos;
      m_caret.preferredColumn = -1;
      m_selection.anchor = snapshot.anchorPos;
      m_selection.caret = snapshot.caretPos;
      m_lastAction = LastAction::None;
    }
  }

private:
  enum class LastAction { None, Typing, Deleting, Pasting };

  TextBuffer m_buffer;
  TextLayout m_layout;
  Caret m_caret;
  Selection m_selection;

  LastAction m_lastAction;
  std::vector<EditorSnapshot> m_undoStack;
  std::vector<EditorSnapshot> m_redoStack;

  void SaveUndoState(LastAction action) {
    if (m_lastAction != action || action == LastAction::Pasting) {
      if (m_undoStack.empty() || m_undoStack.back().textState != m_buffer.codepoints) {
        m_undoStack.push_back({m_buffer.codepoints, m_caret.logicalIndex, m_selection.anchor});
        if (m_undoStack.size() > 100) {
          m_undoStack.erase(m_undoStack.begin());
        }
      }
      m_redoStack.clear();
      m_lastAction = action;
    }
  }

  size_t FindWordBoundaryLeft(size_t start) {
    if (start == 0) return 0;
    size_t i = start - 1;
    while (i > 0 && unicode::IsWordBoundary(m_buffer.codepoints[i])) {
      i--;
    }
    while (i > 0 && !unicode::IsWordBoundary(m_buffer.codepoints[i - 1])) {
      i--;
    }
    return i;
  }

  size_t FindWordBoundaryRight(size_t start) {
    size_t len = m_buffer.GetSize();
    if (start >= len) return len;
    size_t i = start;
    while (i < len && unicode::IsWordBoundary(m_buffer.codepoints[i])) {
      i++;
    }
    while (i < len && !unicode::IsWordBoundary(m_buffer.codepoints[i])) {
      i++;
    }
    return i;
  }

  size_t FindLineBoundaryLeft(size_t start, const std::vector<PhysicalLine>& physicalLines) {
    if (physicalLines.empty()) return 0;
    auto [lineIdx, colIdx] = m_layout.IndexToLineCol(start);
    return m_layout.LineColToIndex(lineIdx, 0);
  }

  size_t FindLineBoundaryRight(size_t start, const std::vector<PhysicalLine>& physicalLines) {
    if (physicalLines.empty()) return m_buffer.GetSize();
    auto [lineIdx, colIdx] = m_layout.IndexToLineCol(start);
    // Fim da linha = col máximo
    size_t lineLen = 0;
    for (size_t i = 0; i < physicalLines[lineIdx].text.length(); ) {
      unsigned char c = physicalLines[lineIdx].text[i];
      if (c <= 0x7F) { i += 1; }
      else if ((c & 0xE0) == 0xC0) { i += 2; }
      else if ((c & 0xF0) == 0xE0) { i += 3; }
      else if ((c & 0xF8) == 0xF0) { i += 4; }
      else { i += 1; }
      lineLen++;
    }
    return m_layout.LineColToIndex(lineIdx, (int)lineLen);
  }

  size_t NavigateUpDown(bool isUp, const std::vector<PhysicalLine>& physicalLines) {
    if (physicalLines.empty()) return m_caret.logicalIndex;
    auto [cursorLine, cursorCol] = m_layout.IndexToLineCol(m_caret.logicalIndex);

    if (m_caret.preferredColumn == -1) {
      m_caret.preferredColumn = cursorCol;
    }

    int targetLine = isUp ? cursorLine - 1 : cursorLine + 1;
    if (targetLine < 0) {
      return 0;
    }
    if (targetLine >= (int)physicalLines.size()) {
      return m_buffer.GetSize();
    }

    // Conta codepoints da linha alvo
    size_t lenTarget = 0;
    for (size_t i = 0; i < physicalLines[targetLine].text.length(); ) {
      unsigned char c = physicalLines[targetLine].text[i];
      if (c <= 0x7F) { i += 1; }
      else if ((c & 0xE0) == 0xC0) { i += 2; }
      else if ((c & 0xF0) == 0xE0) { i += 3; }
      else if ((c & 0xF8) == 0xF0) { i += 4; }
      else { i += 1; }
      lenTarget++;
    }
    int newCol = std::min(m_caret.preferredColumn, (int)lenTarget);
    return m_layout.LineColToIndex(targetLine, newCol);
  }
};

} // namespace jay
