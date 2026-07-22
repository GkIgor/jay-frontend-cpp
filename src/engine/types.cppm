module;
#include <cstdint>
export module jay.engine.types;

export namespace jay::engine {

// ─────────────────────────────────────────────
// Tipos geométricos primitivos
// Nenhum cabeçalho Raylib é incluído aqui.
// ─────────────────────────────────────────────

struct Vec2 {
    float x{0.0f};
    float y{0.0f};

    [[nodiscard]] Vec2 operator+(Vec2 rhs) const noexcept { return {x + rhs.x, y + rhs.y}; }
    [[nodiscard]] Vec2 operator-(Vec2 rhs) const noexcept { return {x - rhs.x, y - rhs.y}; }
    [[nodiscard]] Vec2 operator*(float s) const noexcept  { return {x * s, y * s}; }
};

struct Rect {
    float x{0.0f};
    float y{0.0f};
    float width{0.0f};
    float height{0.0f};

    [[nodiscard]] bool Contains(Vec2 point) const noexcept {
        return point.x >= x && point.x <= x + width &&
               point.y >= y && point.y <= y + height;
    }

    [[nodiscard]] Vec2 TopLeft()     const noexcept { return {x, y}; }
    [[nodiscard]] Vec2 BottomRight() const noexcept { return {x + width, y + height}; }
    [[nodiscard]] float Right()      const noexcept { return x + width; }
    [[nodiscard]] float Bottom()     const noexcept { return y + height; }
};

struct Color {
    uint8_t r{0};
    uint8_t g{0};
    uint8_t b{0};
    uint8_t a{255};
};

// Restrições de layout para o algoritmo Box Constraints.
// minW/minH: tamanho mínimo que o widget deve ter.
// maxW/maxH: tamanho máximo disponível pelo pai.
struct BoxConstraints {
    float minW{0.0f};
    float minH{0.0f};
    float maxW{0.0f};
    float maxH{0.0f};

    // Retorna constraints que forcam um tamanho exato.
    [[nodiscard]] static BoxConstraints Tight(float w, float h) noexcept {
        return {w, h, w, h};
    }

    // Retorna constraints sem limite máximo (expansível).
    [[nodiscard]] static BoxConstraints Loose(float maxW, float maxH) noexcept {
        return {0.0f, 0.0f, maxW, maxH};
    }
};

// Pré-declaração para uso em RenderContext sem incluir raylib.h.
// O tamanho real de Font é opaco aqui — apenas RenderContext o conhece.
struct FontHandle {
    void* ptr{nullptr};  // Ponteiro opaco para ::Font da Raylib.
};

} // namespace jay::engine
