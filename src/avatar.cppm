module;
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <unordered_map>
export module avatar;

export namespace jay {

enum class State { Idle, Thinking, Sleeping, Executing };

std::optional<State> StringToState(const std::string& stateStr) {
  static const std::unordered_map<std::string, State> stateMap = {{"idle", State::Idle},
                                                                  {"thinking", State::Thinking},
                                                                  {"sleeping", State::Sleeping},
                                                                  {"executing", State::Executing}};
  auto it = stateMap.find(stateStr);
  if (it != stateMap.end()) return it->second;
  return std::nullopt;
}

class Avatar {
public:
  Avatar() : m_state(State::Idle) {}
  void SetState(State state) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_state = state;
  }
  State GetState() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_state;
  }
  void PlayAnimation(const std::string& animationName) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_animations.push(animationName);
  }
  std::optional<std::string> ConsumeNextAnimation() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_animations.empty()) return std::nullopt;
    std::string anim = m_animations.front();
    m_animations.pop();
    return anim;
  }
  bool IsPromptingPermission() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_isPromptingPermission;
  }
  std::string GetPendingPrompt() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_pendingPrompt;
  }
  std::string GetPendingRefId() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_pendingRefId;
  }
  void PromptPermission(const std::string& prompt, const std::string& refId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_isPromptingPermission = true;
    m_pendingPrompt = prompt;
    m_pendingRefId = refId;
  }
  void ClearPermissionPrompt() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_isPromptingPermission = false;
    m_pendingPrompt = "";
    m_pendingRefId = "";
  }

private:
  mutable std::mutex m_mutex;
  State m_state;
  std::queue<std::string> m_animations;
  bool m_isPromptingPermission = false;
  std::string m_pendingPrompt;
  std::string m_pendingRefId;
};
}  // namespace jay
