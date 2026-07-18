module;
#include <chrono>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
export module event_dispatcher;

import app_state;
import avatar_state;
import chat_state;

using json = nlohmann::json;

export namespace jay {

class EventDispatcher {
public:
  explicit EventDispatcher(std::shared_ptr<ApplicationState> state)
    : m_state(std::move(state)), m_actionCounter(0) {}

  void Dispatch(const std::string& jsonMessage) {
    try {
      auto payload = json::parse(jsonMessage);
      if (!payload.contains("type") || !payload["type"].is_string()) return;
      std::string type = payload["type"];

      if (type == "state.changed") {
        HandleStateChanged(payload);
      } else if (type == "animation.play") {
        HandleAnimationPlay(payload);
      } else if (type == "request.permission") {
        HandleRequestPermission(payload);
      } else if (type == "response") {
        HandleResponse(payload);
      } else if (type == "tool.completed") {
        HandleToolCompleted(payload);
      }
    } catch (...) {
    }
  }

private:
  std::shared_ptr<ApplicationState> m_state;
  int m_actionCounter;

  void HandleStateChanged(const json& payload) {
    if (payload.contains("payload") && payload["payload"].contains("state")) {
      if (auto stateOpt = StringToState(payload["payload"]["state"])) {
        m_state->avatar.SetState(*stateOpt);
      }
    }
  }

  void HandleAnimationPlay(const json& payload) {
    if (payload.contains("payload") && payload["payload"].contains("animation")) {
      m_state->avatar.PlayAnimation(payload["payload"]["animation"]);
    }
  }

  void HandleRequestPermission(const json& payload) {
    if (payload.contains("payload") && payload["payload"].contains("ref_id")) {
      std::string prompt = payload["payload"].contains("prompt")
                             ? payload["payload"]["prompt"].get<std::string>()
                             : payload["payload"]["permission"].get<std::string>();
      m_state->permission.PromptPermission(prompt, payload["payload"]["ref_id"]);
    }
  }

  void HandleResponse(const json& payload) {
    if (!payload.contains("payload") || !payload["payload"].contains("data")) return;

    std::string status = payload["payload"].contains("status")
                           ? payload["payload"]["status"].get<std::string>() : "ok";
    std::string text   = payload["payload"]["data"].get<std::string>();
    std::string refId  = payload["payload"].contains("ref_id")
                           ? payload["payload"]["ref_id"].get<std::string>() : "";

    long long ts = nowMs();

    std::vector<ToolAction> actions = m_state->chat.FlushToolActions();
    if (!actions.empty()) {
      std::string groupId = "tool-group-" + std::to_string(ts);
      m_state->chat.AddChatMessage(
        groupId, "system", "", ts - 1, ChatKind::ToolGroup,
        ToolGroupPayload{std::move(actions)}
      );
    }

    ChatKind kind = (status == "error") ? ChatKind::Error : ChatKind::Assistant;
    m_state->chat.AddChatMessage(refId, "jay", text, ts, kind);
  }

  void HandleToolCompleted(const json& payload) {
    if (!payload.contains("payload")) return;
    const auto& p = payload["payload"];

    std::string name    = p.contains("tool") ? p["tool"].get<std::string>() : "?";
    bool        success = p.contains("success") ? p["success"].get<bool>() : false;
    std::string error   = (!success && p.contains("error") && p["error"].is_string())
                            ? p["error"].get<std::string>() : "";

    std::string id = name + "-" + std::to_string(++m_actionCounter);
    m_state->chat.AccumulateToolAction({std::move(id), std::move(name), success, std::move(error)});
  }

  static long long nowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();
  }
};

} // namespace jay
