import os

with open('src/avatar.cppm', 'w') as f:
    f.write('''module;
#include <string>
#include <mutex>
#include <queue>
#include <optional>
#include <unordered_map>
export module avatar;

export namespace jay {

enum class State { Idle, Thinking, Sleeping, Executing };

std::optional<State> StringToState(const std::string& stateStr) {
    static const std::unordered_map<std::string, State> stateMap = {
        {"idle", State::Idle}, {"thinking", State::Thinking},
        {"sleeping", State::Sleeping}, {"executing", State::Executing}
    };
    auto it = stateMap.find(stateStr);
    if (it != stateMap.end()) return it->second;
    return std::nullopt;
}

class Avatar {
public:
    Avatar() : m_state(State::Idle) {}
    void SetState(State state) { std::lock_guard<std::mutex> lock(m_mutex); m_state = state; }
    State GetState() const { std::lock_guard<std::mutex> lock(m_mutex); return m_state; }
    void PlayAnimation(const std::string& animationName) { std::lock_guard<std::mutex> lock(m_mutex); m_animations.push(animationName); }
    std::optional<std::string> ConsumeNextAnimation() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_animations.empty()) return std::nullopt;
        std::string anim = m_animations.front();
        m_animations.pop();
        return anim;
    }
private:
    mutable std::mutex m_mutex;
    State m_state;
    std::queue<std::string> m_animations;
};
}
''')

with open('src/event_dispatcher.cppm', 'w') as f:
    f.write('''module;
#include <string>
#include <memory>
#include <iostream>
#include <nlohmann/json.hpp>
export module event_dispatcher;

import avatar;

using json = nlohmann::json;

export namespace jay {
class EventDispatcher {
public:
    explicit EventDispatcher(std::shared_ptr<Avatar> avatar) : m_avatar(std::move(avatar)) {}
    void Dispatch(const std::string& jsonMessage) {
        try {
            auto payload = json::parse(jsonMessage);
            if (!payload.contains("type") || !payload["type"].is_string()) return;
            std::string type = payload["type"];
            if (type == "avatar.state") {
                if (payload.contains("state")) {
                    if (auto stateOpt = StringToState(payload["state"])) m_avatar->SetState(*stateOpt);
                }
            } else if (type == "avatar.animation") {
                if (payload.contains("animation")) m_avatar->PlayAnimation(payload["animation"]);
            }
        } catch (...) {}
    }
private:
    std::shared_ptr<Avatar> m_avatar;
};
}
''')

with open('src/ipc_client.cppm', 'w') as f:
    f.write('''module;
#include <string>
#include <thread>
#include <atomic>
#include <memory>
#include <iostream>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
export module ipc_client;

import event_dispatcher;

export namespace jay {
class IPCClient {
public:
    explicit IPCClient(std::shared_ptr<EventDispatcher> dispatcher) : m_dispatcher(std::move(dispatcher)), m_running(false) {
        const char* xdg = std::getenv("XDG_RUNTIME_DIR");
        if (xdg) m_socketPath = std::string(xdg) + "/jay/jay.sock";
        else {
            const char* home = std::getenv("HOME");
            if (home) m_socketPath = std::string(home) + "/.jay/jay.sock";
            else m_socketPath = "/tmp/jay/jay.sock";
        }
    }
    ~IPCClient() { Stop(); }
    void Start() { if (m_running) return; m_running = true; m_thread = std::thread(&IPCClient::RunLoop, this); }
    void Stop() { m_running = false; if (m_thread.joinable()) m_thread.join(); }
private:
    void RunLoop() {
        char buffer[4096]; std::string messageBuffer;
        while (m_running) {
            int sock = ConnectSocket();
            if (sock == -1) { std::this_thread::sleep_for(std::chrono::seconds(2)); continue; }
            std::cout << "IPCClient: Conectado ao Core!\\n";
            while (m_running) {
                ssize_t bytesRead = read(sock, buffer, sizeof(buffer) - 1);
                if (bytesRead > 0) {
                    buffer[bytesRead] = '\\0'; messageBuffer += buffer;
                    size_t pos;
                    while ((pos = messageBuffer.find('\\n')) != std::string::npos) {
                        std::string msg = messageBuffer.substr(0, pos); messageBuffer.erase(0, pos + 1);
                        if (!msg.empty()) m_dispatcher->Dispatch(msg);
                    }
                } else break;
            }
            close(sock); std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }
    int ConnectSocket() {
        int sock = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sock < 0) return -1;
        struct sockaddr_un addr; std::memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX; std::strncpy(addr.sun_path, m_socketPath.c_str(), sizeof(addr.sun_path) - 1);
        if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) { close(sock); return -1; }
        return sock;
    }
    std::shared_ptr<EventDispatcher> m_dispatcher;
    std::atomic<bool> m_running;
    std::thread m_thread;
    std::string m_socketPath;
};
}
''')

