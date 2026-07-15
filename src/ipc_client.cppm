module;
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
export module ipc_client;

import event_dispatcher;

export namespace jay {
class IPCClient {
public:
  explicit IPCClient(std::shared_ptr<EventDispatcher> dispatcher)
    : m_dispatcher(std::move(dispatcher)), m_running(false) {
    const char* xdg = std::getenv("XDG_RUNTIME_DIR");
    if (xdg)
      m_socketPath = std::string(xdg) + "/jay/jay.sock";
    else {
      const char* home = std::getenv("HOME");
      if (home)
        m_socketPath = std::string(home) + "/.jay/jay.sock";
      else
        m_socketPath = "/tmp/jay/jay.sock";
    }
  }
  ~IPCClient() { Stop(); }
  void Start() {
    if (m_running) return;
    m_running = true;
    m_thread = std::thread(&IPCClient::RunLoop, this);
  }
  void Stop() {
    m_running = false;
    if (m_thread.joinable()) m_thread.join();
  }
  void SendMessage(const std::string& message) {
    int sock = m_socket;
    if (sock != -1) {
      std::string raw = message + "\n";
      write(sock, raw.c_str(), raw.size());
    }
  }

private:
  void RunLoop() {
    char buffer[4096];
    std::string messageBuffer;
    while (m_running) {
      int sock = ConnectSocket();
      if (sock == -1) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        continue;
      }
      m_socket = sock;
      std::cout << "IPCClient: Conectado ao Core!\n";
      while (m_running) {
        ssize_t bytesRead = read(sock, buffer, sizeof(buffer) - 1);
        if (bytesRead > 0) {
          buffer[bytesRead] = '\0';
          messageBuffer += buffer;
          size_t pos;
          while ((pos = messageBuffer.find('\n')) != std::string::npos) {
            std::string msg = messageBuffer.substr(0, pos);
            messageBuffer.erase(0, pos + 1);
            if (!msg.empty()) m_dispatcher->Dispatch(msg);
          }
        } else
          break;
      }
      m_socket = -1;
      close(sock);
      std::this_thread::sleep_for(std::chrono::seconds(2));
    }
  }
  int ConnectSocket() {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) return -1;
    struct sockaddr_un addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, m_socketPath.c_str(), sizeof(addr.sun_path) - 1);
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
      close(sock);
      return -1;
    }
    return sock;
  }
  std::shared_ptr<EventDispatcher> m_dispatcher;
  std::atomic<bool> m_running;
  std::atomic<int> m_socket{-1};
  std::thread m_thread;
  std::string m_socketPath;
};
}  // namespace jay
