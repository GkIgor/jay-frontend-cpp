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
import jay.shared.widgets.layout_primitives;
import jay.state.actions;
import unicode;

export namespace jay::features::chat {

class MessageBubbleWidget : public jay::engine::Widget {
public:
    explicit MessageBubbleWidget(jay::state::MessageDTO message)
        : m_message(std::move(message)),
          m_copyTimer(0.0f),
          m_isCopyHovered(false),
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
        float maxBubbleW = constraints.maxW * 0.70f;
        if (maxBubbleW < 140.0f) maxBubbleW = constraints.maxW;

        float fontSize = 16.0f;
        float paddingH = 18.0f;
        float paddingV = 14.0f;
        float copyBtnH = 26.0f;

        float availableTextW = maxBubbleW - paddingH * 2.0f;
        if (availableTextW < 60.0f) availableTextW = 60.0f;

        float approxCharW = 9.0f;
        float textWidthEst = static_cast<float>(m_cleanContent.length()) * approxCharW;
        float lines = std::ceil(textWidthEst / availableTextW);
        if (lines < 1.0f) lines = 1.0f;

        float bubbleW = (textWidthEst < availableTextW) ? (textWidthEst + paddingH * 2.0f + 40.0f) : maxBubbleW;
        if (bubbleW < 120.0f) bubbleW = 120.0f;

        float textContentH = lines * (fontSize + 6.0f);
        float bubbleH = paddingV * 2.0f + textContentH + copyBtnH;

        m_bounds.width  = bubbleW;
        m_bounds.height = bubbleH;

        m_copyBtnRect = jay::engine::Rect{
            m_bounds.x + m_bounds.width - 32.0f,
            m_bounds.y + m_bounds.height - 30.0f,
            24.0f,
            22.0f
        };

        m_layoutDirty = false;
    }

    void SetBounds(jay::engine::Rect bounds) noexcept override {
        jay::engine::Widget::SetBounds(bounds);
        m_copyBtnRect = jay::engine::Rect{
            m_bounds.x + m_bounds.width - 32.0f,
            m_bounds.y + m_bounds.height - 30.0f,
            24.0f,
            22.0f
        };
    }

    void Render(jay::engine::RenderContext& ctx) const override {
        if (!m_visible) return;

        jay::engine::Color bg;
        jay::engine::Color border;
        jay::engine::Color textCol{230, 237, 243, 255};

        if (m_message.sender == "user") {
            bg     = jay::engine::Color{31, 111, 235, 255};
            border = jay::engine::Color{56, 139, 253, 255};
        } else if (m_message.status == jay::state::MessageStatus::Failed) {
            bg     = jay::engine::Color{218, 54, 51, 60};
            border = jay::engine::Color{218, 54, 51, 255};
        } else {
            bg     = jay::engine::Color{33, 38, 45, 255};
            border = jay::engine::Color{48, 54, 61, 255};
        }

        ctx.DrawRectRounded(m_bounds, 12.0f, bg);
        ctx.DrawRectLines(m_bounds, 1.0f, border);

        float fontSize = 16.0f;
        float paddingH = 18.0f;
        float paddingV = 14.0f;

        if (m_message.status == jay::state::MessageStatus::Pending && m_message.sender != "user") {
            float cy = m_bounds.y + paddingV + 10.0f;
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
            ctx.DrawText(
                m_cleanContent,
                {m_bounds.x + paddingH, m_bounds.y + paddingV},
                fontSize,
                textCol
            );
        }

        jay::engine::Color iconCol = m_isCopyHovered
            ? jay::engine::Color{88, 166, 255, 255}
            : jay::engine::Color{139, 148, 158, 255};

        if (m_copyTimer > 0.0f) {
            ctx.DrawRectRounded(m_copyBtnRect, 4.0f, jay::engine::Color{48, 161, 78, 255});
            ctx.DrawText("✓", {m_copyBtnRect.x + 6.0f, m_copyBtnRect.y + 3.0f}, 14.0f, jay::engine::Color{255, 255, 255, 255});
        } else {
            ctx.DrawText("📋", {m_copyBtnRect.x + 4.0f, m_copyBtnRect.y + 3.0f}, 14.0f, iconCol);
        }
    }

    bool OnEvent(const jay::engine::InputEvent& event) override {
        if (!m_visible || !m_enabled) return false;

        if (event.kind == jay::engine::InputEventKind::MouseMove) {
            m_isCopyHovered = m_copyBtnRect.Contains({event.mouseX, event.mouseY});
        }

        if (event.kind == jay::engine::InputEventKind::MousePress && event.key == 0) {
            if (m_copyBtnRect.Contains({event.mouseX, event.mouseY})) {
                m_copyTimer = 2.0f;
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
    float                 m_animTime;

    void TrimMessageText() {
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
