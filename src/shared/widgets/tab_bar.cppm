module;
#include <string>
#include <vector>
#include <functional>
export module jay.shared.widgets.tab_bar;

import jay.engine.types;
import jay.engine.render_context;
import jay.engine.widget;

export namespace jay::shared::widgets {

class TabBarWidget : public jay::engine::Widget {
public:
    explicit TabBarWidget(std::vector<std::string> labels)
        : m_labels(std::move(labels)),
          m_activeTab(0),
          m_hoveredTab(-1) {}

    void SetOnTabChanged(std::function<void(int)> callback) {
        m_onTabChanged = std::move(callback);
    }

    void SetActiveTab(int index) {
        if (index >= 0 && index < static_cast<int>(m_labels.size()) && index != m_activeTab) {
            m_activeTab = index;
            if (m_onTabChanged) m_onTabChanged(m_activeTab);
            MarkLayoutDirty();
        }
    }

    [[nodiscard]] int GetActiveTab() const noexcept {
        return m_activeTab;
    }

    void Layout(const jay::engine::BoxConstraints& constraints) override {
        m_bounds.width  = constraints.maxW;
        m_bounds.height = (constraints.maxH > 0.0f) ? constraints.maxH : 48.0f;
        m_layoutDirty   = false;
    }

    void Render(jay::engine::RenderContext& ctx) const override {
        if (!m_visible || m_labels.empty()) return;

        jay::engine::Color bg{22, 27, 34, 255};
        jay::engine::Color border{48, 54, 61, 255};
        jay::engine::Color activeGlow{88, 166, 255, 255};
        jay::engine::Color textMain{201, 209, 217, 255};
        jay::engine::Color textSec{139, 148, 158, 255};

        ctx.DrawRect(m_bounds, bg);
        ctx.DrawRect(
            jay::engine::Rect{m_bounds.x, m_bounds.y + m_bounds.height - 1.0f, m_bounds.width, 1.0f},
            border
        );

        float tabW = m_bounds.width / static_cast<float>(m_labels.size());

        for (size_t i = 0; i < m_labels.size(); ++i) {
            float tabX = m_bounds.x + i * tabW;
            jay::engine::Rect tabRect{tabX, m_bounds.y, tabW, m_bounds.height};

            bool isActive  = (m_activeTab == static_cast<int>(i));
            bool isHovered = (m_hoveredTab == static_cast<int>(i));

            if (isActive) {
                ctx.DrawRect(tabRect, jay::engine::Color{31, 111, 235, 40});
                ctx.DrawRect(
                    jay::engine::Rect{tabX + 20.0f, m_bounds.y + m_bounds.height - 3.0f, tabW - 40.0f, 3.0f},
                    activeGlow
                );
            } else if (isHovered) {
                ctx.DrawRect(tabRect, jay::engine::Color{255, 255, 255, 10});
            }

            float textW = ctx.MeasureText(m_labels[i], 16.0f);
            float textX = tabX + (tabW - textW) * 0.5f;
            float textY = m_bounds.y + (m_bounds.height - 16.0f) * 0.5f;

            ctx.DrawText(
                m_labels[i],
                {textX, textY},
                16.0f,
                isActive ? textMain : textSec
            );
        }
    }

    bool OnEvent(const jay::engine::InputEvent& event) override {
        if (!m_visible || !m_enabled || m_labels.empty()) return false;

        float tabW = m_bounds.width / static_cast<float>(m_labels.size());

        if (event.kind == jay::engine::InputEventKind::MouseMove) {
            m_hoveredTab = -1;
            if (m_bounds.Contains({event.mouseX, event.mouseY})) {
                int index = static_cast<int>((event.mouseX - m_bounds.x) / tabW);
                if (index >= 0 && index < static_cast<int>(m_labels.size())) {
                    m_hoveredTab = index;
                }
            }
        }

        if (event.kind == jay::engine::InputEventKind::MousePress && event.key == 0) {
            if (m_bounds.Contains({event.mouseX, event.mouseY})) {
                int clickedIndex = static_cast<int>((event.mouseX - m_bounds.x) / tabW);
                if (clickedIndex >= 0 && clickedIndex < static_cast<int>(m_labels.size())) {
                    SetActiveTab(clickedIndex);
                    event.handled = true;
                    return true;
                }
            }
        }

        // Atalho global Ctrl + Tab para alternar abas
        if (event.kind == jay::engine::InputEventKind::KeyPress && event.ctrl) {
            if (event.key == 258) { // KEY_TAB
                int nextTab = (m_activeTab + 1) % static_cast<int>(m_labels.size());
                SetActiveTab(nextTab);
                event.handled = true;
                return true;
            }
        }

        return false;
    }

private:
    std::vector<std::string> m_labels;
    int                      m_activeTab;
    int                      m_hoveredTab;
    std::function<void(int)> m_onTabChanged;
};

} // namespace jay::shared::widgets
