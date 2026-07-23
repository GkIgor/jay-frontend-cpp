module;
#include <memory>
#include <vector>
#include <string>
#include <functional>
export module jay.features.chat.viewmodel;

import jay.state.state_store;
import jay.state.actions;
import jay.usecases.send_message;

export namespace jay::features::chat {

class ChatViewModel {
public:
    ChatViewModel(jay::state::StateStore& store, jay::usecases::SendMessageUseCase& sendMessageUseCase)
        : m_store(store), m_sendMessageUseCase(sendMessageUseCase) {
        m_store.Subscribe([this]() { OnStateChanged(); });
        OnStateChanged();
    }

    [[nodiscard]] const std::string& GetActiveChatId() const noexcept {
        return m_activeChatId;
    }

    [[nodiscard]] const std::vector<jay::state::MessageDTO>& GetMessages() const noexcept {
        static const std::vector<jay::state::MessageDTO> empty;
        auto* msgs = m_store.GetMessagesForChat(m_activeChatId);
        return msgs ? *msgs : empty;
    }

    void SendMessage(const std::string& text) {
        if (m_activeChatId.empty() || text.empty()) return;
        m_sendMessageUseCase.Execute(m_activeChatId, text);
    }

    void SetOnUpdateCallback(std::function<void()> callback) {
        m_onUpdate = std::move(callback);
    }

private:
    jay::state::StateStore&               m_store;
    jay::usecases::SendMessageUseCase&    m_sendMessageUseCase;
    std::string                           m_activeChatId;
    std::function<void()>                 m_onUpdate;

    void OnStateChanged() {
        std::string currentActive = m_store.GetActiveChatId();
        m_activeChatId = currentActive;
        if (m_onUpdate) m_onUpdate();
    }
};

} // namespace jay::features::chat
