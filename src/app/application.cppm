module;
#include <raylib.h>
#include <memory>
#include <string>
#include <vector>
#include <iostream>

export module application;

import main_window;
import jay.engine.types;
import jay.engine.render_context;
import jay.engine.layout_engine;
import jay.engine.input_dispatcher;
import jay.engine.focus_manager;
import jay.state.state_store;
import jay.usecases.send_message;
import jay.usecases.create_chat;
import jay.usecases.approve_permission;
import jay.app.shell;
import ipc_client;
import event_dispatcher;
import renderer;
import app_state;

export namespace jay {

// ─────────────────────────────────────────────────────────────────
// Application — Composition Root & Master Game Loop
//
// Invariantes Arquiteturais:
//   1. Composition Root: Possui via std::unique_ptr a StateStore,
//      IPCClient, RenderContext, Shell e UseCases.
//   2. Master Game Loop: 5 Fases Determinísticas por Frame (60 FPS):
//        Phase 1: Poll Inputs (Raylib -> InputDispatcher)
//        Phase 2: Drain IPC Queue (IPC Thread -> Main Thread Inbound)
//        Phase 3: Update & Relayout (dt, Tweens, BoxConstraints)
//        Phase 4: Render (BeginDrawing -> RenderContext -> EndDrawing)
//        Phase 5: End Frame
// ─────────────────────────────────────────────────────────────────
class Application {
public:
    Application() {
        // ── Composition Root: Instanciação com Ownership Único ─────
        m_stateStore     = std::make_unique<jay::state::StateStore>();
        
        // Compatibilidade com legado para EventDispatcher existente
        m_legacyState    = std::make_shared<ApplicationState>();
        m_dispatcher     = std::make_shared<EventDispatcher>(m_legacyState);
        m_ipcClient      = std::make_shared<IPCClient>(m_dispatcher);

        m_renderContext  = std::make_unique<jay::engine::RenderContext>();
        m_shell          = std::make_unique<jay::app::Shell>(*m_stateStore, *m_ipcClient);

        // Instancia os Use Cases
        m_sendMessageUseCase   = std::make_unique<jay::usecases::SendMessageUseCase>(*m_stateStore, *m_ipcClient);
        m_createChatUseCase    = std::make_unique<jay::usecases::CreateChatUseCase>(*m_stateStore, *m_ipcClient);
        m_approvePermissionUseCase = std::make_unique<jay::usecases::ApprovePermissionUseCase>(*m_stateStore, *m_ipcClient);

        // Renderer legado de transição
        m_legacyRenderer = std::make_unique<Renderer>(m_legacyState, m_ipcClient);
    }

    ~Application() = default;

    void Run(const std::string& fontPath) {
        // Inicializa a janela da Raylib (1200x850)
        MainWindow::Init(1200, 850, "Jay UI — GUI Engine C++20");

        // Carrega a fonte no RenderContext (fora do Hot Loop)
        std::vector<int> codepoints;
        for (int i = 32; i < 256; ++i) codepoints.push_back(i);
        m_renderContext->LoadFont(fontPath.c_str(), 32, codepoints.data(), static_cast<int>(codepoints.size()));

        // Inicializa o Renderer legado
        m_legacyRenderer->Init(fontPath);

        // Inicializa o Shell
        m_shell->Init();

        // Inicia a thread assíncrona do IPC
        m_ipcClient->Start();

        // ── Master Game Loop Determinístico (60 FPS) ───────────────
        while (!MainWindow::ShouldClose()) {
            float dt = GetFrameTime();

            // ── Phase 1: Poll Inputs ──────────────────────────────
            m_focusManager.ProcessTabNavigation();
            jay::engine::InputDispatcher::Poll(*m_shell, m_focusManager.GetFocused());

            // ── Phase 2: Drain IPC Queue ──────────────────────────
            m_ipcClient->PollInbound();

            // ── Phase 3: Update & Relayout ────────────────────────
            m_shell->Update(dt);
            jay::engine::BoxConstraints windowConstraints = jay::engine::BoxConstraints::Tight(
                m_renderContext->ScreenWidth(),
                m_renderContext->ScreenHeight()
            );
            jay::engine::LayoutEngine::Resolve(*m_shell, windowConstraints);

            // ── Phase 4: Render ───────────────────────────────────
            m_legacyRenderer->Draw();
        }

        // ── Encerrando a aplicação ────────────────────────────────
        m_ipcClient->Stop();
        m_legacyRenderer->CleanUp();
        MainWindow::Close();
    }

private:
    // Ownership Único (Composition Root)
    std::unique_ptr<jay::state::StateStore>             m_stateStore;
    std::unique_ptr<jay::engine::RenderContext>         m_renderContext;
    std::unique_ptr<jay::app::Shell>                    m_shell;
    jay::engine::FocusManager                           m_focusManager;

    // Use Cases (Owned por Application)
    std::unique_ptr<jay::usecases::SendMessageUseCase>   m_sendMessageUseCase;
    std::unique_ptr<jay::usecases::CreateChatUseCase>    m_createChatUseCase;
    std::unique_ptr<jay::usecases::ApprovePermissionUseCase> m_approvePermissionUseCase;

    // Componentes compartilhados / legados de transição
    std::shared_ptr<ApplicationState>                   m_legacyState;
    std::shared_ptr<EventDispatcher>                    m_dispatcher;
    std::shared_ptr<IPCClient>                          m_ipcClient;
    std::unique_ptr<Renderer>                           m_legacyRenderer;
};

} // namespace jay
