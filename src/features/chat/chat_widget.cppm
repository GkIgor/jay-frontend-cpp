module;
#include <memory>
#include <vector>
export module jay.features.chat.widget;

import jay.engine.types;
import jay.engine.render_context;
import jay.engine.widget;
import jay.features.chat.viewmodel;
import jay.features.chat.message_bubble_widget;

export namespace jay::features::chat {

class ChatWidget : public jay::engine::ContainerWidget {
public:
    explicit ChatWidget(std::shared_ptr<ChatViewModel> viewModel)
        : m_viewModel(std::move(viewModel)) {
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

        // Distribui layout verticalmente entre os balões de mensagem
        float currentY = m_bounds.y + 16.0f;
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

        m_layoutDirty = false;
    }

    void Render(jay::engine::RenderContext& ctx) const override {
        if (!m_visible) return;

        // Desenha feed de chat com Scissor ativado
        ctx.PushScissor(m_bounds);
        ContainerWidget::Render(ctx);
        ctx.PopScissor();
    }

private:
    std::shared_ptr<ChatViewModel> m_viewModel;

    void SyncMessagesWithState() {
        if (!m_viewModel) return;

        m_children.clear();
        const auto& messages = m_viewModel->GetMessages();

        for (const auto& msg : messages) {
            auto bubble = std::make_unique<MessageBubbleWidget>(msg);
            AddChild(std::move(bubble));
        }

        MarkLayoutDirty();
    }
};

} // namespace jay::features::chat
