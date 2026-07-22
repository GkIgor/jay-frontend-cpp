module;
#include <vector>
#include <unordered_map>
#include <string>
#include <functional>
#include <optional>
#include <variant>
#include <cassert>
export module jay.state.state_store;

import jay.state.actions;

export namespace jay::state {

// ─────────────────────────────────────────────────────────────────
// StateStore — Container Passivo e Fonte Única de Verdade da Aplicação
//
// Invariantes Arquiteturais (Review 3):
//   1. Passivo: Não conhece IPCClient, sockets ou dependências de rede.
//   2. Sem Lógica de Negócio: Apenas armazena, muta via Reducers puramente
//      funcionais e notifica observadores síncronamente.
//   3. Sem Referências Visuais: Nunca armazena ponteiros para Widgets ou UI.
//   4. Execução Síncrona: Dispatch(Action) roda na Main Thread de forma
//      ordenada. Dispatches aninhados são proibidos (assert/guard).
// ─────────────────────────────────────────────────────────────────
class StateStore {
public:
    StateStore() {
        m_subscribers.reserve(16);
    }

    ~StateStore() = default;

    // Não copiável nem movível — StateStore tem identidade única.
    StateStore(const StateStore&)            = delete;
    StateStore& operator=(const StateStore&) = delete;

    // ── Leituras de Estado (Getters Imutáveis / Views) ────────────

    [[nodiscard]] ConnectionState GetConnectionState() const noexcept {
        return m_connectionState;
    }

    [[nodiscard]] const std::string& GetActiveChatId() const noexcept {
        return m_activeChatId;
    }

    [[nodiscard]] const std::vector<ChatDTO>& GetChats() const noexcept {
        return m_chats;
    }

    [[nodiscard]] const std::vector<MessageDTO>* GetMessagesForChat(const std::string& chatId) const noexcept {
        auto it = m_messagesByChat.find(chatId);
        if (it != m_messagesByChat.end()) {
            return &it->second;
        }
        return nullptr;
    }

    [[nodiscard]] const std::vector<ToolDTO>& GetTools() const noexcept {
        return m_tools;
    }

    [[nodiscard]] const std::optional<PermissionRequestDTO>& GetActivePermissionRequest() const noexcept {
        return m_activePermissionRequest;
    }

    // ── Mutação Determinística via Dispatch(Action) ───────────────

    // Despacha uma Ação imutável para a StateStore.
    // Síncrono, executado na Main Thread. Proibido durante outro Dispatch.
    void Dispatch(const Action& action) {
        assert(!m_isDispatching && "Dispatch aninhado é proibido!");
        m_isDispatching = true;

        std::visit([this](const auto& act) {
            ApplyReducer(act);
        }, action);

        m_isDispatching = false;

        NotifySubscribers();
    }

    // Registra um observador síncrono para ser notificado a cada Dispatch.
    void Subscribe(std::function<void()> subscriber) {
        m_subscribers.push_back(std::move(subscriber));
    }

private:
    ConnectionState                                        m_connectionState{ConnectionState::Disconnected};
    std::string                                            m_activeChatId;
    std::vector<ChatDTO>                                   m_chats;
    std::unordered_map<std::string, std::vector<MessageDTO>> m_messagesByChat;
    std::vector<ToolDTO>                                   m_tools;
    std::optional<PermissionRequestDTO>                    m_activePermissionRequest;

    std::vector<std::function<void()>>                     m_subscribers;
    bool                                                   m_isDispatching{false};

    void NotifySubscribers() {
        for (const auto& sub : m_subscribers) {
            if (sub) sub();
        }
    }

    // ── Reducers Puramente Funcionais (Sem efeitos colaterais) ───

    void ApplyReducer(const SetConnectionStateAction& act) {
        m_connectionState = act.state;
    }

    void ApplyReducer(const SetActiveChatAction& act) {
        m_activeChatId = act.chatId;
    }

    void ApplyReducer(const SetChatsAction& act) {
        m_chats = act.chats;
    }

    void ApplyReducer(const AddChatAction& act) {
        m_chats.push_back(act.chat);
    }

    void ApplyReducer(const SetMessagesAction& act) {
        m_messagesByChat[act.chatId] = act.messages;
    }

    void ApplyReducer(const AddMessageAction& act) {
        m_messagesByChat[act.message.chatId].push_back(act.message);
    }

    void ApplyReducer(const UpdateMessageStatusAction& act) {
        for (auto& [chatId, msgs] : m_messagesByChat) {
            for (auto& msg : msgs) {
                if (msg.id == act.messageId) {
                    msg.status = act.status;
                    return;
                }
            }
        }
    }

    void ApplyReducer(const SetToolsAction& act) {
        m_tools = act.tools;
    }

    void ApplyReducer(const SetPermissionPromptAction& act) {
        m_activePermissionRequest = act.request;
    }
};

} // namespace jay::state
