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
import ipc_client;

export namespace jay::app {

// ─────────────────────────────────────────────────────────────────
// Shell — Root Layout & Window Manager (Presentation Layer)
//
// Invariantes Arquiteturais:
//   - Herda de ContainerWidget.
//   - Recebe referências não-proprietárias para StateStore& e IPCClient&.
//   - Não possui propriedade sobre componentes de infraestrutura.
// ─────────────────────────────────────────────────────────────────
class Shell : public jay::engine::ContainerWidget {
public:
    Shell(jay::state::StateStore& store, jay::IPCClient& ipc)
        : m_store(store), m_ipc(ipc) {}

    void Init() override {
        // Cria um painel raiz genérico para o layout do Shell
        auto rootPanel = std::make_unique<jay::shared::widgets::Panel>(
            jay::engine::Color{13, 17, 23, 255},  // Background escuro
            jay::engine::Color{48, 54, 61, 255},  // Border
            0.0f                                  // Sem arredondamento de canto no painel principal
        );

        // Adiciona um label de título no topo
        auto titleLabel = std::make_unique<jay::shared::widgets::Label>(
            "JAY DESKTOP ASSISTANT (GUI ENGINE C++20)",
            16.0f,
            jay::engine::Color{88, 166, 255, 255}
        );
        rootPanel->AddChild(std::move(titleLabel));

        AddChild(std::move(rootPanel));
    }

    void Layout(const jay::engine::BoxConstraints& constraints) override {
        ContainerWidget::Layout(constraints);
    }

private:
    jay::state::StateStore& m_store;
    jay::IPCClient&         m_ipc;
};

} // namespace jay::app
