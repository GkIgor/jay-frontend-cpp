module;
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
export module event_dispatcher;

import avatar;

using json = nlohmann::json;

export namespace jay {
class EventDispatcher {
public:
  explicit EventDispatcher(std::shared_ptr<Avatar> avatar) : m_avatar(std::move(avatar)) {}
  void Dispatch(const std::string& jsonMessage) {
    try {
      auto payload = json::parse(jsonMessage);
      if (!payload.contains("type") || !payload["type"].is_string()) return;
      std::string type = payload["type"];
      if (type == "state.changed") {
        if (payload.contains("payload") && payload["payload"].contains("state")) {
          if (auto stateOpt = StringToState(payload["payload"]["state"])) m_avatar->SetState(*stateOpt);
        }
      } else if (type == "animation.play") {
        if (payload.contains("payload") && payload["payload"].contains("animation"))
          m_avatar->PlayAnimation(payload["payload"]["animation"]);
      }
    } catch (...) {
    }
  }

private:
  std::shared_ptr<Avatar> m_avatar;
};
}  // namespace jay
