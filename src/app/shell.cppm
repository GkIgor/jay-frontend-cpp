module;
#include <memory>
#include <vector>
#include <string>
export module jay.app.shell;

import jay.engine.types;
import jay.engine.render_context;
import jay.engine.widget;
import jay.shared.widgets.panel;
import jay.shared.widgets.label;
import jay.shared.widgets.button;
import jay.state.state_store;
import jay.usecases.send_message;
import jay.usecases.create_chat;
import jay.usecases.approve_permission;
import jay.features.avatar.viewmodel;
import jay.features.avatar.widget;
import jay.features.chat.viewmodel;
import jay.features.chat.widget;
import jay.features.chat.input_widget;
import jay.features.permissions.widget;
import ipc_client;

export namespace jay::app {

// ─────────────────────────────────────────────────────────────────
// Shell — Root Layout & Window Manager
// ─────────────────────────────────────────────────────────────────
class Shell : public jay::engine::ContainerWidget {
public:
    Shell(jay::state::StateStore& store,
          jay::IPCClient& ipc,
          jay::usecases::SendMessageUseCase& sendMessageUseCase,
          jay::usecases::CreateChatUseCase& createChatUseCase,
          jay::usecases::ApprovePermissionUseCase& approvePermissionUseCase)
        : m_store(store),
          m_ipc(ipc),
          m_sendMessageUseCase(sendMessageUseCase),
          m_createChatUseCase(createChatUseCase),
          m_approvePermissionUseCase(approvePermissionUseCase) {}

    void Init() override {
        // 1. Instancia os ViewModels das features
        m_avatarViewModel = std::make_shared<jay::features::avatar::AvatarViewModel>(m_store);
        m_chatViewModel   = std::make_shared<jay::features::chat::ChatViewModel>(m_store, m_sendMessageUseCase);

        // 2. Instancia os Widgets visuais principais
        auto avatarWidget = std::make_unique<jay::features::avatar::AvatarWidget>(m_avatarViewModel);
        auto chatWidget   = std::make_unique<jay::features::chat::ChatWidget>(m_chatViewModel);
        auto chatInput    = std::make_unique<jay::features::chat::ChatInputWidget>(m_chatViewModel);
        auto permWidget   = std::make_unique<jay::features::permissions::PermissionsWidget>(m_store, m_approvePermissionUseCase);

        m_avatarWidgetRef = avatarWidget.get();
        m_chatWidgetRef   = chatWidget.get();
        m_chatInputRef    = chatInput.get();
        m_permWidgetRef   = permWidget.get();

        // 3. Adiciona na subárvore de filhos
        AddChild(std::move(avatarWidget));
        AddChild(std::move(chatWidget));
        AddChild(std::move(chatInput));
        AddChild(std::move(permWidget)); // Modal no topo

        ContainerWidget::Init();
    }

    void Layout(const jay::engine::BoxConstraints& constraints) override {
        m_bounds.width  = constraints.maxW;
        m_bounds.height = constraints.maxH;

        float sideW = m_bounds.width * 0.35f;
        if (sideW < 300.0f) sideW = 300.0f;
        float mainW = m_bounds.width - sideW;

        // Posiciona o Avatar no lado esquerdo (35% da tela)
        if (m_avatarWidgetRef) {
            m_avatarWidgetRef->Layout(jay::engine::BoxConstraints::Tight(sideW, m_bounds.height));
            m_avatarWidgetRef->SetBounds(jay::engine::Rect{m_bounds.x, m_bounds.y, sideW, m_bounds.height});
        }

        // Posiciona o Chat no lado direito (65% da tela)
        float inputH = 54.0f;
        float chatFeedH = m_bounds.height - inputH - 16.0f;

        if (m_chatWidgetRef) {
            m_chatWidgetRef->Layout(jay::engine::BoxConstraints::Tight(mainW, chatFeedH));
            m_chatWidgetRef->SetBounds(jay::engine::Rect{m_bounds.x + sideW, m_bounds.y, mainW, chatFeedH});
        }

        if (m_chatInputRef) {
            m_chatInputRef->Layout(jay::engine::BoxConstraints::Tight(mainW - 32.0f, inputH));
            m_chatInputRef->SetBounds(jay::engine::Rect{m_bounds.x + sideW + 16.0f, m_bounds.y + chatFeedH + 8.0f, mainW - 32.0f, inputH});
        }

        // Modal de Permissões sobre toda a janela
        if (m_permWidgetRef) {
            m_permWidgetRef->Layout(jay::engine::BoxConstraints::Tight(m_bounds.width, m_bounds.height));
            m_permWidgetRef->SetBounds(m_bounds);
        }

        m_layoutDirty = false;
    }

private:
    jay::state::StateStore&                          m_store;
    jay::IPCClient&                                 m_ipc;
    jay::usecases::SendMessageUseCase&             m_sendMessageUseCase;
    jay::usecases::CreateChatUseCase&              m_createChatUseCase;
    jay::usecases::ApprovePermissionUseCase&          m_approvePermissionUseCase;

    std::shared_ptr<jay::features::avatar::AvatarViewModel> m_avatarViewModel;
    std::shared_ptr<jay::features::chat::ChatViewModel>     m_chatViewModel;

    jay::engine::Widget* m_avatarWidgetRef{nullptr};
    jay::engine::Widget* m_chatWidgetRef{nullptr};
    jay::engine::Widget* m_chatInputRef{nullptr};
    jay::engine::Widget* m_permWidgetRef{nullptr};
};

} // namespace jay::app
