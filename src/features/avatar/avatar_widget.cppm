module;
#include <memory>
#include <cmath>
export module jay.features.avatar.widget;

import jay.engine.types;
import jay.engine.render_context;
import jay.engine.widget;
import jay.engine.animation_engine;
import jay.features.avatar.viewmodel;

export namespace jay::features::avatar {

class AvatarWidget : public jay::engine::Widget {
public:
    explicit AvatarWidget(std::shared_ptr<AvatarViewModel> viewModel)
        : m_viewModel(std::move(viewModel)) {
        if (m_viewModel) {
            m_viewModel->SetOnUpdateCallback([this]() { MarkLayoutDirty(); });
        }
    }

    void Init() override {
        // Inicializa animações via Tween sem alocações dinâmicas no Hot Loop
        m_pulseTween.Start(0.95f, 1.05f, 1.5f, jay::engine::Easing::InOutCubic);
        if (m_viewModel) {
            m_colorTween.Start(m_viewModel->GetTargetColor(), m_viewModel->GetTargetColor(), 0.3f);
        }
    }

    void Update(float deltaTime) override {
        // Avança as animações visuais com base no tempo delta
        float scale = m_pulseTween.Tick(deltaTime);
        if (m_pulseTween.IsComplete()) {
            // Loop do pulso
            float from = (scale > 1.0f) ? 1.05f : 0.95f;
            float to   = (scale > 1.0f) ? 0.95f : 1.05f;
            m_pulseTween.Start(from, to, 1.5f, jay::engine::Easing::InOutCubic);
        }

        if (m_viewModel) {
            jay::engine::Color targetColor = m_viewModel->GetTargetColor();
            if (m_currentColor.r != targetColor.r || m_currentColor.g != targetColor.g || m_currentColor.b != targetColor.b) {
                m_colorTween.Start(m_currentColor, targetColor, 0.3f);
            }
        }

        m_currentColor = m_colorTween.Tick(deltaTime);
    }

    void Layout(const jay::engine::BoxConstraints& constraints) override {
        m_bounds.width  = constraints.maxW;
        m_bounds.height = constraints.maxH;
        m_layoutDirty   = false;
    }

    void Render(jay::engine::RenderContext& ctx) const override {
        if (!m_visible) return;

        // Calcula o centro e o raio base do avatar
        float centerX = m_bounds.x + m_bounds.width * 0.5f;
        float centerY = m_bounds.y + m_bounds.height * 0.4f;
        float baseRadius = (m_bounds.width < m_bounds.height ? m_bounds.width : m_bounds.height) * 0.18f;

        float currentRadius = baseRadius * m_pulseTween.Current();

        // 1. Halo Externo (Translúcido)
        jay::engine::Color haloColor = m_currentColor;
        haloColor.a = 40;
        jay::engine::Rect haloRect{
            centerX - currentRadius * 1.3f,
            centerY - currentRadius * 1.3f,
            currentRadius * 2.6f,
            currentRadius * 2.6f
        };
        ctx.DrawRectRounded(haloRect, haloRect.width * 0.5f, haloColor);

        // 2. Círculo Núcleo Principal
        jay::engine::Rect coreRect{
            centerX - currentRadius,
            centerY - currentRadius,
            currentRadius * 2.0f,
            currentRadius * 2.0f
        };
        ctx.DrawRectRounded(coreRect, coreRect.width * 0.5f, m_currentColor);

        // 3. Texto Informativo de Estado
        std::string_view stateText = "JAY ASSISTANT";
        float fontSize = 16.0f;
        float textW = ctx.MeasureText(stateText, fontSize);
        ctx.DrawText(stateText, {centerX - textW * 0.5f, centerY + currentRadius + 30.0f}, fontSize, jay::engine::Color{201, 209, 217, 255});
    }

private:
    std::shared_ptr<AvatarViewModel> m_viewModel;
    jay::engine::TweenFloat          m_pulseTween;
    jay::engine::TweenColor          m_colorTween;
    jay::engine::Color               m_currentColor{88, 166, 255, 255};
};

} // namespace jay::features::avatar
