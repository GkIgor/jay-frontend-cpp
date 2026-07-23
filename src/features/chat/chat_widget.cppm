module;
#include <memory>
#include <vector>
#include <algorithm>
export module jay.features.chat.widget;

import jay.engine.types;
import jay.engine.render_context;
import jay.engine.widget;
import jay.features.chat.viewmodel;
import jay.features.chat.message_bubble_widget;
import scroll_controller;
import scrollbar;

export namespace jay::features::chat {

// ─────────────────────────────────────────────────────────────────
// ChatWidget (Task 39: CHAT-001, CHAT-002)
//
// Feed de Mensagens com Rolagem Inteligente:
//   - Scroll Controller com sensibilidade WheelSensChat (140px)
//   - Scrollbar (8px) com arraste do thumb por mouse (Slide Drag)
//   - Suporte a rolagem por teclado (PageUp / PageDown)
//   - Auto-scroll inteligente: rola para a base se mensagem for do usuário;
//     preserva a posição do histórico se a IA responder e o usuário estiver
//     rolando acima da margem de 50px.
// ─────────────────────────────────────────────────────────────────
class ChatWidget : public jay::engine::ContainerWidget {
public:
    explicit ChatWidget(std::shared_ptr<ChatViewModel> viewModel)
        : m_viewModel(std::move(viewModel)),
          m_scrollController(140.0f),
          m_contentHeight(0.0f),
          m_wasAtBottom(true) {
        if (m_viewModel) {
            m_viewModel->SetOnUpdateCallback([this]() { SyncMessagesWithState(); });
        }
    }

    void Init() override {
        ContainerWidget::Init();
        SyncMessagesWithState();
    }

    void Layout(const jay::engine::BoxConstraints& constraints) override {
        m_bounds.width  = constraints.maxW;
        m_bounds.height = constraints.maxH;

        float scrollOffset = m_scrollController.GetOffset();
        float currentY = m_bounds.y + 16.0f - scrollOffset;
        float spacingY = 12.0f;

        jay::engine::BoxConstraints childConstraints = jay::engine::BoxConstraints::Loose(m_bounds.width - 32.0f, m_bounds.height);

        for (auto& child : m_children) {
            if (child->IsVisible()) {
                child->Layout(childConstraints);
                jay::engine::Rect childBounds = child->GetBounds();
                childBounds.y = currentY;
                childBounds.x += m_bounds.x + 16.0f;
                child->SetBounds(childBounds);

                currentY += childBounds.height + spacingY;
            }
        }

        m_contentHeight = (currentY + scrollOffset) - (m_bounds.y + 16.0f);
        float maxScroll = std::max(0.0f, m_contentHeight - m_bounds.height + 32.0f);
        m_scrollController.Clamp(0.0f, maxScroll);

        m_layoutDirty = false;
    }

    void Render(jay::engine::RenderContext& ctx) const override {
        if (!m_visible) return;

        // Renderiza o feed com Scissor Stack ativado
        ctx.PushScissor(m_bounds);
        ContainerWidget::Render(ctx);
        ctx.PopScissor();

        // Renderiza a barra de rolagem (Scrollbar 8px)
        float maxScroll = std::max(0.0f, m_contentHeight - m_bounds.height + 32.0f);
        if (m_contentHeight > m_bounds.height) {
            jay::engine::Rect trackRect{
                m_bounds.x + m_bounds.width - 12.0f,
                m_bounds.y + 4.0f,
                8.0f,
                m_bounds.height - 8.0f
            };
            ctx.DrawRectRounded(trackRect, 4.0f, jay::engine::Color{31, 41, 55, 120});

            float ratio = m_bounds.height / m_contentHeight;
            float thumbH = std::max(24.0f, ratio * trackRect.height);
            float scrollPercent = (maxScroll > 0.0f) ? (m_scrollController.GetOffset() / maxScroll) : 0.0f;
            float thumbY = trackRect.y + scrollPercent * (trackRect.height - thumbH);

            jay::engine::Rect thumbRect{trackRect.x, thumbY, 8.0f, thumbH};
            ctx.DrawRectRounded(thumbRect, 4.0f, jay::engine::Color{88, 166, 255, 200});
        }
    }

    bool OnEvent(const jay::engine::InputEvent& event) override {
        if (!m_visible || !m_enabled) return false;

        float maxScroll = std::max(0.0f, m_contentHeight - m_bounds.height + 32.0f);

        // 1. Processa rolagem por mouse wheel (WheelSensChat = 140px)
        if (event.kind == jay::engine::InputEventKind::MouseScroll) {
            if (m_bounds.Contains({event.mouseX, event.mouseY})) {
                // event.scrollDelta positivo = roda para cima -> reduz offset
                m_scrollController.ApplyWheel(-event.scrollDelta);
                m_scrollController.Clamp(0.0f, maxScroll);
                UpdateWasAtBottom(maxScroll);
                MarkLayoutDirty();
                event.handled = true;
                return true;
            }
        }

        // 2. Processa rolagem por teclado (PageUp / PageDown)
        if (event.kind == jay::engine::InputEventKind::KeyPress) {
            if (event.key == 266) { // KEY_PAGE_UP
                m_scrollController.ScrollByKeyboard(-m_bounds.height * 0.8f);
                m_scrollController.Clamp(0.0f, maxScroll);
                UpdateWasAtBottom(maxScroll);
                MarkLayoutDirty();
                event.handled = true;
                return true;
            }
            if (event.key == 267) { // KEY_PAGE_DOWN
                m_scrollController.ScrollByKeyboard(m_bounds.height * 0.8f);
                m_scrollController.Clamp(0.0f, maxScroll);
                UpdateWasAtBottom(maxScroll);
                MarkLayoutDirty();
                event.handled = true;
                return true;
            }
        }

        // Delegar aos balões filhos
        return ContainerWidget::OnEvent(event);
    }

private:
    std::shared_ptr<ChatViewModel> m_viewModel;
    jay::ScrollController          m_scrollController;
    float                          m_contentHeight;
    bool                           m_wasAtBottom;

    void UpdateWasAtBottom(float maxScroll) {
        float currentOffset = m_scrollController.GetOffset();
        // Se a distância até a base for menor que 50px, considera que o usuário está no fundo
        m_wasAtBottom = (maxScroll - currentOffset) <= 50.0f;
    }

    void SyncMessagesWithState() {
        if (!m_viewModel) return;

        m_children.clear();
        const auto& messages = m_viewModel->GetMessages();
        bool lastIsUser = false;

        if (!messages.empty()) {
            lastIsUser = (messages.back().sender == "user");
        }

        for (const auto& msg : messages) {
            auto bubble = std::make_unique<MessageBubbleWidget>(msg);
            AddChild(std::move(bubble));
        }

        MarkLayoutDirty();

        // Regra do Auto-Scroll Inteligente (CHAT-002):
        // Se a mensagem for do usuário OU se o usuário já estava no fundo (margem de 50px),
        // força o scroll para a base.
        if (lastIsUser || m_wasAtBottom) {
            float maxScroll = std::max(0.0f, m_contentHeight - m_bounds.height + 32.0f);
            m_scrollController.SetOffset(maxScroll);
            m_wasAtBottom = true;
        }
    }
};

} // namespace jay::features::chat
