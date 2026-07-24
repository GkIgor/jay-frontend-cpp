module;
#include <memory>
#include <vector>
#include <algorithm>
export module jay.shared.widgets.layout_primitives;

import jay.engine.types;
import jay.engine.render_context;
import jay.engine.widget;

export namespace jay::shared::widgets {

struct EdgeInsets {
    float top{0.0f};
    float right{0.0f};
    float bottom{0.0f};
    float left{0.0f};

    static EdgeInsets All(float value) noexcept {
        return {value, value, value, value};
    }

    static EdgeInsets Symmetric(float vertical, float horizontal) noexcept {
        return {vertical, horizontal, vertical, horizontal};
    }

    static EdgeInsets LTRB(float l, float t, float r, float b) noexcept {
        return {t, r, b, l};
    }
};

class PaddingWidget : public jay::engine::ContainerWidget {
public:
    PaddingWidget(EdgeInsets padding, std::unique_ptr<jay::engine::Widget> child)
        : m_padding(padding) {
        if (child) AddChild(std::move(child));
    }

    void Layout(const jay::engine::BoxConstraints& constraints) override {
        float horizontalPadding = m_padding.left + m_padding.right;
        float verticalPadding   = m_padding.top + m_padding.bottom;

        float childMaxW = std::max(0.0f, constraints.maxW - horizontalPadding);
        float childMaxH = std::max(0.0f, constraints.maxH - verticalPadding);

        jay::engine::BoxConstraints childConstraints = jay::engine::BoxConstraints::Loose(childMaxW, childMaxH);

        float contentW = 0.0f;
        float contentH = 0.0f;

        if (!m_children.empty() && m_children[0]->IsVisible()) {
            auto& child = m_children[0];
            child->Layout(childConstraints);
            jay::engine::Rect childBounds = child->GetBounds();

            child->SetBounds(jay::engine::Rect{
                m_bounds.x + m_padding.left,
                m_bounds.y + m_padding.top,
                childBounds.width,
                childBounds.height
            });

            contentW = childBounds.width;
            contentH = childBounds.height;
        }

        m_bounds.width  = contentW + horizontalPadding;
        m_bounds.height = contentH + verticalPadding;
        m_layoutDirty   = false;
    }

private:
    EdgeInsets m_padding;
};

class ColumnWidget : public jay::engine::ContainerWidget {
public:
    explicit ColumnWidget(float spacing = 8.0f)
        : m_spacing(spacing) {}

    void Layout(const jay::engine::BoxConstraints& constraints) override {
        m_bounds.width = constraints.maxW;

        float currentY = m_bounds.y;
        float maxChildW = 0.0f;

        jay::engine::BoxConstraints childConstraints = jay::engine::BoxConstraints::Loose(constraints.maxW, constraints.maxH);

        for (size_t i = 0; i < m_children.size(); ++i) {
            auto& child = m_children[i];
            if (!child->IsVisible()) continue;

            child->Layout(childConstraints);
            jay::engine::Rect childBounds = child->GetBounds();

            child->SetBounds(jay::engine::Rect{
                m_bounds.x,
                currentY,
                childBounds.width,
                childBounds.height
            });

            currentY += childBounds.height + ((i < m_children.size() - 1) ? m_spacing : 0.0f);
            if (childBounds.width > maxChildW) maxChildW = childBounds.width;
        }

        m_bounds.height = currentY - m_bounds.y;
        m_layoutDirty   = false;
    }

private:
    float m_spacing;
};

class RowWidget : public jay::engine::ContainerWidget {
public:
    explicit RowWidget(float spacing = 8.0f)
        : m_spacing(spacing) {}

    void Layout(const jay::engine::BoxConstraints& constraints) override {
        float currentX = m_bounds.x;
        float maxChildH = 0.0f;

        jay::engine::BoxConstraints childConstraints = jay::engine::BoxConstraints::Loose(constraints.maxW, constraints.maxH);

        for (size_t i = 0; i < m_children.size(); ++i) {
            auto& child = m_children[i];
            if (!child->IsVisible()) continue;

            child->Layout(childConstraints);
            jay::engine::Rect childBounds = child->GetBounds();

            child->SetBounds(jay::engine::Rect{
                currentX,
                m_bounds.y,
                childBounds.width,
                childBounds.height
            });

            currentX += childBounds.width + ((i < m_children.size() - 1) ? m_spacing : 0.0f);
            if (childBounds.height > maxChildH) maxChildH = childBounds.height;
        }

        m_bounds.width  = currentX - m_bounds.x;
        m_bounds.height = maxChildH;
        m_layoutDirty   = false;
    }

private:
    float m_spacing;
};

} // namespace jay::shared::widgets
