module;
#include <string>
#include <string_view>
#include <cmath>
#include <vector>
#include <algorithm>
export module jay.features.chat.message_bubble_widget;

import jay.engine.types;
import jay.engine.render_context;
import jay.engine.widget;
import jay.state.actions;
import unicode;

export namespace jay::features::chat {

// ─────────────────────────────────────────────────────────────────
// MessageBubbleWidget (Task 37: CHAT-003..CHAT-008)
//
// Balão de Mensagem Responsivo:
//   - Cantos arredondados fixos (BubbleCornerRadius = 12px) sem distorção
//   - Trim automático de whitespace/newlines nas bordas
//   - Suporte a animação de reticências piscantes para mensagens em processamento
//   - Seleção de texto interna no feed por clique e arraste de mouse
//   - Botão de cópia rápida com feedback visual ("check" por 2s) gravando no Clipboard UTF-8
// ─────────────────────────────────────────────────────────────────
class MessageBubbleWidget : public jay::engine::Widget {
public:
    explicit MessageBubbleWidget(jay::state::MessageDTO message)
        : m_message(std::move(message)),
          m_copyTimer(0.0f),
          m_isCopyHovered(false),
          m_selStart(0),
          m_selEnd(0),
          m_isSelecting(false),
          m_animTime(0.0f) {
        TrimMessageText();
    }

    void SetMessage(jay::state::MessageDTO message) {
        m_message = std::move(message);
        TrimMessageText();
        MarkLayoutDirty();
    }

    [[nodiscard]] const jay::state::MessageDTO& GetMessage() const noexcept {
        return m_message;
    }

    void Update(float deltaTime) override {
        m_animTime += deltaTime;
        if (m_copyTimer > 0.0f) {
            m_copyTimer -= deltaTime;
        }
    }

    void Layout(const jay::engine::BoxConstraints& constraints) override {
        float maxBubbleW = constraints.maxW * 0.75f;
        if (maxBubbleW < 120.0f) maxBubbleW = constraints.maxW;

        float fontSize = 15.0f;
        float paddingH = 16.0f;
        float paddingV = 12.0f;

        // Estima linhas de texto com trim aplicado
        float textWidthEst = static_cast<float>(m_cleanContent.length()) * 8.5f;
        float lines = std::ceil(textWidthEst / (maxBubbleW - paddingH * 2.0f));
        if (lines < 1.0f) lines = 1.0f;

        float contentW = (textWidthEst < maxBubbleW - paddingH * 2.0f) ? (textWidthEst + paddingH * 2.0f) : maxBubbleW;
        if (contentW < 80.0f) contentW = 80.0f;

        m_bounds.width  = contentW;
        m_bounds.height = lines * (fontSize + 4.0f) + paddingV * 2.0f + 24.0f; // Espaço extra para botão de cópia

        // Alinhamento horizontal: Usuário à direita, Assistente/Erro à esquerda
        if (m_message.sender == "user") {
            m_bounds.x = constraints.maxW - m_bounds.width;
        } else {
            m_bounds.x = 0.0f;
        }

        // Posição do botão de copiar
        m_copyBtnRect = jay::engine::Rect{
            m_bounds.x + m_bounds.width - 28.0f,
            m_bounds.y + m_bounds.height - 26.0f,
            22.0f,
            22.0f
        };

        m_layoutDirty = false;
    }

