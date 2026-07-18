module;
#include <string>
#include <vector>
#include <variant>
#include <mutex>
export module chat_state;

export namespace jay {

enum class ChatKind {
  User,
  Assistant,
  Error,
  ToolGroup,
  ThinkBlock,
  ToolOutput,
  ExecLog,
  ScheduleToast,
  Unknown
};

inline ChatKind ParseChatKind(const std::string& k) {
  if (k == "user")           return ChatKind::User;
  if (k == "assistant")      return ChatKind::Assistant;
  if (k == "error")          return ChatKind::Error;
  if (k == "tool_group")     return ChatKind::ToolGroup;
  if (k == "think_block")    return ChatKind::ThinkBlock;
  if (k == "tool_output")    return ChatKind::ToolOutput;
  if (k == "exec_log")       return ChatKind::ExecLog;
  if (k == "schedule_toast") return ChatKind::ScheduleToast;
  return ChatKind::Unknown;
}

struct ToolAction {
  std::string id;
  std::string name;
  bool        success;
  std::string error;
};

struct ToolGroupPayload {
  std::vector<ToolAction> actions;
};

struct ThinkBlockPayload  {};
struct ToolOutputPayload  {};
struct ExecLogPayload     {};
struct ScheduleToastPayload {};

using ChatPayload = std::variant<
  std::monostate,
  ToolGroupPayload,
  ThinkBlockPayload,
  ToolOutputPayload,
  ExecLogPayload,
  ScheduleToastPayload
>;

struct ChatMessage {
  std::string id;
  std::string sender;
  std::string text;
  long long   timestamp;
  ChatKind    kind;
  ChatPayload payload;
};

class ChatState {
public:
  ChatState() = default;

  void AddChatMessage(std::string id, std::string sender, std::string text,
                      long long timestamp, ChatKind kind,
                      ChatPayload payload = std::monostate{}) {
    std::lock_guard<std::mutex> lk(m_mutex);
    m_messages.push_back({std::move(id), std::move(sender), std::move(text),
                           timestamp, kind, std::move(payload)});
  }

  std::vector<ChatMessage> GetChatFeed() const {
    std::lock_guard<std::mutex> lk(m_mutex);
    return m_messages;
  }

  void AccumulateToolAction(ToolAction action) {
    std::lock_guard<std::mutex> lk(m_mutex);
    m_pending.push_back(std::move(action));
  }

  std::vector<ToolAction> FlushToolActions() {
    std::lock_guard<std::mutex> lk(m_mutex);
    std::vector<ToolAction> result;
    result.swap(m_pending);
    return result;
  }

private:
  mutable std::mutex       m_mutex;
  std::vector<ChatMessage> m_messages;
  std::vector<ToolAction>  m_pending;
};

} // namespace jay
