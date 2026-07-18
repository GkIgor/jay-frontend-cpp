module;
#include <mutex>
#include <string>
export module permission_state;

export namespace jay {

class PermissionState {
public:
  PermissionState() = default;

  bool IsPromptingPermission() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_isPrompting;
  }

  std::string GetPendingPrompt() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_prompt;
  }

  std::string GetPendingRefId() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_refId;
  }

  void PromptPermission(const std::string& prompt, const std::string& refId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_isPrompting = true;
    m_prompt = prompt;
    m_refId = refId;
  }

  void ClearPermissionPrompt() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_isPrompting = false;
    m_prompt = "";
    m_refId = "";
  }

private:
  mutable std::mutex m_mutex;
  bool m_isPrompting = false;
  std::string m_prompt;
  std::string m_refId;
};

} // namespace jay
