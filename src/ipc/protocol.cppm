module;
#include <cstdint>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
export module jay.ipc.protocol;

using json = nlohmann::json;

export namespace jay::ipc {

constexpr int32_t kProtocolVersion = 1;

constexpr int32_t kMsgRegisterClient = 100;
constexpr int32_t kMsgCreateChat = 200;
constexpr int32_t kMsgCreateMessage = 300;

enum class AuthorType : uint8_t {
  Registration = 1,
  Agent = 2,
  Tool = 3,
  System = 4,
};

enum class MessageRole : uint8_t {
  User = 1,
  Assistant = 2,
  System = 3,
  Tool = 4,
};

enum class MessageContentType : uint8_t {
  TextPlain = 1,
  Markdown = 2,
  JSON = 3,
  ToolCall = 4,
  ToolResult = 5,
};

// ─────────────────────────────────────────────────────────────────
// RequestEnvelope (Protocolo v1)
// ─────────────────────────────────────────────────────────────────
struct RequestEnvelope {
  int32_t protocolVersion{kProtocolVersion};
  std::string requestId;
  std::string clientId;
  int32_t type{0};
  json payload;

  [[nodiscard]] std::string Serialize() const {
    json j = {{"protocol_version", protocolVersion},
              {"request_id", requestId},
              {"client_id", clientId},
              {"type", type},
              {"payload", payload}};
    return j.dump();
  }
};

// ─────────────────────────────────────────────────────────────────
// ResponseEnvelope (Protocolo v1)
// ─────────────────────────────────────────────────────────────────
struct ResponseEnvelope {
  int32_t protocolVersion{kProtocolVersion};
  std::string requestId;
  int32_t type{0};
  int32_t status{0};
  std::optional<std::string> error;
  json payload;

  [[nodiscard]] static std::optional<ResponseEnvelope> Deserialize(const std::string& rawJson) {
    try {
      auto j = json::parse(rawJson);
      ResponseEnvelope env;
      if (j.contains("protocol_version")) env.protocolVersion = j["protocol_version"].get<int32_t>();
      if (j.contains("request_id")) env.requestId = j["request_id"].get<std::string>();
      if (j.contains("type")) env.type = j["type"].get<int32_t>();
      if (j.contains("status")) env.status = j["status"].get<int32_t>();
      if (j.contains("error") && !j["error"].is_null()) {
        env.error = j["error"].get<std::string>();
      }
      if (j.contains("payload")) env.payload = j["payload"];
      return env;
    } catch (...) {
      return std::nullopt;
    }
  }
};

}  // namespace jay::ipc
