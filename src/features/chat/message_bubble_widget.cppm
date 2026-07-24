module;
#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>
#include <vector>
export module jay.features.chat.message_bubble_widget;

import jay.engine.types;
import jay.engine.render_context;
import jay.engine.widget;
import jay.shared.widgets.layout_primitives;
import jay.state.actions;
import unicode;

export namespace jay::features::chat {

class MessageBubbleWidget : public jay::engine::Widget {
public:
  explicit MessageBubbleWidget(jay::state::MessageDTO message)
    : m_message(std::move(message)), m_copyTimer(0.0f), m_isCopyHovered(false), m_animTime(0.0f) {
    TrimMessageText();
  }

  void SetMessage(jay::state::MessageDTO message) {
    m_message = std::move(message);
    TrimMessageText();
    MarkLayoutDirty();
  }

  [[nodiscard]] const jay::state::MessageDTO& GetMessage() const noexcept { return m_message; }

  void Update(float deltaTime) override {
    m_animTime += deltaTime;
    if (m_copyTimer > 0.0f) {
      m_copyTimer -= deltaTime;
    }
  }

  void Layout(const jay::engine::BoxConstraints& constraints) override {
    float maxBubbleW = constraints.maxW * 0.70f;
    if (maxBubbleW < 220.0f) maxBubbleW = constraints.maxW;

    float fontSize = 18.0f;
    float paddingH = 22.0f;
    float paddingV = 16.0f;
    float copyBarH = 34.0f;

    float availableTextW = maxBubbleW - paddingH * 2.0f;
    if (availableTextW < 80.0f) availableTextW = 80.0f;

    float approxCharW = 10.0f;
    float textWidthEst = static_cast<float>(m_cleanContent.length()) * approxCharW;
    float lines = std::ceil(textWidthEst / availableTextW);
    if (lines < 1.0f) lines = 1.0f;

    float bubbleW = (textWidthEst < availableTextW) ? (textWidthEst + paddingH * 2.0f) : maxBubbleW;
    if (bubbleW < 200.0f) bubbleW = 200.0f;

    float textContentH = lines * (fontSize + 6.0f);
    float bubbleH = paddingV * 2.0f + textContentH + copyBarH;

    m_bounds.width = bubbleW;
    m_bounds.height = bubbleH;

    UpdateCopyBtnRect();
    m_layoutDirty = false;
  }

  void SetBounds(jay::engine::Rect bounds) noexcept override {
    jay::engine::Widget::SetBounds(bounds);
    UpdateCopyBtnRect();
  }

  void Render(jay::engine::RenderContext& ctx) const override {
    if (!m_visible) return;

    jay::engine::Color bg;
    jay::engine::Color textCol{230, 237, 243, 255};

    if (m_message.sender == "user") {
      bg = jay::engine::Color{31, 111, 235, 255};
    } else if (m_message.status == jay::state::MessageStatus::Failed) {
      bg = jay::engine::Color{94, 35, 38, 255};
    } else {
      bg = jay::engine::Color{33, 38, 45, 255};
    }

    ctx.DrawRectRounded(m_bounds, 12.0f, bg);

    float fontSize = 18.0f;
    float paddingH = 22.0f;
    float paddingV = 16.0f;

    if (m_message.status == jay::state::MessageStatus::Pending && m_message.sender != "user") {
      float cy = m_bounds.y + paddingV + 10.0f;
      float cx = m_bounds.x + paddingH + 10.0f;

      for (int i = 0; i < 3; ++i) {
        float offsetAnim = std::sin(m_animTime * 8.0f + i * 1.2f) * 4.0f;
        jay::engine::Rect dotRect{cx + i * 20.0f, cy + offsetAnim, 6.0f, 6.0f};
        ctx.DrawRectRounded(dotRect, 3.0f, jay::engine::Color{201, 209, 217, 255});
      }
    } else {
      jay::engine::Color effectiveTextCol = textCol;
      if (m_message.sender == "user" && m_message.status == jay::state::MessageStatus::Pending) {
        effectiveTextCol = jay::engine::Color{230, 237, 243, 100};
      }
      ctx.DrawText(m_cleanContent, {m_bounds.x + paddingH, m_bounds.y + paddingV}, fontSize, effectiveTextCol);
    }

    if (m_copyTimer > 0.0f) {
      ctx.DrawRectRounded(m_copyBtnRect, 6.0f, jay::engine::Color{35, 134, 54, 255});
      ctx.DrawText("Copiado", {m_copyBtnRect.x + 13.0f, m_copyBtnRect.y + 5.0f}, 13.0f,
                   jay::engine::Color{255, 255, 255, 255});
    } else {
      if (m_isCopyHovered) {
        ctx.DrawRectRounded(m_copyBtnRect, 6.0f, jay::engine::Color{56, 139, 253, 255});
      }

      const jay::engine::Color labelColor =
        m_isCopyHovered ? jay::engine::Color{255, 255, 255, 255} : jay::engine::Color{173, 186, 199, 255};
      ctx.DrawText("Copiar", {m_copyBtnRect.x + 17.0f, m_copyBtnRect.y + 5.0f}, 13.0f, labelColor);
    }
  }

  bool OnEvent(const jay::engine::InputEvent& event) override {
    if (!m_visible || !m_enabled) return false;

    if (event.kind == jay::engine::InputEventKind::MouseMove) {
      m_isCopyHovered = m_copyBtnRect.Contains({event.mouseX, event.mouseY});
    }

    if (event.kind == jay::engine::InputEventKind::MousePress && event.key == 0) {
      if (m_copyBtnRect.Contains({event.mouseX, event.mouseY})) {
        m_copyTimer = 2.0f;
        jay::engine::WriteClipboardText(m_cleanContent);
        event.handled = true;
        return true;
      }
    }

    return false;
  }

private:
  jay::state::MessageDTO m_message;
  std::string m_cleanContent;
  jay::engine::Rect m_copyBtnRect;
  float m_copyTimer;
  bool m_isCopyHovered;
  float m_animTime;

  void UpdateCopyBtnRect() {
    m_copyBtnRect =
      jay::engine::Rect{m_bounds.x + m_bounds.width - 88.0f, m_bounds.y + m_bounds.height - 32.0f, 76.0f, 24.0f};
  }

  void TrimMessageText() {
    std::string s = m_message.content;
    size_t first = s.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) {
      m_cleanContent = "";
      return;
    }
    size_t last = s.find_last_not_of(" \t\n\r");
    m_cleanContent = s.substr(first, (last - first + 1));
  }
};

}  // namespace jay::features::chat
