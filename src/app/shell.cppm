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
import jay.shared.widgets.tab_bar;
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
        m_avatarViewModel = std::make_shared<jay::features::avatar::AvatarViewModel>(m_store);
        m_chatViewModel   = std::make_shared<jay::features::chat::ChatViewModel>(m_store, m_sendMessageUseCase);

        auto tabBarWidget = std::make_unique<jay::shared::widgets::TabBarWidget>(
            std::vector<std::string>{"AVATAR", "CHAT"}
        );
        tabBarWidget->SetOnTabChanged([this](int tabIndex) {
            OnTabChanged(tabIndex);
        });

        auto avatarWidget = std::make_unique<jay::features::avatar::AvatarWidget>(m_avatarViewModel);
        auto chatWidget   = std::make_unique<jay::features::chat::ChatWidget>(m_chatViewModel);
        auto chatInput    = std::make_unique<jay::features::chat::ChatInputWidget>(m_chatViewModel);
        auto permWidget   = std::make_unique<jay::features::permissions::PermissionsWidget>(m_store, m_approvePermissionUseCase);

        m_tabBarRef       = tabBarWidget.get();
        m_avatarWidgetRef = avatarWidget.get();
        m_chatWidgetRef   = chatWidget.get();
        m_chatInputRef    = chatInput.get();
        m_permWidgetRef   = permWidget.get();

        AddChild(std::move(tabBarWidget));
        AddChild(std::move(avatarWidget));
        AddChild(std::move(chatWidget));
        AddChild(std::move(chatInput));
        AddChild(std::move(permWidget));

        ContainerWidget::Init();
        OnTabChanged(1); // Aba CHAT como padrão
    }

    void Layout(const jay::engine::BoxConstraints& constraints) override {
        m_bounds.width  = constraints.maxW;
        m_bounds.height = constraints.maxH;

        float tabBarH = 48.0f;
        if (m_tabBarRef) {
            m_tabBarRef->Layout(jay::engine::BoxConstraints::Tight(m_bounds.width, tabBarH));
            m_tabBarRef->SetBounds(jay::engine::Rect{m_bounds.x, m_bounds.y, m_bounds.width, tabBarH});
        }

        float contentY = m_bounds.y + tabBarH;
        float contentH = m_bounds.height - tabBarH;
        int activeTab  = m_tabBarRef ? m_tabBarRef->GetActiveTab() : 1;

        if (activeTab == 0) {
            if (m_avatarWidgetRef) {
                m_avatarWidgetRef->SetVisible(true);
                m_avatarWidgetRef->Layout(jay::engine::BoxConstraints::Tight(m_bounds.width, contentH));
                m_avatarWidgetRef->SetBounds(jay::engine::Rect{m_bounds.x, contentY, m_bounds.width, contentH});
            }
            if (m_chatWidgetRef) m_chatWidgetRef->SetVisible(false);
            if (m_chatInputRef)  m_chatInputRef->SetVisible(false);
        } else {
            if (m_avatarWidgetRef) m_avatarWidgetRef->SetVisible(false);

            float inputH = 54.0f;
            float chatFeedH = contentH - inputH - 16.0f;
            if (chatFeedH < 100.0f) chatFeedH = 100.0f;

            if (m_chatWidgetRef) {
                m_chatWidgetRef->SetVisible(true);
                m_chatWidgetRef->Layout(jay::engine::BoxConstraints::Tight(m_bounds.width, chatFeedH));
                m_chatWidgetRef->SetBounds(jay::engine::Rect{m_bounds.x, contentY, m_bounds.width, chatFeedH});
            }

            if (m_chatInputRef) {
                m_chatInputRef->SetVisible(true);
                float inputY = contentY + chatFeedH + 8.0f;
                m_chatInputRef->Layout(jay::engine::BoxConstraints::Tight(m_bounds.width - 32.0f, inputH));
                m_chatInputRef->SetBounds(jay::engine::Rect{m_bounds.x + 16.0f, inputY, m_bounds.width - 32.0f, inputH});
            }
        }

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

    jay::shared::widgets::TabBarWidget* m_tabBarRef{nullptr};
    jay::engine::Widget*                m_avatarWidgetRef{nullptr};
    jay::engine::Widget*                m_chatWidgetRef{nullptr};
    jay::engine::Widget*                m_chatInputRef{nullptr};
    jay::engine::Widget*                m_permWidgetRef{nullptr};

    void OnTabChanged(int /*tabIndex*/) {
        MarkLayoutDirty();
    }
};

} // namespace jay::app