with open('src/renderer.cppm', 'w') as f:
    f.write('''module;
#include <memory>
#include <iostream>
#include <raylib.h>
export module renderer;

import avatar;

export namespace jay {
class Renderer {
public:
    explicit Renderer(std::shared_ptr<Avatar> avatar) : m_avatar(std::move(avatar)) {}
    ~Renderer() { if (IsWindowReady()) CloseWindow(); }
    void Run() {
        InitWindow(400, 400, "Jay Frontend");
        SetTargetFPS(60);
        while (!WindowShouldClose()) {
            BeginDrawing(); ClearBackground(RAYWHITE);
            DrawAvatarState();
            EndDrawing();
        }
    }
private:
    std::shared_ptr<Avatar> m_avatar;
    void DrawAvatarState() {
        State currentState = m_avatar->GetState();
        const int cx = GetScreenWidth() / 2, cy = GetScreenHeight() / 2;
        switch (currentState) {
            case State::Idle: DrawCircle(cx, cy, 50.0f, LIGHTGRAY); DrawText("Jay (Idle)", cx - 40, cy + 70, 20, DARKGRAY); break;
            case State::Thinking: DrawCircle(cx, cy, 50.0f, ORANGE); DrawText("Jay (Thinking...)", cx - 70, cy + 70, 20, DARKGRAY); break;
            case State::Executing: DrawCircle(cx, cy, 50.0f, GREEN); DrawText("Jay (Executing)", cx - 60, cy + 70, 20, DARKGRAY); break;
            case State::Sleeping: DrawCircle(cx, cy, 50.0f, DARKBLUE); DrawText("Jay (Sleeping)", cx - 60, cy + 70, 20, LIGHTGRAY); break;
        }
        if (auto anim = m_avatar->ConsumeNextAnimation()) std::cout << "Anim: " << *anim << "\\n";
    }
};
}
''')

with open('src/main.cpp', 'w') as f:
    f.write('''#include <memory>
#include <iostream>

import avatar;
import event_dispatcher;
import ipc_client;
import renderer;

int main() {
    std::cout << "=== Inicializando Jay Frontend ===" << std::endl;
    auto avatar = std::make_shared<jay::Avatar>();
    auto dispatcher = std::make_shared<jay::EventDispatcher>(avatar);
    jay::IPCClient ipcClient(dispatcher);
    ipcClient.Start();
    jay::Renderer renderer(avatar);
    renderer.Run();
    ipcClient.Stop();
    return 0;
}
''')

with open('CMakeLists.txt', 'w') as f:
    f.write('''cmake_minimum_required(VERSION 3.28)
project(jay-frontend-cpp CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(FetchContent)
FetchContent_Declare(nlohmann_json GIT_REPOSITORY https://github.com/nlohmann/json.git GIT_TAG v3.11.3)
set(JSON_BuildTests OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(nlohmann_json)
FetchContent_Declare(raylib GIT_REPOSITORY https://github.com/raysan5/raylib.git GIT_TAG 5.0)
set(BUILD_EXAMPLES OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(raylib)
add_executable(jay-frontend src/main.cpp)
target_sources(jay-frontend
  PUBLIC
    FILE_SET CXX_MODULES FILES
    src/avatar.cppm
    src/event_dispatcher.cppm
    src/ipc_client.cppm
    src/renderer.cppm
)
target_link_libraries(jay-frontend PRIVATE raylib nlohmann_json::nlohmann_json pthread)
''')
