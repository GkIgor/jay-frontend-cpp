module;
#include <raylib.h>
#include <memory>
#include <string>
export module application;

import main_window;
import app_state;
import ipc_client;
import event_dispatcher;
import renderer;

export namespace jay {

class Application {
public:
  Application() {
    m_state = std::make_shared<ApplicationState>();
    m_dispatcher = std::make_shared<EventDispatcher>(m_state);
    m_ipcClient = std::make_shared<IPCClient>(m_dispatcher);
    m_renderer = std::make_unique<Renderer>(m_state, m_ipcClient);
  }

  void Run(const std::string& fontPath) {
    // Inicializa a janela principal do Raylib (resolução de 1200x850)
    MainWindow::Init(1200, 850, "Jay UI - Core Integration");
    
    m_renderer->Init(fontPath);
    m_ipcClient->Start();

    while (!MainWindow::ShouldClose()) {
      m_renderer->Draw();
    }

    m_ipcClient->Stop();
    m_renderer->CleanUp();
    MainWindow::Close();
  }

private:
  std::shared_ptr<ApplicationState> m_state;
  std::shared_ptr<EventDispatcher> m_dispatcher;
  std::shared_ptr<IPCClient> m_ipcClient;
  std::unique_ptr<Renderer> m_renderer;
};

} // namespace jay
