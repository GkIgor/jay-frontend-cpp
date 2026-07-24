module;
#include <chrono>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
export module jay.usecases.create_chat;

import jay.usecases.base;
import jay.state.actions;
import jay.state.state_store;
import jay.ipc.protocol;
import ipc_client;

using json = nlohmann::json;

export namespace jay::usecases {

class CreateChatUseCase : public BaseUseCase {
public:
  using BaseUseCase::BaseUseCase;

  void Execute(const std::string& title = "Nova Conversa") {
    json payload = {{"title", title}};

    m_ipc.SendRPC(200, payload, [this, title](const jay::ipc::ResponseEnvelope& resp) {
      std::cerr << "[CreateChatUseCase] type=200 resp status=" << resp.status << "\n";
      if (resp.status == 0 && resp.payload.contains("chat")) {
        const auto& c = resp.payload["chat"];
        std::string chatId = c.contains("id") ? c["id"].get<std::string>() : "";
        std::string createdAt = c.contains("created_at") ? c["created_at"].get<std::string>() : "";
        std::string updatedAt = c.contains("updated_at") ? c["updated_at"].get<std::string>() : "";

        jay::state::ChatDTO chat{.id = chatId, .title = title, .createdAt = createdAt, .updatedAt = updatedAt};

        m_store.Dispatch(jay::state::AddChatAction{chat});
        m_store.Dispatch(jay::state::SetActiveChatAction{chatId});
      }
    });
  }
};

}  // namespace jay::usecases
