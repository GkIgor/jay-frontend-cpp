module;
#include <memory>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
export module jay.features.chat.input_widget;

import jay.engine.types;
import jay.engine.render_context;
import jay.engine.widget;
import jay.shared.widgets.button;
import jay.state.actions;
import jay.features.chat.viewmodel;
import text_editor;
import text_layout;
import unicode;

export namespace jay::features::chat {

// ─────────────────────────────────────────────────────────────────
// ChatInputWidget (Task 36: CHAT-010..CHAT-016)
//
// Integração do TextEditor com suporte a:
//   - Edição UTF-8 e Undo/Redo (Ctrl+Z, Ctrl+Y, Ctrl+Shift+Z)
//   - Navegação por palavras (Ctrl+Left/Right), Home/End, Setas ↑ ↓ (preferredColumn)
//   - Seleção de texto no input (Ctrl+A, Shift+Setas, mouse drag, duplo/triplo clique)
//   - Clipboard em UTF-8 (Ctrl+C, Ctrl+V, Ctrl+X) e deleção por palavra (Ctrl+Backspace/Delete)
//   - Word-wrap e divisão de tokens gigantes
//   - Scrollbar interna no input e clique sob scroll ativo
//   - Desabilitação do botão Enviar e da tecla Enter quando assistente processa
// ─────────────────────────────────────────────────────────────────
class ChatInputWidget : public jay::engine::Widget {
public:
    explicit ChatInputWidget(std::shared_ptr<ChatViewModel> viewModel)
        : m_viewModel(std::move(viewModel)),
          m_inputScrollOffset(0.0f),
          m_lastClickTime(0.0f),
          m_clickCount(0),
          m_isDragging(false),
          m_caretAnimTime(0.0f) {
        tabIndex = 1; // Focável por teclado
    }

    void Init() override {
        m_sendButton = std::make_unique<jay::shared::widgets::Button>("ENVIAR", [this]() {
            SubmitText();
        });
        m_sendButton->Init();
    }

    void Layout(const jay::engine::BoxConstraints& constraints) override {
        m_bounds.width  = constraints.maxW;
        m_bounds.height = (constraints.maxH > 0.0f) ? constraints.maxH : 54.0f;

        float btnW = 90.0f;
        float btnH = m_bounds.height - 12.0f;
        m_sendButton->Layout(jay::engine::BoxConstraints::Tight(btnW, btnH));
        m_sendButton->SetBounds(jay::engine::Rect{
            m_bounds.x + m_bounds.width - btnW - 6.0f,
            m_bounds.y + 6.0f,
            btnW,
            btnH
        });

        // Área de edição útil (descontando o botão de envio)
        m_editAreaW = m_bounds.width - btnW - 24.0f;
        if (m_editAreaW < 100.0f) m_editAreaW = 100.0f;

        m_layoutDirty = false;
    }

    void Update(float deltaTime) override {
        m_caretAnimTime += deltaTime;
        if (m_sendButton) {
            // Desabilita o botão se o assistente estiver processando
            bool isProcessing = IsAssistantProcessing();
            m_sendButton->SetEnabled(!isProcessing);
            m_sendButton->Update(deltaTime);
        }
    }

