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
        // Inicializa o painel do modal e os botões de ação
        m_modalPanel = std::make_unique<jay::shared::widgets::Panel>(
            jay::engine::Color{22, 27, 34, 245},  // Glassmorphism escuro
            jay::engine::Color{56, 139, 253, 255}, // Bordas azuis de destaque
            12.0f
        );

        m_titleLabel = std::make_unique<jay::shared::widgets::Label>(
            "SOLICITAÇÃO DE PERMISSÃO", 16.0f, jay::engine::Color{210, 153, 34, 255}
        );

        m_promptLabel = std::make_unique<jay::shared::widgets::Label>(
            "", 14.0f, jay::engine::Color{201, 209, 217, 255}
        );

        m_allowButton = std::make_unique<jay::shared::widgets::Button>("PERMITIR", [this]() {
            if (m_activeRequest) {
                m_approveUseCase.Execute(m_activeRequest->refId, true);
            }
        });

        m_denyButton = std::make_unique<jay::shared::widgets::Button>("NEGAR", [this]() {
            if (m_activeRequest) {
                m_approveUseCase.Execute(m_activeRequest->refId, false);
            }
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

        // Calcula posição centralizada do modal
        float modalW = (m_bounds.width * 0.5f > 420.0f) ? 420.0f : m_bounds.width * 0.8f;
        float modalH = 180.0f;
        float modalX = (m_bounds.width - modalW) * 0.5f;
        float modalY = (m_bounds.height - modalH) * 0.5f;

        m_modalBounds = jay::engine::Rect{modalX, modalY, modalW, modalH};
        m_modalPanel->Layout(jay::engine::BoxConstraints::Tight(modalW, modalH));
        m_modalPanel->SetBounds(m_modalBounds);

        // Posiciona os elementos internos do modal
        float btnW = 120.0f;
        float btnH = 36.0f;
        float btnY = modalY + modalH - btnH - 16.0f;

        m_allowButton->Layout(jay::engine::BoxConstraints::Tight(btnW, btnH));
        m_allowButton->SetBounds(jay::engine::Rect{modalX + modalW - btnW * 2.0f - 24.0f, btnY, btnW, btnH});

        m_denyButton->Layout(jay::engine::BoxConstraints::Tight(btnW, btnH));
        m_denyButton->SetBounds(jay::engine::Rect{modalX + modalW - btnW - 16.0f, btnY, btnW, btnH});

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

        // Overlay escuro de fundo cobrindo toda a janela
        ctx.DrawRect(m_bounds, jay::engine::Color{0, 0, 0, 160});

        // Desenha o painel central do modal
        m_modalPanel->Render(ctx);

        // Desenha o título e o prompt
        ctx.DrawText("SOLICITAÇÃO DE PERMISSÃO", {m_modalBounds.x + 16.0f, m_modalBounds.y + 16.0f}, 16.0f, jay::engine::Color{210, 153, 34, 255});
        ctx.DrawText(m_activeRequest->prompt, {m_modalBounds.x + 16.0f, m_modalBounds.y + 48.0f}, 14.0f, jay::engine::Color{201, 209, 217, 255});

        // Desenha os botões de ação
        m_allowButton->Render(ctx);
        m_denyButton->Render(ctx);
    }

    bool OnEvent(const jay::engine::InputEvent& event) override {
        if (!m_activeRequest.has_value()) return false;

        // Se o modal estiver visível, consome todos os eventos para bloquear a tela de fundo
        if (m_allowButton->OnEvent(event)) return true;
        if (m_denyButton->OnEvent(event)) return true;

        event.handled = true;
        return true; // Bloqueia propagação para widgets inferiores
    }

private:
    jay::state::StateStore&                          m_store;
    jay::usecases::ApprovePermissionUseCase&         m_approveUseCase;
    std::optional<jay::state::PermissionRequestDTO> m_activeRequest;

    std::unique_ptr<jay::shared::widgets::Panel>  m_modalPanel;
    std::unique_ptr<jay::shared::widgets::Label>  m_titleLabel;
    std::unique_ptr<jay::shared::widgets::Label>  m_promptLabel;
    std::unique_ptr<jay::shared::widgets::Button> m_allowButton;
    std::unique_ptr<jay::shared::widgets::Button> m_denyButton;
    jay::engine::Rect                            m_modalBounds;

    void OnStateChanged() {
        auto req = m_store.GetActivePermissionRequest();
        if (req != m_activeRequest) {
            m_activeRequest = req;
            MarkLayoutDirty();
        }
    }
};

} // namespace jay::features::permissions
