module;
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <vector>
export module app_state;

export namespace jay {

enum class State { Idle, Thinking, Sleeping, Executing };

inline std::optional<State> StringToState(const std::string& stateStr) {
  if (stateStr == "idle") return State::Idle;
  if (stateStr == "thinking") return State::Thinking;
  if (stateStr == "sleeping") return State::Sleeping;
  if (stateStr == "executing") return State::Executing;
  return std::nullopt;
}

struct ChatMessage {
  std::string id;
  std::string sender; // "user", "jay"
  std::string text;
  long long timestamp;
  std::string kind;    // "user", "assistant", "system", "tool", "error"
};

struct ChatState {
  std::vector<ChatMessage> messages;
  float scrollOffset = 0.0f;
};

struct AvatarState {
  State state = State::Idle;
  std::queue<std::string> animations;
};

struct PermissionState {
  bool isPrompting = false;
  std::string prompt;
  std::string refId;
};

class ApplicationState {
public:
  ApplicationState() = default;

  // --- AvatarState Thread-Safe Accessors ---
  void SetState(State state) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_avatarState.state = state;
  }
  State GetState() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_avatarState.state;
  }
  void PlayAnimation(const std::string& anim) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_avatarState.animations.push(anim);
  }
  std::optional<std::string> ConsumeNextAnimation() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_avatarState.animations.empty()) return std::nullopt;
    std::string anim = m_avatarState.animations.front();
    m_avatarState.animations.pop();
    return anim;
  }

  // --- PermissionState Thread-Safe Accessors ---
  bool IsPromptingPermission() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_permissionState.isPrompting;
  }
  std::string GetPendingPrompt() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_permissionState.prompt;
  }
  std::string GetPendingRefId() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_permissionState.refId;
  }
  void PromptPermission(const std::string& prompt, const std::string& refId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_permissionState.isPrompting = true;
    m_permissionState.prompt = prompt;
    m_permissionState.refId = refId;
  }
  void ClearPermissionPrompt() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_permissionState.isPrompting = false;
    m_permissionState.prompt = "";
    m_permissionState.refId = "";
  }

  // --- ChatState Thread-Safe Accessors ---
  void AddChatMessage(const std::string& id, const std::string& sender, const std::string& text, long long timestamp, const std::string& kind) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_chatState.messages.push_back({id, sender, text, timestamp, kind});
  }
  std::vector<ChatMessage> GetChatFeed() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_chatState.messages;
  }
  float GetScrollOffset() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_chatState.scrollOffset;
  }
  void SetScrollOffset(float offset) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_chatState.scrollOffset = offset;
  }

private:
  mutable std::mutex m_mutex;
  AvatarState m_avatarState;
  PermissionState m_permissionState;
  ChatState m_chatState;
};

} // namespace jay
