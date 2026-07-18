module;
#include <mutex>
#include <queue>
#include <string>
#include <optional>
export module avatar_state;

export namespace jay {

enum class State { Idle, Thinking, Sleeping, Executing };

inline std::optional<State> StringToState(const std::string& stateStr) {
  if (stateStr == "idle") return State::Idle;
  if (stateStr == "thinking") return State::Thinking;
  if (stateStr == "sleeping") return State::Sleeping;
  if (stateStr == "executing") return State::Executing;
  return std::nullopt;
}

class AvatarState {
public:
  AvatarState() = default;

  void SetState(State state) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_state = state;
  }

  State GetState() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_state;
  }

  void PlayAnimation(const std::string& anim) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_animations.push(anim);
  }

  std::optional<std::string> ConsumeNextAnimation() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_animations.empty()) return std::nullopt;
    std::string anim = m_animations.front();
    m_animations.pop();
    return anim;
  }

private:
  mutable std::mutex m_mutex;
  State m_state = State::Idle;
  std::queue<std::string> m_animations;
};

} // namespace jay
