module;
#include <raylib.h>

#include <memory>
#include <string>
#include <vector>

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
import jay.usecases.register_client;
import jay.usecases.approve_permission;
import jay.app.shell;
import ipc_client;
import event_dispatcher;
import app_state;

export namespace jay {

// ─────────────────────────────────────────────────────────────────
// Application — Composition Root & Master Game Loop
// ─────────────────────────────────────────────────────────────────
class Application {
public:
  Application() {
    // ── Composition Root: Instanciação com Ownership Único ─────
    m_stateStore = std::make_unique<jay::state::StateStore>();

    // Objeto de transição legado
    m_legacyState = std::make_shared<ApplicationState>();
    m_dispatcher = std::make_shared<EventDispatcher>(m_legacyState);
    m_ipcClient = std::make_shared<IPCClient>(m_dispatcher);

    m_renderContext = std::make_unique<jay::engine::RenderContext>();

    // Instancia os Use Cases
    m_registerClientUseCase = std::make_unique<jay::usecases::RegisterClientUseCase>(*m_stateStore, *m_ipcClient);
    m_sendMessageUseCase = std::make_unique<jay::usecases::SendMessageUseCase>(*m_stateStore, *m_ipcClient);
    m_createChatUseCase = std::make_unique<jay::usecases::CreateChatUseCase>(*m_stateStore, *m_ipcClient);
    m_approvePermissionUseCase = std::make_unique<jay::usecases::ApprovePermissionUseCase>(*m_stateStore, *m_ipcClient);

    // Instancia o Shell com o novo motor visual
    m_shell = std::make_unique<jay::app::Shell>(*m_stateStore, *m_ipcClient, *m_sendMessageUseCase,
                                                *m_createChatUseCase, *m_approvePermissionUseCase);
  }

  ~Application() = default;

  void Run(const std::string& fontPath) {
    // Inicializa a janela da Raylib (1200x850)
    MainWindow::Init(1200, 850, "Jay UI — GUI Engine C++20");

    // Carrega a fonte no RenderContext (fora do Hot Loop)
    std::vector<int> codepoints;
    for (int i = 32; i < 256; ++i) codepoints.push_back(i);
    m_renderContext->LoadFont(fontPath.c_str(), 32, codepoints.data(), static_cast<int>(codepoints.size()));

    // Inicializa a subárvore do Shell
    m_shell->Init();

    // Inicia a thread assíncrona do IPC
    m_ipcClient->Start();

    // Registra callback de conexão para disparar o registro somente quando o socket estiver pronto
    m_ipcClient->SetOnConnected([this]() {
      m_registerClientUseCase->Execute([this]() {
        m_createChatUseCase->Execute("Sessão Principal");
      });
    });

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
      jay::engine::BoxConstraints windowConstraints =
        jay::engine::BoxConstraints::Tight(m_renderContext->ScreenWidth(), m_renderContext->ScreenHeight());
      jay::engine::LayoutEngine::Resolve(*m_shell, windowConstraints);

      // ── Phase 4: Render ───────────────────────────────────
      BeginDrawing();
      ClearBackground(::Color{13, 17, 23, 255});
      m_shell->Render(*m_renderContext);
      EndDrawing();
    }

    // ── Encerrando a aplicação ────────────────────────────────
    m_ipcClient->Stop();
    MainWindow::Close();
  }

private:
  std::unique_ptr<jay::state::StateStore> m_stateStore;
  std::unique_ptr<jay::engine::RenderContext> m_renderContext;
  std::unique_ptr<jay::app::Shell> m_shell;
  jay::engine::FocusManager m_focusManager;

  std::unique_ptr<jay::usecases::RegisterClientUseCase> m_registerClientUseCase;
  std::unique_ptr<jay::usecases::SendMessageUseCase> m_sendMessageUseCase;
  std::unique_ptr<jay::usecases::CreateChatUseCase> m_createChatUseCase;
  std::unique_ptr<jay::usecases::ApprovePermissionUseCase> m_approvePermissionUseCase;

  std::shared_ptr<ApplicationState> m_legacyState;
  std::shared_ptr<EventDispatcher> m_dispatcher;
  std::shared_ptr<IPCClient> m_ipcClient;
};

}  // namespace jay
