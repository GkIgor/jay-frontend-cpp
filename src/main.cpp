#include <iostream>
#include <memory>

import app_state;
import event_dispatcher;
import ipc_client;
import renderer;

int main() {
  std::cout << "=== Inicializando Jay Frontend (C++) ===" << std::endl;
  auto state = std::make_shared<jay::ApplicationState>();
  auto dispatcher = std::make_shared<jay::EventDispatcher>(state);
  auto ipcClient = std::make_shared<jay::IPCClient>(dispatcher);
  ipcClient->Start();
  jay::Renderer renderer(state, ipcClient);
  renderer.Run();
  ipcClient->Stop();
  return 0;
}