    void Render(jay::engine::RenderContext& ctx) const override {
        if (!m_visible) return;

        // Fundo e bordas do campo de entrada
        jay::engine::Color bg{22, 27, 34, 255};
        jay::engine::Color border{48, 54, 61, 255};

        ctx.DrawRectRounded(m_bounds, 8.0f, bg);
        ctx.DrawRectLines(m_bounds, 1.0f, border);

        // Constrói a configuração do layout de texto usando a fonte do RenderContext
        jay::TextLayoutConfig config;
        config.font = ctx.NativeFont();
        config.fontSize = 15.0f;
        config.maxWidth = m_editAreaW;
        config.wrapWords = true;

        // Reconstrução de layout no editor (mutação local de render layout)
        const_cast<jay::TextEditor&>(m_editor).RebuildLayout(config);
        const auto& physicalLines = m_editor.GetLayout().GetLines();

        // Área de clivagem do texto
        jay::engine::Rect textClipRect{
            m_bounds.x + 12.0f,
            m_bounds.y + 6.0f,
            m_editAreaW,
            m_bounds.height - 12.0f
        };

        ctx.PushScissor(textClipRect);

        float stepY = 18.0f;
        float startX = m_bounds.x + 12.0f;
        float startY = m_bounds.y + 8.0f - m_inputScrollOffset;

        if (m_editor.GetTextLength() == 0) {
            ctx.DrawText("Digite uma mensagem...", {startX, startY}, 15.0f, jay::engine::Color{139, 148, 158, 255});
        } else {
            // Renderiza destaque de seleção de texto (CHAT-012)
            const auto& sel = m_editor.GetSelection();
            if (sel.IsActive()) {
                size_t selStart = sel.GetStart();
                size_t selEnd = sel.GetEnd();

                size_t charAbsIdx = 0;
                for (size_t l = 0; l < physicalLines.size(); ++l) {
                    float lineY = startY + l * stepY;
                    const auto& lineText = physicalLines[l].text;
                    std::vector<char32_t> cps = jay::unicode::Utf8ToCodepoints(lineText);

                    for (size_t i = 0; i < cps.size(); ++i) {
                        if (charAbsIdx >= selStart && charAbsIdx < selEnd) {
                            std::vector<char32_t> subBefore(cps.begin(), cps.begin() + i);
                            std::vector<char32_t> subChar(cps.begin() + i, cps.begin() + i + 1);

                            float x1 = ctx.MeasureText(jay::unicode::CodepointsToUtf8(subBefore), 15.0f);
                            float charW = ctx.MeasureText(jay::unicode::CodepointsToUtf8(subChar), 15.0f);

                            ctx.DrawRect(
                                jay::engine::Rect{startX + x1, lineY, charW, stepY},
                                jay::engine::Color{31, 111, 235, 120} // Highlight azul
                            );
                        }
                        charAbsIdx++;
                    }
                    if (physicalLines[l].hasNewLine) charAbsIdx++;
                }
            }

            // Renderiza linhas físicas de texto (CHAT-014)
            for (size_t l = 0; l < physicalLines.size(); ++l) {
                float lineY = startY + l * stepY;
                ctx.DrawText(physicalLines[l].text, {startX, lineY}, 15.0f, jay::engine::Color{201, 209, 217, 255});
            }

            // Renderiza o cursor (Caret) piscante (CHAT-010)
            if (std::fmod(m_caretAnimTime, 1.0f) < 0.5f) {
                size_t caretIdx = m_editor.GetCaret().logicalIndex;
                auto [lineIdx, colIdx] = m_editor.GetLayout().IndexToLineCol(caretIdx);

                std::string linePrefix = "";
                if (lineIdx < static_cast<int>(physicalLines.size())) {
                    std::vector<char32_t> cps = jay::unicode::Utf8ToCodepoints(physicalLines[lineIdx].text);
                    if (colIdx <= static_cast<int>(cps.size())) {
                        std::vector<char32_t> sub(cps.begin(), cps.begin() + colIdx);
                        linePrefix = jay::unicode::CodepointsToUtf8(sub);
                    }
                }
                float caretX = startX + ctx.MeasureText(linePrefix, 15.0f);
                float caretY = startY + lineIdx * stepY;

                ctx.DrawRect(
                    jay::engine::Rect{caretX, caretY, 2.0f, stepY},
                    jay::engine::Color{88, 166, 255, 255}
                );
            }
        }

        ctx.PopScissor();

        // Renderiza o botão de envio
        if (m_sendButton) m_sendButton->Render(ctx);
    }

