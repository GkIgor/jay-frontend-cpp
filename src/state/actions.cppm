module;
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>
export module jay.state.actions;

export namespace jay::state {

// ─────────────────────────────────────────────────────────────────
// Estado de Conexão com o Daemon Unix Socket
// ─────────────────────────────────────────────────────────────────
enum class ConnectionState : uint8_t { Disconnected, Connecting, Connected, Reconnecting, Failed };

// ─────────────────────────────────────────────────────────────────
// DTOs Imutáveis de Domínio (Sem referências a Widgets ou UI)
// ─────────────────────────────────────────────────────────────────
struct ChatDTO {
  std::string id;
  std::string title;
  std::string createdAt;
  std::string updatedAt;
};

enum class MessageStatus : uint8_t { Pending, Sent, Failed };

struct MessageDTO {
  std::string id;
  std::string chatId;
  std::string sender;  // "user", "jay", "system"
  std::string content;
  int64_t timestamp{0};
  MessageStatus status{MessageStatus::Sent};
};

struct ToolDTO {
  std::string id;
  std::string name;
  std::string description;
  std::string version;
};

struct PermissionRequestDTO {
  std::string refId;
  std::string prompt;
  std::string toolName;

  bool operator==(const PermissionRequestDTO& rhs) const noexcept {
    return refId == rhs.refId && prompt == rhs.prompt && toolName == rhs.toolName;
  }
};

// ─────────────────────────────────────────────────────────────────
// Actions Imutáveis (Value Types sem membros mutáveis)
// ─────────────────────────────────────────────────────────────────

struct SetConnectionStateAction {
  ConnectionState state{ConnectionState::Disconnected};
};

struct SetActiveChatAction {
  std::string chatId;
};

struct SetChatsAction {
  std::vector<ChatDTO> chats;
};

struct AddChatAction {
  ChatDTO chat;
};

struct SetMessagesAction {
  std::string chatId;
  std::vector<MessageDTO> messages;
};

struct AddMessageAction {
  MessageDTO message;
};

struct UpdateMessageStatusAction {
  std::string messageId;
  MessageStatus status;
};

struct SetToolsAction {
  std::vector<ToolDTO> tools;
};

struct SetPermissionPromptAction {
  std::optional<PermissionRequestDTO> request;
};

// Discriminante genérico contendo qualquer Ação de Domínio
using Action =
  std::variant<SetConnectionStateAction, SetActiveChatAction, SetChatsAction, AddChatAction, SetMessagesAction,
               AddMessageAction, UpdateMessageStatusAction, SetToolsAction, SetPermissionPromptAction>;

}  // namespace jay::state
