module;
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <nlohmann/json.hpp>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <functional>
#include <random>
#include <sstream>
#include <iomanip>

export module ipc_client;

import event_dispatcher;
import jay.ipc.protocol;

export namespace jay {

using ResponseCallback = std::function<void(const jay::ipc::ResponseEnvelope&)>;

class IPCClient {
public:
    explicit IPCClient(std::shared_ptr<EventDispatcher> dispatcher)
        : m_dispatcher(std::move(dispatcher)), m_running(false) {
        const char* xdg = std::getenv("XDG_RUNTIME_DIR");
        if (xdg) {
            m_socketPath = std::string(xdg) + "/jay/jay.sock";
        } else {
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

    // ── Envio Assíncrono RPC (Protocolo v1) ──────────────────────
    std::string SendRPC(int32_t type, const nlohmann::json& payload, ResponseCallback callback = nullptr) {
        std::string reqId = GenerateUUID();
        jay::ipc::RequestEnvelope env;
        env.protocolVersion = jay::ipc::kProtocolVersion;
        env.requestId       = reqId;
        env.clientId        = "client_cpp_gui";
        env.type            = type;
        env.payload         = payload;

        if (callback) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_pendingCallbacks[reqId] = std::move(callback);
        }

        std::string serialized = env.Serialize();
        SendMessage(serialized);
        return reqId;
    }

    // Método de legado para envio de raw strings
    void SendMessage(const std::string& message) {
        int sock = m_socket;
        if (sock != -1) {
            std::string raw = message + "\n";
            write(sock, raw.c_str(), raw.size());
        }
    }

    // ── Processamento na Main Thread (Fase 2 do Game Loop) ───────
    // Drena a fila de respostas recebidas do Socket e executa os callbacks
    // com total segurança de thread na Main Thread (60 FPS).
    void PollInbound() {
        std::vector<std::pair<ResponseCallback, jay::ipc::ResponseEnvelope>> batch;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            batch.swap(m_inboundQueue);
        }
        for (const auto& [cb, resp] : batch) {
            if (cb) cb(resp);
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
            std::cout << "IPCClient: Conectado ao Core (Protocolo v1)!\n";
            while (m_running) {
                ssize_t bytesRead = read(sock, buffer, sizeof(buffer) - 1);
                if (bytesRead > 0) {
                    buffer[bytesRead] = '\0';
                    messageBuffer += buffer;
                    size_t pos;
                    while ((pos = messageBuffer.find('\n')) != std::string::npos) {
                        std::string msg = messageBuffer.substr(0, pos);
                        messageBuffer.erase(0, pos + 1);
                        if (!msg.empty()) HandleRawFrame(msg);
                    }
                } else {
                    break;
                }
            }
            m_socket = -1;
            close(sock);
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }

    void HandleRawFrame(const std::string& msg) {
        // Tenta desserializar como ResponseEnvelope v1
        auto respOpt = jay::ipc::ResponseEnvelope::Deserialize(msg);
        if (respOpt.has_value()) {
            const auto& resp = *respOpt;
            ResponseCallback cb = nullptr;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                auto it = m_pendingCallbacks.find(resp.requestId);
                if (it != m_pendingCallbacks.end()) {
                    cb = std::move(it->second);
                    m_pendingCallbacks.erase(it);
                }
            }
            if (cb) {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_inboundQueue.push_back({std::move(cb), resp});
                return;
            }
        }

        // Se não era resposta RPC com callback pendente, envia ao EventDispatcher tradicional
        if (m_dispatcher) {
            m_dispatcher->Dispatch(msg);
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

    static std::string GenerateUUID() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(0, 15);
        std::stringstream ss;
        for (int i = 0; i < 32; ++i) {
            if (i == 8 || i == 12 || i == 16 || i == 20) ss << "-";
            ss << std::hex << dis(gen);
        }
        return ss.str();
    }

    std::shared_ptr<EventDispatcher> m_dispatcher;
    std::atomic<bool> m_running;
    std::atomic<int> m_socket{-1};
    std::thread m_thread;
    std::string m_socketPath;

    std::mutex m_mutex;
    std::unordered_map<std::string, ResponseCallback> m_pendingCallbacks;
    std::vector<std::pair<ResponseCallback, jay::ipc::ResponseEnvelope>> m_inboundQueue;
};

} // namespace jay