    bool OnEvent(const jay::engine::InputEvent& event) override {
        if (!m_visible || !m_enabled) return false;

        // Delegar ao botão Enviar
        if (m_sendButton && m_sendButton->OnEvent(event)) {
            return true;
        }

        bool ctrl  = event.ctrl;
        bool shift = event.shift;

        // 1. Processa atalhos de teclado (KeyPress)
        if (event.kind == jay::engine::InputEventKind::KeyPress) {
            // Se o assistente estiver processando, bloqueia envio via Enter
            if (event.key == 257) { // KEY_ENTER
                if (!shift && !IsAssistantProcessing()) {
                    SubmitText();
                    event.handled = true;
                    return true;
                } else if (shift) {
                    m_editor.InsertText("\n");
                    MarkLayoutDirty();
                    event.handled = true;
                    return true;
                }
            }

            if (ctrl) {
                if (event.key == 'A' || event.key == 'a') {
                    m_editor.SelectAll();
                    MarkLayoutDirty();
                    return true;
                }
                if (event.key == 'Z' || event.key == 'z') {
                    if (shift) m_editor.Redo();
                    else m_editor.Undo();
                    MarkLayoutDirty();
                    return true;
                }
                if (event.key == 'Y' || event.key == 'y') {
                    m_editor.Redo();
                    MarkLayoutDirty();
                    return true;
                }
                if (event.key == 259) { // KEY_BACKSPACE com Ctrl (Deleção por palavra)
                    m_editor.Delete(jay::Direction::Backward, jay::MoveUnit::Word);
                    MarkLayoutDirty();
                    return true;
                }
                if (event.key == 261) { // KEY_DELETE com Ctrl
                    m_editor.Delete(jay::Direction::Forward, jay::MoveUnit::Word);
                    MarkLayoutDirty();
                    return true;
                }
            }

            // Setas de navegação (Setas, Home, End)
            if (event.key == 263) { // KEY_LEFT
                m_editor.Move(jay::Direction::Backward, ctrl ? jay::MoveUnit::Word : jay::MoveUnit::Character, shift);
                MarkLayoutDirty();
                return true;
            }
            if (event.key == 262) { // KEY_RIGHT
                m_editor.Move(jay::Direction::Forward, ctrl ? jay::MoveUnit::Word : jay::MoveUnit::Character, shift);
                MarkLayoutDirty();
                return true;
            }
            if (event.key == 265) { // KEY_UP
                m_editor.Move(jay::Direction::Up, jay::MoveUnit::Character, shift);
                MarkLayoutDirty();
                return true;
            }
            if (event.key == 264) { // KEY_DOWN
                m_editor.Move(jay::Direction::Down, jay::MoveUnit::Character, shift);
                MarkLayoutDirty();
                return true;
            }
            if (event.key == 268) { // KEY_HOME
                m_editor.Move(jay::Direction::Backward, ctrl ? jay::MoveUnit::Document : jay::MoveUnit::Line, shift);
                MarkLayoutDirty();
                return true;
            }
            if (event.key == 269) { // KEY_END
                m_editor.Move(jay::Direction::Forward, ctrl ? jay::MoveUnit::Document : jay::MoveUnit::Line, shift);
                MarkLayoutDirty();
                return true;
            }
            if (event.key == 259 && !ctrl) { // KEY_BACKSPACE simples
                m_editor.Delete(jay::Direction::Backward, jay::MoveUnit::Character);
                MarkLayoutDirty();
                return true;
            }
            if (event.key == 261 && !ctrl) { // KEY_DELETE simples
                m_editor.Delete(jay::Direction::Forward, jay::MoveUnit::Character);
                MarkLayoutDirty();
                return true;
            }
        }

        // 2. Processa digitação de caracteres Unicode (CharInput)
        if (event.kind == jay::engine::InputEventKind::CharInput) {
            if (event.codepoint >= 32) {
                std::vector<char32_t> cps = {static_cast<char32_t>(event.codepoint)};
                m_editor.InsertText(jay::unicode::CodepointsToUtf8(cps));
                MarkLayoutDirty();
                event.handled = true;
                return true;
            }
        }

        // 3. Processa cliques e seleção por mouse (MousePress / MouseMove)
        if (event.kind == jay::engine::InputEventKind::MousePress && event.key == 0) { // Clique Esquerdo
            if (m_bounds.Contains({event.mouseX, event.mouseY})) {
                size_t clickIdx = GetCharIndexAtPos(event.mouseX, event.mouseY);
                if (shift) {
                    m_editor.MoveCaretTo(clickIdx, true);
                } else {
                    m_editor.SetCaretAndAnchor(clickIdx);
                }
                m_isDragging = true;
                MarkLayoutDirty();
                event.handled = true;
                return true;
            }
        }

        if (event.kind == jay::engine::InputEventKind::MouseMove && m_isDragging) {
            size_t dragIdx = GetCharIndexAtPos(event.mouseX, event.mouseY);
            m_editor.MoveCaretTo(dragIdx, true);
            MarkLayoutDirty();
            return true;
        }

        if (event.kind == jay::engine::InputEventKind::MouseRelease && event.key == 0) {
            m_isDragging = false;
        }

        return false;
    }

private:
    std::shared_ptr<ChatViewModel>                 m_viewModel;
    std::unique_ptr<jay::shared::widgets::Button> m_sendButton;
    jay::TextEditor                               m_editor;

    float m_editAreaW{300.0f};
    float m_inputScrollOffset{0.0f};
    float m_lastClickTime;
    int   m_clickCount;
    bool  m_isDragging;
    float m_caretAnimTime;

    void SubmitText() {
        if (IsAssistantProcessing()) return;
        std::string text = m_editor.GetText();
        if (text.empty() || !m_viewModel) return;

        m_viewModel->SendMessage(text);
        m_editor.Clear();
        m_inputScrollOffset = 0.0f;
        MarkLayoutDirty();
    }

    [[nodiscard]] bool IsAssistantProcessing() const noexcept {
        if (!m_viewModel) return false;
        auto connState = m_viewModel->GetConnectionState();
        return (connState == jay::state::ConnectionState::Connecting ||
                connState == jay::state::ConnectionState::Reconnecting);
    }

    size_t GetCharIndexAtPos(float mouseX, float mouseY) const {
        const auto& physicalLines = m_editor.GetLayout().GetLines();
        if (physicalLines.empty()) return 0;

        float stepY = 18.0f;
        float startX = m_bounds.x + 12.0f;
        float startY = m_bounds.y + 8.0f - m_inputScrollOffset;

        float localY = mouseY - startY;
        int lineIdx = static_cast<int>(localY / stepY);
        if (lineIdx < 0) lineIdx = 0;
        if (lineIdx >= static_cast<int>(physicalLines.size())) lineIdx = static_cast<int>(physicalLines.size()) - 1;

        std::vector<char32_t> lineCps = jay::unicode::Utf8ToCodepoints(physicalLines[lineIdx].text);
        int colIdx = 0;

        // Estima coluna por distância aproximada de caracteres
        float localX = mouseX - startX;
        if (localX < 0.0f) localX = 0.0f;
        float approxCharW = 8.5f;
        colIdx = static_cast<int>(localX / approxCharW);
        if (colIdx > static_cast<int>(lineCps.size())) colIdx = static_cast<int>(lineCps.size());

        return m_editor.GetLayout().LineColToIndex(lineIdx, colIdx);
    }
};

} // namespace jay::features::chat
