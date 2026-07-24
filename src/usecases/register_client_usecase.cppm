module;
#include <functional>
#include <nlohmann/json.hpp>
#include <string>
export module jay.usecases.register_client;

import jay.usecases.base;
import jay.state.actions;
import jay.state.state_store;
import jay.ipc.protocol;
import ipc_client;

using json = nlohmann::json;

export namespace jay::usecases {

class RegisterClientUseCase : public BaseUseCase {
public:
  using BaseUseCase::BaseUseCase;

  void Execute(std::function<void()> onSuccess = nullptr) {
    json payload = {{"client_id", kFrontendClientID}};

    m_ipc.SendRPC(
      static_cast<int32_t>(jay::ipc::kMsgRegisterClient), payload,
      [this, onSuccess = std::move(onSuccess)](const jay::ipc::ResponseEnvelope& resp) {
        if (resp.status == 0) {
          m_store.Dispatch(jay::state::SetConnectionStateAction{.state = jay::state::ConnectionState::Connected});
          if (onSuccess) onSuccess();
        }
      });
  }

private:
  static constexpr const char* kFrontendClientID = "client_cpp_gui";
};

}  // namespace jay::usecases
