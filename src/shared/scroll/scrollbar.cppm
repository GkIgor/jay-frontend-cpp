module;
#include <raylib.h>
#include <algorithm>
export module scrollbar;

import theme;

export namespace jay {

struct ScrollbarStyle {
  float width = Theme::ScrollbarWidth;
  float minThumbHeight = Theme::ScrollbarMinThumb;
  float marginV = 4.0f;
  Color trackColor = GetColor(0x1F293733);
  Color thumbColor = Theme::Glow;
  Color thumbHoverColor = Theme::Glow;
};

class Scrollbar {
public:
  explicit Scrollbar(ScrollbarStyle style = {}) : m_style(style) {}

  // Retorna true quando deve ser exibida (content > visible)
  bool IsVisible(float contentHeight, float visibleHeight) const {
    return contentHeight > visibleHeight;
  }

  // Largura ocupada (para descontar do TextLayout maxWidth)
  float GetWidth() const { return m_style.width; }

  // Retorna o rect do track (para Page Up/Page Down e hit-testing futuro)
  Rectangle GetTrackRect(Rectangle bounds) const {
    return {
      bounds.x + bounds.width - m_style.width - 4.0f,
      bounds.y + m_style.marginV,
      m_style.width,
      bounds.height - m_style.marginV * 2.0f
    };
  }

  // Retorna o rect do thumb atual (para hit-testing no ScrollController)
  Rectangle GetThumbRect(Rectangle bounds, float scrollOffset, float contentHeight, float visibleHeight) const {
    Rectangle track = GetTrackRect(bounds);
    float thumbHeight = ComputeThumbHeight(track.height, contentHeight, visibleHeight);
    float maxScroll = contentHeight - visibleHeight;
    float scrollPercent = (maxScroll != 0.0f) ? (scrollOffset / maxScroll) : 0.0f;
    scrollPercent = std::clamp(scrollPercent, 0.0f, 1.0f);
    float thumbY = track.y + scrollPercent * (track.height - thumbHeight);
    return { track.x, thumbY, track.width, thumbHeight };
  }

  // Desenha track + thumb
  void Draw(Rectangle bounds, float scrollOffset, float contentHeight, float visibleHeight) const {
    if (!IsVisible(contentHeight, visibleHeight)) return;

    Rectangle track = GetTrackRect(bounds);
    DrawRectangleRounded(track, 1.0f, 4, m_style.trackColor);

    Rectangle thumb = GetThumbRect(bounds, scrollOffset, contentHeight, visibleHeight);
    Vector2 mousePos = GetMousePosition();
    bool isHover = CheckCollisionPointRec(mousePos, thumb);
    Color color = isHover ? m_style.thumbHoverColor : m_style.thumbColor;
    DrawRectangleRounded(thumb, 1.0f, 4, color);
  }

private:
  ScrollbarStyle m_style;

  float ComputeThumbHeight(float trackHeight, float contentHeight, float visibleHeight) const {
    float ratio = visibleHeight / contentHeight;
    float thumbHeight = ratio * trackHeight;
    return std::max(thumbHeight, m_style.minThumbHeight);
  }
};

} // namespace jay
