module;
#include <string>
#include <nlohmann/json.hpp>
export module jay.usecases.approve_permission;

import jay.usecases.base;
import jay.state.actions;
import jay.state.state_store;
import jay.ipc.protocol;
import ipc_client;

using json = nlohmann::json;

export namespace jay::usecases {

class ApprovePermissionUseCase : public BaseUseCase {
public:
    using BaseUseCase::BaseUseCase;

    void Execute(const std::string& refId, bool allow) {
        json payload = {
            {"type", "permission.response"},
            {"payload", {
                {"ref_id", refId},
                {"allowed", allow}
            }}
        };

        m_ipc.SendMessage(payload.dump());
        m_store.Dispatch(jay::state::SetPermissionPromptAction{std::nullopt});
    }
};

} // namespace jay::usecases
