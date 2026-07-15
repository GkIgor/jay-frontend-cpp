#include <iostream>
#include <memory>

import avatar;
import event_dispatcher;
import ipc_client;
import renderer;

int main() {
  std::cout << "=== Inicializando Jay Frontend (C++) ===" << std::endl;
  auto avatar = std::make_shared<jay::Avatar>();
  auto dispatcher = std::make_shared<jay::EventDispatcher>(avatar);
  auto ipcClient = std::make_shared<jay::IPCClient>(dispatcher);
  ipcClient->Start();
  jay::Renderer renderer(avatar, ipcClient);
  renderer.Run();
  ipcClient->Stop();
  return 0;
}
