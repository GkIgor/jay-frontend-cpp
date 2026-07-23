module;
#include <memory>
#include <functional>
export module jay.features.avatar.viewmodel;

import jay.state.state_store;
import jay.state.actions;
import jay.engine.types;

export namespace jay::features::avatar {

class AvatarViewModel {
public:
    explicit AvatarViewModel(jay::state::StateStore& store)
        : m_store(store) {
        // Registra observador síncrono para notificação de alterações no estado
        m_store.Subscribe([this]() { OnStateChanged(); });
        OnStateChanged();
    }

    [[nodiscard]] jay::state::ConnectionState GetConnectionState() const noexcept {
        return m_connectionState;
    }

    [[nodiscard]] jay::engine::Color GetTargetColor() const noexcept {
        switch (m_connectionState) {
            case jay::state::ConnectionState::Connected:
                return jay::engine::Color{48, 161, 78, 255};    // Verde Success (Execução / Ativo)
            case jay::state::ConnectionState::Connecting:
            case jay::state::ConnectionState::Reconnecting:
                return jay::engine::Color{210, 153, 34, 255};   // Amarelo Warning (Conectando)
            case jay::state::ConnectionState::Failed:
            case jay::state::ConnectionState::Disconnected:
            default:
                return jay::engine::Color{88, 166, 255, 255};   // Azul Primary (Idle / Aguardando)
        }
    }

    void SetOnUpdateCallback(std::function<void()> callback) {
        m_onUpdate = std::move(callback);
    }

private:
    jay::state::StateStore&          m_store;
    jay::state::ConnectionState     m_connectionState{jay::state::ConnectionState::Disconnected};
    std::function<void()>           m_onUpdate;

    void OnStateChanged() {
        auto newState = m_store.GetConnectionState();
        if (newState != m_connectionState) {
            m_connectionState = newState;
            if (m_onUpdate) m_onUpdate();
        }
    }
};

} // namespace jay::features::avatar
