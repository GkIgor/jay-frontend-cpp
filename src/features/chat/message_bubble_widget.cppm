module;
#include <string>
#include <string_view>
#include <cmath>
export module jay.features.chat.message_bubble_widget;

import jay.engine.types;
import jay.engine.render_context;
import jay.engine.widget;
import jay.state.actions;

export namespace jay::features::chat {

class MessageBubbleWidget : public jay::engine::Widget {
public:
    explicit MessageBubbleWidget(jay::state::MessageDTO message)
        : m_message(std::move(message)) {}

    void SetMessage(jay::state::MessageDTO message) {
        m_message = std::move(message);
        MarkLayoutDirty();
    }

    [[nodiscard]] const jay::state::MessageDTO& GetMessage() const noexcept {
        return m_message;
    }

    void Layout(const jay::engine::BoxConstraints& constraints) override {
        // Largura máxima da bolha: 70% do espaço disponível
        float maxBubbleW = constraints.maxW * 0.7f;
        if (maxBubbleW < 100.0f) maxBubbleW = constraints.maxW;

        // Estima a altura com base no tamanho do texto
        float fontSize = 15.0f;
        float paddingH = 16.0f;
        float paddingV = 12.0f;

        // Por padrão no Layout, assumimos altura estimada de linha
        float textWidthEst = static_cast<float>(m_message.content.length()) * 8.5f;
        float lines = std::ceil(textWidthEst / (maxBubbleW - paddingH * 2.0f));
        if (lines < 1.0f) lines = 1.0f;

        m_bounds.width  = (textWidthEst < maxBubbleW - paddingH * 2.0f) ? (textWidthEst + paddingH * 2.0f) : maxBubbleW;
        m_bounds.height = lines * (fontSize + 4.0f) + paddingV * 2.0f;

        // Alinhamento horizontal: Usuário à direita, Assistente/Erro à esquerda
        if (m_message.sender == "user") {
            m_bounds.x = constraints.maxW - m_bounds.width;
        } else {
            m_bounds.x = 0.0f;
        }

        m_layoutDirty = false;
    }

    void Render(jay::engine::RenderContext& ctx) const override {
        if (!m_visible || m_message.content.empty()) return;

        // Cores semânticas conforme o remetente
        jay::engine::Color bg;
        jay::engine::Color border;
        jay::engine::Color textCol{201, 209, 217, 255};

        if (m_message.sender == "user") {
            bg     = jay::engine::Color{31, 111, 235, 255};   // Azul UserBubble
            border = jay::engine::Color{56, 139, 253, 255};
        } else if (m_message.status == jay::state::MessageStatus::Failed) {
            bg     = jay::engine::Color{218, 54, 51, 60};     // Vermelho Erro
            border = jay::engine::Color{218, 54, 51, 255};
        } else {
            bg     = jay::engine::Color{33, 38, 45, 255};     // Cinza JayBubble
            border = jay::engine::Color{48, 54, 61, 255};
        }

        // Desenha a bolha arredondada
        ctx.DrawRectRounded(m_bounds, 12.0f, bg);
        ctx.DrawRectLines(m_bounds, 1.0f, border);

        // Desenha o conteúdo textual da mensagem
        float fontSize = 15.0f;
        float paddingH = 16.0f;
        float paddingV = 12.0f;

        ctx.DrawText(
            m_message.content,
            {m_bounds.x + paddingH, m_bounds.y + paddingV},
            fontSize,
            textCol
        );
    }

private:
    jay::state::MessageDTO m_message;
};

} // namespace jay::features::chat
