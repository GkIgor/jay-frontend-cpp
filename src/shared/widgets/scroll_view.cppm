module;
#include <raylib.h>
export module scroll_view;

import theme;

export namespace jay {

class ScrollView {
public:
  static void DrawScrollbar(float scrollOffset, float contentHeight, float visibleHeight, float tabHeight, int screenWidth) {
    if (contentHeight <= visibleHeight) return;

    Rectangle track = { screenWidth - 12.0f, tabHeight + 4.0f, 6.0f, visibleHeight - 8.0f };
    DrawRectangleRounded(track, 1.0f, 4, GetColor(0x1F293755));

    float thumbHeight = (visibleHeight / contentHeight) * track.height;
    if (thumbHeight < 30.0f) thumbHeight = 30.0f;

    float maxScroll = visibleHeight - contentHeight;
    float scrollPercent = (maxScroll != 0.0f) ? (scrollOffset / maxScroll) : 0.0f;
    float thumbY = track.y + scrollPercent * (track.height - thumbHeight);

    Rectangle thumb = { track.x, thumbY, track.width, thumbHeight };
    bool isHovered = CheckCollisionPointRec(GetMousePosition(), thumb);

    DrawRectangleRounded(thumb, 1.0f, 4, isHovered ? Theme::Glow : Theme::Border);
  }
};

} // namespace jay
