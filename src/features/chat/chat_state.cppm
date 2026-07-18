module;
#include <mutex>
#include <string>
#include <vector>
export module chat_state;

export namespace jay {

struct ChatMessage {
  std::string id;
  std::string sender; // "user", "jay"
  std::string text;
  long long timestamp;
  std::string kind;    // "user", "assistant", "system", "tool", "error"
};

class ChatState {
public:
  ChatState() = default;

  void AddChatMessage(const std::string& id, const std::string& sender, const std::string& text, long long timestamp, const std::string& kind) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_messages.push_back({id, sender, text, timestamp, kind});
  }

  std::vector<ChatMessage> GetChatFeed() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_messages;
  }

  float GetScrollOffset() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_scrollOffset;
  }

  void SetScrollOffset(float offset) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_scrollOffset = offset;
  }

private:
  mutable std::mutex m_mutex;
  std::vector<ChatMessage> m_messages;
  float m_scrollOffset = 0.0f;
};

} // namespace jay
