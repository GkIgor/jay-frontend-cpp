module;
#include <chrono>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>
#include <random>
#include <sstream>
#include <string>
export module jay.usecases.send_message;

import jay.usecases.base;
import jay.state.actions;
import jay.state.state_store;
import jay.ipc.protocol;
import ipc_client;

using json = nlohmann::json;

export namespace jay::usecases {

class SendMessageUseCase : public BaseUseCase {
public:
  using BaseUseCase::BaseUseCase;

  void Execute(const std::string& chatId, const std::string& text) {
    if (text.empty()) return;

    std::string msgId = GenerateUUID();
    int64_t now =
      std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();

    // ── Passo 1: Update Otimista na StateStore ────────────────
    jay::state::MessageDTO pendingMsg{.id = msgId,
                                      .chatId = chatId,
                                      .sender = "user",
                                      .content = text,
                                      .timestamp = now,
                                      .status = jay::state::MessageStatus::Pending};
    m_store.Dispatch(jay::state::AddMessageAction{pendingMsg});

    // ── Passo 2: Disparo RPC MsgCreateMessage (300) ────────────
    json payload = {{"chat_id", chatId},
                    {"author_type", static_cast<uint8_t>(jay::ipc::AuthorType::Registration)},
                    {"role", static_cast<uint8_t>(jay::ipc::MessageRole::User)},
                    {"content", text},
                    {"content_type", static_cast<uint8_t>(jay::ipc::MessageContentType::TextPlain)},
                    {"trigger_agent", true}};

    m_ipc.SendRPC(300, payload, [this, msgId, chatId, now](const jay::ipc::ResponseEnvelope& resp) {
      std::cerr << "[SendMessageUseCase] type=300 resp status=" << resp.status << " msgId=" << msgId << "\n";
      if (resp.status == 0) {
        m_store.Dispatch(
          jay::state::UpdateMessageStatusAction{.messageId = msgId, .status = jay::state::MessageStatus::Sent});

        std::string thinkingId = GenerateUUID();
        jay::state::MessageDTO thinkingMsg{.id = thinkingId,
                                           .chatId = chatId,
                                           .sender = "jay",
                                           .content = "",
                                           .timestamp = now,
                                           .status = jay::state::MessageStatus::Pending};
        m_store.Dispatch(jay::state::AddMessageAction{thinkingMsg});

        json procPayload = {{"chat_id", chatId}};
        m_ipc.SendRPC(350, procPayload, [this, chatId, thinkingId](const jay::ipc::ResponseEnvelope& resp) {
          std::cerr << "[SendMessageUseCase] type=350 resp status=" << resp.status << "\n";

          m_store.Dispatch(
            jay::state::UpdateMessageStatusAction{.messageId = thinkingId, .status = jay::state::MessageStatus::Sent});

          if (resp.status == 0 && resp.payload.contains("processed_message")) {
            const auto& m = resp.payload["processed_message"];
            std::string msgId = m.contains("id") ? m["id"].get<std::string>() : "";
            std::string content = m.contains("content") ? m["content"].get<std::string>() : "";

            jay::state::MessageDTO dto{.id = msgId,
                                       .chatId = chatId,
                                       .sender = "jay",
                                       .content = content,
                                       .status = jay::state::MessageStatus::Sent};
            m_store.Dispatch(jay::state::AddMessageAction{dto});

            std::cerr << "[SendMessageUseCase] type=350 resposta IA: " << content.substr(0, 80) << "\n";
          } else {
            jay::state::MessageDTO errDto{.id = GenerateUUID(),
                                          .chatId = chatId,
                                          .sender = "system",
                                          .content = "Erro ao processar a resposta.",
                                          .status = jay::state::MessageStatus::Failed};
            m_store.Dispatch(jay::state::AddMessageAction{errDto});
          }
        });
      } else {
        // ── Passo 3 (Falha): Marca falha na mensagem ───────
        m_store.Dispatch(
          jay::state::UpdateMessageStatusAction{.messageId = msgId, .status = jay::state::MessageStatus::Failed});
      }
    });
  }

private:
  static std::string GenerateUUID() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    std::stringstream ss;
    for (int i = 0; i < 32; ++i) {
      if (i == 8 || i == 12 || i == 16 || i == 20) ss << "-";
      ss << std::hex << dis(gen);
    }
    return ss.str();
  }
};

}  // namespace jay::usecases
