module;
#include <chrono>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
export module event_dispatcher;

import app_state;
import avatar_state;

using json = nlohmann::json;

export namespace jay {

/**
 * EventDispatcher
 * 
 * TODO/Evolução Futura:
 * Atualmente o Dispatcher centraliza o parsing de eventos via um switch de strings (if-else).
 * Conforme novas features forem criadas, esta classe poderá evoluir para um padrão Pub/Sub,
 * onde cada Feature (como ChatFeature ou PermissionFeature) assina dinamicamente os tipos de eventos
 * que lhe interessam:
 * 
 * ex:
 *   m_dispatcher->Subscribe("response", [](const json& payload) { ... });
 */
class EventDispatcher {
public:
  explicit EventDispatcher(std::shared_ptr<ApplicationState> state) : m_state(std::move(state)) {}

  void Dispatch(const std::string& jsonMessage) {
    try {
      auto payload = json::parse(jsonMessage);
      if (!payload.contains("type") || !payload["type"].is_string()) return;
      std::string type = payload["type"];

      if (type == "state.changed") {
        if (payload.contains("payload") && payload["payload"].contains("state")) {
          if (auto stateOpt = StringToState(payload["payload"]["state"])) {
            m_state->avatar.SetState(*stateOpt);
          }
        }
      } else if (type == "animation.play") {
        if (payload.contains("payload") && payload["payload"].contains("animation")) {
          m_state->avatar.PlayAnimation(payload["payload"]["animation"]);
        }
      } else if (type == "request.permission") {
        if (payload.contains("payload") && payload["payload"].contains("ref_id")) {
          std::string prompt = payload["payload"].contains("prompt")
                                 ? payload["payload"]["prompt"].get<std::string>()
                                 : payload["payload"]["permission"].get<std::string>();
          m_state->permission.PromptPermission(prompt, payload["payload"]["ref_id"]);
        }
      } else if (type == "response") {
        if (payload.contains("payload") && payload["payload"].contains("data")) {
          std::string refId = payload["payload"].contains("ref_id") ? payload["payload"]["ref_id"].get<std::string>() : "";
          std::string status = payload["payload"].contains("status") ? payload["payload"]["status"].get<std::string>() : "ok";
          std::string text = payload["payload"]["data"].get<std::string>();
          std::string kind = (status == "error") ? "error" : "assistant";
          
          long long timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::system_clock::now().time_since_epoch()).count();
          m_state->chat.AddChatMessage(refId, "jay", text, timestamp, kind);
        }
      } else if (type == "tool.completed") {
        if (payload.contains("payload")) {
          std::string tool = payload["payload"]["tool"].get<std::string>();
          bool success = payload["payload"]["success"].get<bool>();
          std::string statusStr = success ? "sucesso" : "falha";
          std::string text = "Ação [" + tool + "] concluída com " + statusStr;
          if (!success && payload["payload"].contains("error") && payload["payload"]["error"].is_string()) {
            text += ": " + payload["payload"]["error"].get<std::string>();
          }
          long long timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::system_clock::now().time_since_epoch()).count();
          m_state->chat.AddChatMessage("", "system", text, timestamp, "tool");
        }
      }
    } catch (...) {
    }
  }

private:
  std::shared_ptr<ApplicationState> m_state;
};

} // namespace jay
