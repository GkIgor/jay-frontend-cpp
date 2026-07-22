module;
#include <string>
#include <functional>
export module jay.shared.widgets.button;

import jay.engine.types;
import jay.engine.render_context;
import jay.engine.widget;

export namespace jay::shared::widgets {

class Button : public jay::engine::Widget {
public:
    explicit Button(std::string label = "Button", std::function<void()> onPress = nullptr)
        : m_label(std::move(label)), m_onPress(std::move(onPress)) {
        tabIndex = 0; // Focável por padrão
    }

    void SetLabel(std::string label) {
        m_label = std::move(label);
        MarkLayoutDirty();
    }

    void SetOnPress(std::function<void()> onPress) {
        m_onPress = std::move(onPress);
    }

    void Update(float /*deltaTime*/) override {
        // Avança transient UI state de hover / pressed
    }

    void Layout(const jay::engine::BoxConstraints& constraints) override {
        m_bounds.width  = (constraints.maxW > 0.0f) ? constraints.maxW : 120.0f;
        m_bounds.height = (constraints.maxH > 0.0f) ? constraints.maxH : 36.0f;
        m_layoutDirty   = false;
    }

    void Render(jay::engine::RenderContext& ctx) const override {
        jay::engine::Color bg = m_isPressed ? jay::engine::Color{31, 111, 235, 255}
                              : m_isHovered ? jay::engine::Color{56, 139, 253, 255}
                                            : jay::engine::Color{48, 54, 61, 255};

        ctx.DrawRectRounded(m_bounds, 6.0f, bg);
        ctx.DrawRectLines(m_bounds, 1.0f, jay::engine::Color{88, 166, 255, 255});

        float fontSize = 14.0f;
        float textW = ctx.MeasureText(m_label, fontSize);
        float textX = m_bounds.x + (m_bounds.width - textW) * 0.5f;
        float textY = m_bounds.y + (m_bounds.height - fontSize) * 0.5f;

        ctx.DrawText(m_label, {textX, textY}, fontSize, jay::engine::Color{255, 255, 255, 255});
    }

    bool OnEvent(const jay::engine::InputEvent& event) override {
        if (!m_visible || !m_enabled) return false;

        if (event.kind == jay::engine::InputEventKind::MouseMove) {
            m_isHovered = m_bounds.Contains({event.mouseX, event.mouseY});
        }

        if (event.kind == jay::engine::InputEventKind::MousePress && event.key == 0) {
            if (m_bounds.Contains({event.mouseX, event.mouseY})) {
                m_isPressed = true;
                event.handled = true;
                return true;
            }
        }

        if (event.kind == jay::engine::InputEventKind::MouseRelease && event.key == 0) {
            if (m_isPressed) {
                m_isPressed = false;
                if (m_bounds.Contains({event.mouseX, event.mouseY})) {
                    if (m_onPress) m_onPress();
                }
                event.handled = true;
                return true;
            }
        }

        return false;
    }

private:
    std::string           m_label;
    std::function<void()> m_onPress;

    // Transient UI State (efêmero e local ao widget)
    bool m_isHovered{false};
    bool m_isPressed{false};
};

} // namespace jay::shared::widgets