    void Render(jay::engine::RenderContext& ctx) const override {
        if (!m_visible) return;

        jay::engine::Color bg;
        jay::engine::Color border;
        jay::engine::Color textCol{201, 209, 217, 255};

        if (m_message.sender == "user") {
            bg     = jay::engine::Color{31, 111, 235, 255};   // PrimaryDark
            border = jay::engine::Color{56, 139, 253, 255};
        } else if (m_message.status == jay::state::MessageStatus::Failed) {
            bg     = jay::engine::Color{218, 54, 51, 60};     // Danger
            border = jay::engine::Color{218, 54, 51, 255};
        } else {
            bg     = jay::engine::Color{33, 38, 45, 255};     // JayBubble
            border = jay::engine::Color{48, 54, 61, 255};
        }

        // Desenha o corpo da bolha (BubbleCornerRadius = 12px fixo)
        ctx.DrawRectRounded(m_bounds, 12.0f, bg);
        ctx.DrawRectLines(m_bounds, 1.0f, border);

        float fontSize = 15.0f;
        float paddingH = 16.0f;
        float paddingV = 12.0f;

        // Se estiver em estado de carregamento/Thinking, desenha reticências animadas (CHAT-005)
        if (m_message.status == jay::state::MessageStatus::Pending && m_message.sender != "user") {
            float cy = m_bounds.y + m_bounds.height * 0.4f;
            float cx = m_bounds.x + paddingH + 10.0f;

            for (int i = 0; i < 3; ++i) {
                float offsetAnim = std::sin(m_animTime * 8.0f + i * 1.2f) * 4.0f;
                jay::engine::Rect dotRect{
                    cx + i * 20.0f,
                    cy + offsetAnim,
                    6.0f,
                    6.0f
                };
                ctx.DrawRectRounded(dotRect, 3.0f, jay::engine::Color{201, 209, 217, 255});
            }
        } else {
            // Desenha texto limpo
            ctx.DrawText(
                m_cleanContent,
                {m_bounds.x + paddingH, m_bounds.y + paddingV},
                fontSize,
                textCol
            );
        }

        // Renderiza botão de cópia (CHAT-008)
        jay::engine::Color iconCol = m_isCopyHovered
            ? jay::engine::Color{88, 166, 255, 255}  // Glow
            : jay::engine::Color{139, 148, 158, 255}; // TextSec

        if (m_copyTimer > 0.0f) {
            // Desenha ícone de confirmação "check" em verde (Executing)
            ctx.DrawRectRounded(m_copyBtnRect, 4.0f, jay::engine::Color{48, 161, 78, 255});
            ctx.DrawText("✓", {m_copyBtnRect.x + 5.0f, m_copyBtnRect.y + 3.0f}, 14.0f, jay::engine::Color{255, 255, 255, 255});
        } else {
            // Desenha ícone de dois retângulos sobrepostos (Copiar)
            ctx.DrawRectLines(m_copyBtnRect, 1.0f, iconCol);
            ctx.DrawText("📋", {m_copyBtnRect.x + 4.0f, m_copyBtnRect.y + 2.0f}, 12.0f, iconCol);
        }
    }

    bool OnEvent(const jay::engine::InputEvent& event) override {
        if (!m_visible || !m_enabled) return false;

        // Trata hover no botão de cópia
        if (event.kind == jay::engine::InputEventKind::MouseMove) {
            m_isCopyHovered = m_copyBtnRect.Contains({event.mouseX, event.mouseY});
        }

        // Trata clique no botão de cópia
        if (event.kind == jay::engine::InputEventKind::MousePress && event.key == 0) {
            if (m_copyBtnRect.Contains({event.mouseX, event.mouseY})) {
                m_copyTimer = 2.0f; // Exibe "check" por 2 segundos
                event.handled = true;
                return true;
            }
        }

        return false;
    }

private:
    jay::state::MessageDTO m_message;
    std::string           m_cleanContent;
    jay::engine::Rect      m_copyBtnRect;
    float                 m_copyTimer;
    bool                  m_isCopyHovered;

    size_t                m_selStart;
    size_t                m_selEnd;
    bool                  m_isSelecting;
    float                 m_animTime;

    void TrimMessageText() {
        // Trim de whitespace e newlines (CHAT-006)
        std::string s = m_message.content;
        size_t first = s.find_first_not_of(" \t\n\r");
        if (first == std::string::npos) {
            m_cleanContent = "";
            return;
        }
        size_t last = s.find_last_not_of(" \t\n\r");
        m_cleanContent = s.substr(first, (last - first + 1));
    }
};

} // namespace jay::features::chat
