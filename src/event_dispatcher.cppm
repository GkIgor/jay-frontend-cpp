module;
#include <string>
#include <memory>
#include <iostream>
#include <nlohmann/json.hpp>
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
            if (type == "avatar.state") {
                if (payload.contains("state")) {
                    if (auto stateOpt = StringToState(payload["state"])) m_avatar->SetState(*stateOpt);
                }
            } else if (type == "avatar.animation") {
                if (payload.contains("animation")) m_avatar->PlayAnimation(payload["animation"]);
            }
        } catch (...) {}
    }
private:
    std::shared_ptr<Avatar> m_avatar;
};
}
