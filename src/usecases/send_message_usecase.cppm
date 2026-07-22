module;
#include <string>
#include <chrono>
#include <random>
#include <sstream>
#include <iomanip>
#include <nlohmann/json.hpp>
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
        int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        // ── Passo 1: Update Otimista na StateStore ────────────────
        jay::state::MessageDTO pendingMsg{
            .id = msgId,
            .chatId = chatId,
            .sender = "user",
            .content = text,
            .timestamp = now,
            .status = jay::state::MessageStatus::Pending
        };
        m_store.Dispatch(jay::state::AddMessageAction{pendingMsg});

        // ── Passo 2: Disparo RPC MsgCreateMessage (300) ────────────
        json payload = {
            {"chat_id", chatId},
            {"role", "user"},
            {"content", text},
            {"trigger_agent", true}
        };

        m_ipc.SendRPC(300, payload, [this, msgId, chatId](const jay::ipc::ResponseEnvelope& resp) {
            if (resp.status == 200) {
                // ── Passo 3 (Sucesso): Confirma estado ─────────────
                m_store.Dispatch(jay::state::UpdateMessageStatusAction{
                    .messageId = msgId,
                    .status = jay::state::MessageStatus::Sent
                });

                // Dispara a inferência de IA MsgProcessChat (350)
                json procPayload = {
                    {"chat_id", chatId}
                };
                m_ipc.SendRPC(350, procPayload, nullptr);
            } else {
                // ── Passo 3 (Falha): Marca falha na mensagem ───────
                m_store.Dispatch(jay::state::UpdateMessageStatusAction{
                    .messageId = msgId,
                    .status = jay::state::MessageStatus::Failed
                });
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

} // namespace jay::usecases
