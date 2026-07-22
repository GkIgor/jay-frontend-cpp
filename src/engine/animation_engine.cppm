module;
#include <cmath>
#include <cstdint>
export module jay.engine.animation_engine;

import jay.engine.types;

export namespace jay::engine {

// ─────────────────────────────────────────────────────────────────
// Funções de Easing
//
// Todas recebem t em [0.0, 1.0] e retornam valor em [0.0, 1.0].
// Não alocam. São funções puras e estáticas.
// ─────────────────────────────────────────────────────────────────
struct Easing {
    [[nodiscard]] static float Linear(float t) noexcept {
        return t;
    }

    [[nodiscard]] static float InOutCubic(float t) noexcept {
        return t < 0.5f
            ? 4.0f * t * t * t
            : 1.0f - ((-2.0f * t + 2.0f) * (-2.0f * t + 2.0f) * (-2.0f * t + 2.0f)) / 2.0f;
    }

    [[nodiscard]] static float OutBounce(float t) noexcept {
        constexpr float n1 = 7.5625f;
        constexpr float d1 = 2.75f;
        if (t < 1.0f / d1) {
            return n1 * t * t;
        } else if (t < 2.0f / d1) {
            t -= 1.5f / d1;
            return n1 * t * t + 0.75f;
        } else if (t < 2.5f / d1) {
            t -= 2.25f / d1;
            return n1 * t * t + 0.9375f;
        } else {
            t -= 2.625f / d1;
            return n1 * t * t + 0.984375f;
        }
    }

    // Spring com stiffness e damping configuráveis.
    // Aproximação numérica discreta; para uso visual é suficiente.
    [[nodiscard]] static float Spring(float t) noexcept {
        // e^(-6t) * cos(2π*t) + 1 — decaimento oscilatório suave.
        constexpr float pi2 = 6.2831853f;
        return 1.0f - std::exp(-6.0f * t) * std::cos(pi2 * t);
    }
};

// ─────────────────────────────────────────────────────────────────
// Tween<T> — interpolador genérico de propriedades visuais.
//
// Suporte: float, Vec2, Color.
//
// Uso correto:
//   - Start(...) é chamado fora do Hot Loop (durante Init ou eventos).
//   - Tick(dt) é chamado no Update() — não aloca, retorna valor por valor.
//
// Zero alocação: todos os membros são escalares.
// ─────────────────────────────────────────────────────────────────
template<typename T>
class Tween {
public:
    using EasingFn = float(*)(float) noexcept;

    Tween() = default;

    // Configura e inicia a animação. Chamado fora do Hot Loop.
    void Start(T from, T to, float durationSeconds,
               EasingFn easingFn = Easing::InOutCubic) noexcept {
        m_from     = from;
        m_to       = to;
        m_duration = durationSeconds;
        m_elapsed  = 0.0f;
        m_easing   = easingFn;
        m_running  = true;
    }

    // Avança a animação por dt segundos e retorna o valor atual.
    // Chamado no Update() do widget — não aloca.
    [[nodiscard]] T Tick(float dt) noexcept {
        if (!m_running) return m_to;

        m_elapsed += dt;
        if (m_elapsed >= m_duration) {
            m_elapsed = m_duration;
            m_running = false;
        }

        float t = (m_duration > 0.0f) ? (m_elapsed / m_duration) : 1.0f;
        float easedT = m_easing ? m_easing(t) : t;
        return Lerp(m_from, m_to, easedT);
    }

    // Retorna o valor atual sem avançar o tempo.
    [[nodiscard]] T Current() const noexcept {
        float t = (m_duration > 0.0f) ? (m_elapsed / m_duration) : 1.0f;
        if (t > 1.0f) t = 1.0f;
        float easedT = m_easing ? m_easing(t) : t;
        return Lerp(m_from, m_to, easedT);
    }

    [[nodiscard]] bool IsComplete() const noexcept { return !m_running; }
    [[nodiscard]] bool IsRunning()  const noexcept { return m_running; }

    void Stop() noexcept { m_running = false; }

    void Reset() noexcept {
        m_elapsed = 0.0f;
        m_running = false;
    }

private:
    T        m_from{};
    T        m_to{};
    float    m_duration{0.0f};
    float    m_elapsed{0.0f};
    EasingFn m_easing{Easing::InOutCubic};
    bool     m_running{false};

    // ── Especialização de Lerp por tipo ──

    [[nodiscard]] static float Lerp(float a, float b, float t) noexcept {
        return a + (b - a) * t;
    }

    [[nodiscard]] static Vec2 Lerp(Vec2 a, Vec2 b, float t) noexcept {
        return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
    }

    [[nodiscard]] static Color Lerp(Color a, Color b, float t) noexcept {
        auto lerp_u8 = [](uint8_t x, uint8_t y, float tt) -> uint8_t {
            return static_cast<uint8_t>(x + static_cast<int>((y - x) * tt));
        };
        return {
            lerp_u8(a.r, b.r, t),
            lerp_u8(a.g, b.g, t),
            lerp_u8(a.b, b.b, t),
            lerp_u8(a.a, b.a, t),
        };
    }
};

// Aliases de uso comum — evitam necessidade de especificar o parâmetro de template.
using TweenFloat = Tween<float>;
using TweenVec2  = Tween<Vec2>;
using TweenColor = Tween<Color>;

} // namespace jay::engine
