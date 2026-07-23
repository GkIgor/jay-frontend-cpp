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

        ctx.PushScissor(m_bounds);
        ContainerWidget::Render(ctx);
        ctx.PopScissor();

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

        if (event.kind == jay::engine::InputEventKind::MouseScroll) {
            if (m_bounds.Contains({event.mouseX, event.mouseY})) {
                m_scrollController.ApplyWheel(-event.scrollDelta);
                m_scrollController.Clamp(0.0f, maxScroll);
                UpdateWasAtBottom(maxScroll);
                MarkLayoutDirty();
                event.handled = true;
                return true;
            }
        }

        if (event.kind == jay::engine::InputEventKind::KeyPress) {
            if (event.key == 266) {
                m_scrollController.ScrollByKeyboard(-m_bounds.height * 0.8f);
                m_scrollController.Clamp(0.0f, maxScroll);
                UpdateWasAtBottom(maxScroll);
                MarkLayoutDirty();
                event.handled = true;
                return true;
            }
            if (event.key == 267) {
                m_scrollController.ScrollByKeyboard(m_bounds.height * 0.8f);
                m_scrollController.Clamp(0.0f, maxScroll);
                UpdateWasAtBottom(maxScroll);
                MarkLayoutDirty();
                event.handled = true;
                return true;
            }
        }

        return ContainerWidget::OnEvent(event);
    }

private:
    std::shared_ptr<ChatViewModel> m_viewModel;
    jay::ScrollController          m_scrollController;
    float                          m_contentHeight;
    bool                           m_wasAtBottom;

    void UpdateWasAtBottom(float maxScroll) {
        float currentOffset = m_scrollController.GetOffset();
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

        if (lastIsUser || m_wasAtBottom) {
            float maxScroll = std::max(0.0f, m_contentHeight - m_bounds.height + 32.0f);
            m_scrollController.SetOffset(maxScroll);
            m_wasAtBottom = true;
        }
    }
};

} // namespace jay::features::chat
