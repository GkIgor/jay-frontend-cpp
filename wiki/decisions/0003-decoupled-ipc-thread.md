# ADR 0003: Thread de IPC Desacoplada e Fila de Mensagens

**Status:** Aceito
**Data:** 14 de Julho de 2026

## Contexto
As funções do Raylib exigem que seu loop (`WindowShouldClose()`, `BeginDrawing()`) seja chamado de forma incessante, tipicamente travado pela taxa de atualização da tela (vsync ou 60 FPS fixos). Se uma chamada ao socket do Core (`read()`) ficar bloqueada aguardando resposta, a janela visual congela (UI Freeze), o que é inaceitável.

## Decisão
Adotamos uma arquitetura de thread em segundo plano (Worker) dedicada **apenas** a gerenciar o IPC. 
- Ela estabelece a conexão UNIX socket e fica bloqueada lendo pacotes.
- Ao obter um pacote completo, usa Mutex/ConditionVariables (`std::queue`, `std::mutex`) para empurrar as informações (ex: Novo texto recebido) a um *Event Dispatcher*.
- A Thread Principal do Raylib desenfileira estes eventos em cada frame.

## Consequências
- **Positivo**: O loop do Raylib (60 FPS) jamais será bloqueado pelo Core, mesmo que o Daemon do Core trave, caia ou o socket demore 5 segundos para transmitir os dados. Interface ultra responsiva e isolada.
- **Negativo**: A complexidade estrutural sobe um degrau, precisando gerenciar alocação concorrente e evitando condition variables que vazem referências no encerramento (exit routine) do programa.
