# Visão Geral da Arquitetura C++

O Frontend em C++ opera como um consumidor e visualizador de estado (State Viewer). Ele não contém a "inteligência" de raciocínio. A arquitetura é separada nas seguintes camadas:

1. **Thread Principal (Renderização / Raylib)**
   - Inicializa a janela via `InitWindow`.
   - Lê os eventos do usuário (teclado, mouse).
   - Renderiza a cada frame (60 FPS) o estado visual do Avatar (ex: ondas sonoras, gradientes).
   - Consulta o Despachante de Eventos para verificar se há alguma atualização visual (ex: um novo texto de log, uma mudança de estado).

2. **Thread Background (Cliente IPC / I/O Sockets)**
   - Estabelece conexão com o socket UNIX (`XDG_RUNTIME_DIR/jay/jay.sock`).
   - Bloqueia e fica lendo mensagens JSON disparadas pelo Daemon do Core.
   - Quando uma mensagem chega, faz o parse via `nlohmann::json`.
   - Empurra o pacote na fila segura do Despachante de Eventos.

3. **Despachante de Eventos (Event Dispatcher)**
   - Fila thread-safe (geralmente usando `std::mutex` e `std::queue`).
   - Permite que a Thread Background enfileire atualizações sem bloquear.
   - A Thread Principal desenfileira atualizações e atualiza o estado visual sem travar a interface.
