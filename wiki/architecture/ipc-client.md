# Cliente IPC e Polling

O Frontend escuta o socket UNIX para receber o estado do Core de forma desacoplada.
Como chamadas de sistema baseadas em socket (como `recv()` em C/C++) podem ser síncronas/bloqueantes caso não tratadas, isolamos tudo em uma *Background Worker Thread* (Thread de I/O em Segundo Plano).

## Funcionamento

1. O cliente Socket (ex: `struct sockaddr_un`) tenta conectar no daemon local.
2. Se a conexão falhar ou cair, ele dorme (`std::this_thread::sleep_for`) por 1 a 2 segundos e tenta novamente. O frontend continua renderizando em 60 FPS o estado `Offline` ou `Disconnected` enquanto isso.
3. Se conectado, a thread faz loop lendo blocos JSON. Ao ler um JSON completo (geralmente até encontrar o final do pacote ou `\n` delimitado pelo SDK Go do Core), submete o pacote deserializado via fila thread-safe.

## Protocolo JSON e Deserialização
O cliente confia na biblioteca `nlohmann::json` para ler as strings e extrair o campo `method` ou `event` com sua payload de dados. Essencial garantir o bloco de `try...catch` nas desserializações para evitar crashes.
