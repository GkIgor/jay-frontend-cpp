module;
#include <raylib.h>
#include <algorithm>
export module scroll_controller;

export namespace jay {

class ScrollController {
public:
  explicit ScrollController(float wheelSensitivity = 120.0f)
      : m_offset(0.0f), m_wheelSensitivity(wheelSensitivity),
        m_isDragging(false), m_dragStartY(0.0f) {}

  // Aplica delta da roda do mouse (wheel > 0 = scroll para cima)
  void ApplyWheel(float wheelDelta) {
    m_offset += wheelDelta * m_wheelSensitivity;
  }

  // Inicia drag se o mousePos está sobre o thumb. Retorna true se iniciou.
  bool TryBeginDrag(Vector2 mousePos, Rectangle thumbRect) {
    if (CheckCollisionPointRec(mousePos, thumbRect)) {
      m_isDragging = true;
      m_dragStartY = mousePos.y - thumbRect.y;
      return true;
    }
    return false;
  }

  // Atualiza durante drag ativo
  void UpdateDrag(float mouseY, float trackY, float trackH, float thumbH, float maxScroll) {
    if (!m_isDragging) return;
    float targetThumbY = mouseY - m_dragStartY;
    float range = trackH - thumbH;
    if (range <= 0.0f) return;
    float percent = (targetThumbY - trackY) / range;
    percent = std::clamp(percent, 0.0f, 1.0f);
    m_offset = percent * maxScroll;
  }

  // Finaliza drag se botão foi solto
  void EndDragIfReleased() {
    if (m_isDragging && !IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
      m_isDragging = false;
    }
  }

  // Scroll por teclado (Page Up, Page Down, Home, End)
  // pageSize: pixels visíveis na viewport; maxScroll: offset máximo absoluto
  void ScrollByKeyboard(float delta) {
    m_offset += delta;
  }

  // Garante offset dentro de [minScroll, maxScroll]
  void Clamp(float minScroll, float maxScroll) {
    m_offset = std::clamp(m_offset, minScroll, maxScroll);
  }

  float GetOffset() const { return m_offset; }
  void SetOffset(float offset) { m_offset = offset; }
  bool IsDragging() const { return m_isDragging; }

private:
  float m_offset;
  float m_wheelSensitivity;
  bool m_isDragging;
  float m_dragStartY;
};

} // namespace jay
