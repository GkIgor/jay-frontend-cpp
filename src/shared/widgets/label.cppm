module;
#include <string>
#include <string_view>
export module jay.shared.widgets.label;

import jay.engine.types;
import jay.engine.render_context;
import jay.engine.widget;

export namespace jay::shared::widgets {

class Label : public jay::engine::Widget {
public:
    explicit Label(std::string text = "", float fontSize = 16.0f, jay::engine::Color color = {201, 209, 217, 255})
        : m_text(std::move(text)), m_fontSize(fontSize), m_color(color) {}

    void SetText(std::string text) {
        m_text = std::move(text);
        MarkLayoutDirty();
    }

    [[nodiscard]] const std::string& GetText() const noexcept { return m_text; }

    void SetFontSize(float fontSize) noexcept {
        m_fontSize = fontSize;
        MarkLayoutDirty();
    }

    void SetColor(jay::engine::Color color) noexcept {
        m_color = color;
    }

    void Layout(const jay::engine::BoxConstraints& constraints) override {
        // Sem alocação: m_bounds é ajustado com base nos limites e tamanho do texto
        m_bounds.width  = constraints.maxW;
        m_bounds.height = m_fontSize;
        m_layoutDirty   = false;
    }

    void Render(jay::engine::RenderContext& ctx) const override {
        if (m_text.empty()) return;
        ctx.DrawText(m_text, {m_bounds.x, m_bounds.y}, m_fontSize, m_color);
    }

private:
    std::string         m_text;
    float               m_fontSize{16.0f};
    jay::engine::Color  m_color{201, 209, 217, 255};
};

} // namespace jay::shared::widgets
