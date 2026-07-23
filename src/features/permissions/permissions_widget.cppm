module;
#include <memory>
#include <string>
#include <optional>
export module jay.features.permissions.widget;

import jay.engine.types;
import jay.engine.render_context;
import jay.engine.widget;
import jay.shared.widgets.panel;
import jay.shared.widgets.label;
import jay.shared.widgets.button;
import jay.state.actions;
import jay.state.state_store;
import jay.usecases.approve_permission;

export namespace jay::features::permissions {

class PermissionsWidget : public jay::engine::Widget {
public:
    PermissionsWidget(jay::state::StateStore& store, jay::usecases::ApprovePermissionUseCase& approveUseCase)
        : m_store(store), m_approveUseCase(approveUseCase) {
        m_store.Subscribe([this]() { OnStateChanged(); });
        OnStateChanged();
    }

    void Init() override {
        m_modalPanel = std::make_unique<jay::shared::widgets::Panel>(
            jay::engine::Color{22, 27, 34, 245},
            jay::engine::Color{56, 139, 253, 255},
            12.0f
        );

        m_allowButton = std::make_unique<jay::shared::widgets::Button>("Permitir [Y]", [this]() {
            RespondPermission(true, "click");
        });

        m_denyButton = std::make_unique<jay::shared::widgets::Button>("Negar [N]", [this]() {
            RespondPermission(false, "click");
        });

        m_modalPanel->Init();
        m_allowButton->Init();
        m_denyButton->Init();
    }

    void Layout(const jay::engine::BoxConstraints& constraints) override {
        m_bounds.width  = constraints.maxW;
        m_bounds.height = constraints.maxH;

        if (!m_activeRequest.has_value()) {
            m_layoutDirty = false;
            return;
        }

        float modalW = (m_bounds.width * 0.5f > 460.0f) ? 460.0f : m_bounds.width * 0.85f;
        float modalH = 170.0f;
        float modalX = (m_bounds.width - modalW) * 0.5f;
        float modalY = (m_bounds.height - modalH) * 0.5f;

        m_modalBounds = jay::engine::Rect{modalX, modalY, modalW, modalH};
        m_modalPanel->Layout(jay::engine::BoxConstraints::Tight(modalW, modalH));
        m_modalPanel->SetBounds(m_modalBounds);

        float btnW = 160.0f;
        float btnH = 36.0f;
        float btnY = modalY + modalH - btnH - 16.0f;

        m_allowButton->Layout(jay::engine::BoxConstraints::Tight(btnW, btnH));
        m_allowButton->SetBounds(jay::engine::Rect{modalX + 30.0f, btnY, btnW, btnH});

        m_denyButton->Layout(jay::engine::BoxConstraints::Tight(btnW, btnH));
        m_denyButton->SetBounds(jay::engine::Rect{modalX + modalW - btnW - 30.0f, btnY, btnW, btnH});

        m_layoutDirty = false;
    }

    void Update(float deltaTime) override {
        if (!m_activeRequest.has_value()) return;

        m_modalPanel->Update(deltaTime);
        m_allowButton->Update(deltaTime);
        m_denyButton->Update(deltaTime);
    }

    void Render(jay::engine::RenderContext& ctx) const override {
        if (!m_activeRequest.has_value()) return;

        ctx.DrawRect(m_bounds, jay::engine::Color{0, 0, 0, 160});

        m_modalPanel->Render(ctx);

        ctx.DrawText("Solicitação de Permissão", {m_modalBounds.x + 20.0f, m_modalBounds.y + 20.0f}, 18.0f, jay::engine::Color{201, 209, 217, 255});
        ctx.DrawText(m_activeRequest->prompt, {m_modalBounds.x + 20.0f, m_modalBounds.y + 55.0f}, 14.0f, jay::engine::Color{139, 148, 158, 255});

        m_allowButton->Render(ctx);
        m_denyButton->Render(ctx);
    }

    bool OnEvent(const jay::engine::InputEvent& event) override {
        if (!m_activeRequest.has_value()) return false;

        if (event.kind == jay::engine::InputEventKind::KeyPress) {
            if (event.key == 'Y' || event.key == 'y') {
                RespondPermission(true, "keyboard");
                event.handled = true;
                return true;
            }
            if (event.key == 'N' || event.key == 'n') {
                RespondPermission(false, "keyboard");
                event.handled = true;
                return true;
            }
        }

        if (m_allowButton->OnEvent(event)) return true;
        if (m_denyButton->OnEvent(event)) return true;

        event.handled = true;
        return true;
    }

private:
    jay::state::StateStore&                          m_store;
    jay::usecases::ApprovePermissionUseCase&         m_approveUseCase;
    std::optional<jay::state::PermissionRequestDTO> m_activeRequest;

    std::unique_ptr<jay::shared::widgets::Panel>  m_modalPanel;
    std::unique_ptr<jay::shared::widgets::Button> m_allowButton;
    std::unique_ptr<jay::shared::widgets::Button> m_denyButton;
    jay::engine::Rect                            m_modalBounds;

    void RespondPermission(bool allow, const std::string& /*modality*/) {
        if (!m_activeRequest.has_value()) return;
        std::string refId = m_activeRequest->refId;
        m_approveUseCase.Execute(refId, allow);
        m_activeRequest = std::nullopt;
        MarkLayoutDirty();
    }

    void OnStateChanged() {
        auto req = m_store.GetActivePermissionRequest();
        if (req != m_activeRequest) {
            m_activeRequest = req;
            MarkLayoutDirty();
        }
    }
};

} // namespace jay::features::permissions
