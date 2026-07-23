module;
#include <string>
#include <vector>
#include <memory>
export module jay.features.chat.tool_group_widget;

import jay.engine.types;
import jay.engine.render_context;
import jay.engine.widget;
import jay.engine.animation_engine;

export namespace jay::features::chat {

struct ToolActionItem {
    std::string id;
    std::string name;
    bool        success{true};
    std::string error;
};

// ─────────────────────────────────────────────────────────────────
// ToolGroupWidget (Task 38: CHAT-009)
//
// Componente de Grupo de Ferramentas Retrátil:
//   - Agrupamento de ações de ferramentas em um balão expansível
//   - Cabeçalho com botão de triângulo e contador de ações ("N ações executadas")
//   - Animação de expansão/colapso via TweenFloat
//   - Linhas individuais com ícone check verde para sucesso e X vermelho para erros
// ─────────────────────────────────────────────────────────────────
class ToolGroupWidget : public jay::engine::Widget {
public:
    static constexpr float kHeaderHeight    = 36.0f;
    static constexpr float kActionRowHeight = 26.0f;
    static constexpr float kPaddingBottom   = 8.0f;

    explicit ToolGroupWidget(std::vector<ToolActionItem> actions)
        : m_actions(std::move(actions)),
          m_expanded(false),
          m_isHeaderHovered(false) {}

    void Init() override {
        float targetH = CalculateTargetHeight();
        m_heightTween.Start(targetH, targetH, 0.2f, jay::engine::Easing::InOutCubic);
    }

    void Update(float deltaTime) override {
        float currentH = m_heightTween.Tick(deltaTime);
        if (m_heightTween.IsRunning()) {
            m_bounds.height = currentH;
            MarkLayoutDirty();
        }
    }

    void Layout(const jay::engine::BoxConstraints& constraints) override {
        m_bounds.width  = constraints.maxW * 0.85f;
        m_bounds.height = m_heightTween.Current();
        m_layoutDirty   = false;
    }

    void Render(jay::engine::RenderContext& ctx) const override {
        if (!m_visible) return;

        jay::engine::Color bg{22, 27, 34, 255};      // Surface
        jay::engine::Color border{48, 54, 61, 255};  // Border

        // Corpo do balão do grupo de ferramentas
        ctx.DrawRectRounded(m_bounds, 8.0f, bg);
        ctx.DrawRectLines(m_bounds, 1.0f, border);

        // Cabeçalho do grupo
        jay::engine::Rect headerRect{m_bounds.x, m_bounds.y, m_bounds.width, kHeaderHeight};
        if (m_isHeaderHovered) {
            ctx.DrawRectRounded(headerRect, 8.0f, jay::engine::Color{255, 255, 255, 10});
        }

        // Ícone de triângulo indicador (expandido / colapsado)
        std::string_view arrowIcon = m_expanded ? "▼" : "▶";
        jay::engine::Color arrowCol = m_isHeaderHovered
            ? jay::engine::Color{201, 209, 217, 255}
            : jay::engine::Color{139, 148, 158, 255};

        ctx.DrawText(arrowIcon, {m_bounds.x + 14.0f, m_bounds.y + 10.0f}, 12.0f, arrowCol);

        // Texto do contador de ações
        int count = static_cast<int>(m_actions.size());
        std::string label = std::to_string(count) + (count == 1 ? " ação executada" : " ações executadas");
        ctx.DrawText(label, {m_bounds.x + 32.0f, m_bounds.y + 9.0f}, 13.0f, arrowCol);

        // Desenha lista de ações se expandido
        if (m_expanded && m_bounds.height > kHeaderHeight + 4.0f) {
            // Linha divisória
            ctx.DrawRect(
                jay::engine::Rect{m_bounds.x + 8.0f, m_bounds.y + kHeaderHeight, m_bounds.width - 16.0f, 1.0f},
                border
            );

            float rowY = m_bounds.y + kHeaderHeight + 6.0f;
            for (const auto& action : m_actions) {
                if (rowY + kActionRowHeight > m_bounds.y + m_bounds.height) break;

                float iconX = m_bounds.x + 14.0f;

                if (action.success) {
                    ctx.DrawText("✓", {iconX, rowY + 2.0f}, 13.0f, jay::engine::Color{48, 161, 78, 255}); // Success
                } else {
                    ctx.DrawText("✗", {iconX, rowY + 2.0f}, 13.0f, jay::engine::Color{218, 54, 51, 255}); // Danger
                }

                jay::engine::Color nameCol = action.success
                    ? jay::engine::Color{139, 148, 158, 255}
                    : jay::engine::Color{218, 54, 51, 255};

                ctx.DrawText(action.name, {m_bounds.x + 32.0f, rowY + 3.0f}, 13.0f, nameCol);

                if (!action.error.empty()) {
                    std::string errText = " — " + action.error;
                    float nameW = ctx.MeasureText(action.name, 13.0f);
                    ctx.DrawText(errText, {m_bounds.x + 32.0f + nameW, rowY + 3.0f}, 12.0f, jay::engine::Color{218, 54, 51, 255});
                }

                rowY += kActionRowHeight;
            }
        }
    }

    bool OnEvent(const jay::engine::InputEvent& event) override {
        if (!m_visible || !m_enabled) return false;

        jay::engine::Rect headerRect{m_bounds.x, m_bounds.y, m_bounds.width, kHeaderHeight};

        if (event.kind == jay::engine::InputEventKind::MouseMove) {
            m_isHeaderHovered = headerRect.Contains({event.mouseX, event.mouseY});
        }

        if (event.kind == jay::engine::InputEventKind::MousePress && event.key == 0) {
            if (headerRect.Contains({event.mouseX, event.mouseY})) {
                ToggleExpanded();
                event.handled = true;
                return true;
            }
        }

        return false;
    }

private:
    std::vector<ToolActionItem> m_actions;
    bool                        m_expanded;
    bool                        m_isHeaderHovered;
    jay::engine::TweenFloat     m_heightTween;

    [[nodiscard]] float CalculateTargetHeight() const noexcept {
        if (!m_expanded) return kHeaderHeight;
        return kHeaderHeight + m_actions.size() * kActionRowHeight + kPaddingBottom;
    }

    void ToggleExpanded() {
        m_expanded = !m_expanded;
        float currentH = m_bounds.height;
        float targetH  = CalculateTargetHeight();
        m_heightTween.Start(currentH, targetH, 0.2f, jay::engine::Easing::InOutCubic);
    }
};

} // namespace jay::features::chat
