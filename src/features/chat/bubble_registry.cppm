module;
#include <raylib.h>
#include <functional>
#include <unordered_map>
#include <variant>
export module bubble_registry;

import chat_state;
import message_bubble;
import tool_group_bubble;
import theme;

export namespace jay {

struct BubbleRendererEntry {
  std::function<float(const ChatBubbleData&, bool expanded)> calculateHeight;
  std::function<bool(Font, const ChatBubbleData&, float scrollOffset,
                     float fontSize, float labelFontSize, Vector2 mousePos, bool expanded,
                     int selIdx, int selStart, int selEnd,
                     int copiedIdx, float copiedTimer)> draw;
};

class BubbleRegistry {
public:
  BubbleRegistry() {
    BubbleRendererEntry textEntry{
      [](const ChatBubbleData& b, bool) -> float {
        return b.lines.size() * 24.0f + 20.0f;
      },
      [](Font font, const ChatBubbleData& b, float scroll, float fs, float lfs,
         Vector2 mouse, bool, int si, int ss, int se, int ci, float ct) -> bool {
        MessageBubble::Draw(font, b, scroll, fs, lfs, mouse, si, ss, se, ci, ct);
        return false;
      }
    };
    Register(ChatKind::User,      textEntry);
    Register(ChatKind::Assistant, textEntry);
    Register(ChatKind::Error,     textEntry);

    BubbleRendererEntry toolGroupEntry{
      [](const ChatBubbleData& b, bool expanded) -> float {
        if (const auto* p = std::get_if<ToolGroupPayload>(&b.payload)) {
          return ToolGroupBubble::CalculateHeight(*p, expanded);
        }
        return ToolGroupBubble::HEADER_HEIGHT;
      },
      [](Font font, const ChatBubbleData& b, float scroll, float fs, float,
         Vector2 mouse, bool expanded, int, int, int, int, float) -> bool {
        return ToolGroupBubble::Draw(font, b, scroll, fs, mouse, expanded);
      }
    };
    Register(ChatKind::ToolGroup, toolGroupEntry);
  }

  float CalculateHeight(const ChatBubbleData& bubble, bool expanded) const {
    const auto* entry = Find(bubble.kind);
    if (!entry) return bubble.lines.size() * 24.0f + 20.0f;
    return entry->calculateHeight(bubble, expanded);
  }

  bool Draw(Font font, const ChatBubbleData& bubble, float scrollOffset,
            float fontSize, float labelFontSize, Vector2 mousePos, bool expanded,
            int selIdx, int selStart, int selEnd, int copiedIdx, float copiedTimer) const {
    const auto* entry = Find(bubble.kind);
    if (!entry) {
      MessageBubble::Draw(font, bubble, scrollOffset, fontSize, labelFontSize,
                          mousePos, selIdx, selStart, selEnd, copiedIdx, copiedTimer);
      return false;
    }
    return entry->draw(font, bubble, scrollOffset, fontSize, labelFontSize,
                       mousePos, expanded, selIdx, selStart, selEnd, copiedIdx, copiedTimer);
  }

private:
  std::unordered_map<int, BubbleRendererEntry> m_renderers;

  void Register(ChatKind kind, BubbleRendererEntry entry) {
    m_renderers[static_cast<int>(kind)] = std::move(entry);
  }

  const BubbleRendererEntry* Find(ChatKind kind) const {
    auto it = m_renderers.find(static_cast<int>(kind));
    return it != m_renderers.end() ? &it->second : nullptr;
  }
};

} // namespace jay
