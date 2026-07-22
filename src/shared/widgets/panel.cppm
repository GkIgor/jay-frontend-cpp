module;
#include <memory>
#include <vector>
export module jay.shared.widgets.panel;

import jay.engine.types;
import jay.engine.render_context;
import jay.engine.widget;

export namespace jay::shared::widgets {

class Panel : public jay::engine::ContainerWidget {
public:
    explicit Panel(jay::engine::Color backgroundColor = {22, 27, 34, 255},
                   jay::engine::Color borderColor = {48, 54, 61, 255},
                   float cornerRadiusPx = 8.0f)
        : m_backgroundColor(backgroundColor), m_borderColor(borderColor), m_cornerRadiusPx(cornerRadiusPx) {}

    void SetBackgroundColor(jay::engine::Color color) noexcept { m_backgroundColor = color; }
    void SetBorderColor(jay::engine::Color color) noexcept { m_borderColor = color; }

    void Layout(const jay::engine::BoxConstraints& constraints) override {
        m_bounds.width  = constraints.maxW;
        m_bounds.height = constraints.maxH;

        // Distribui constraints para os filhos
        jay::engine::BoxConstraints childConstraints = jay::engine::BoxConstraints::Loose(m_bounds.width, m_bounds.height);
        for (auto& child : m_children) {
            if (child->IsVisible() && child->IsLayoutDirty()) {
                child->Layout(childConstraints);
            }
        }
        m_layoutDirty = false;
    }

    void Render(jay::engine::RenderContext& ctx) const override {
        if (!m_visible) return;

        ctx.DrawRectRounded(m_bounds, m_cornerRadiusPx, m_backgroundColor);
        if (m_borderColor.a > 0) {
            ctx.DrawRectLines(m_bounds, 1.0f, m_borderColor);
        }

        // Renderiza os filhos com Scissor recortando nos limites do painel
        ctx.PushScissor(m_bounds);
        ContainerWidget::Render(ctx);
        ctx.PopScissor();
    }

private:
    jay::engine::Color m_backgroundColor;
    jay::engine::Color m_borderColor;
    float              m_cornerRadiusPx{8.0f};
};

} // namespace jay::shared::widgets
