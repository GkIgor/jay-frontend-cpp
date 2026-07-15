# Objetivos do Frontend (Goals)

O que buscamos alcançar com o desenvolvimento do C++ Frontend:

1. **Multiplataforma Simples**: Rodar eficientemente em Linux (Desktop principal), Windows e macOS, aproveitando a versatilidade multiplataforma e portabilidade da biblioteca Raylib.
2. **Tempo Real**: Prover uma UI rápida e responsiva que exiba o avatar, legendas das falas, botões de ação e modais de notificação solicitados pelo Core instantaneamente.
3. **Distribuição Simplificada**: Compilar o frontend como um executável pequeno e rápido, fácil de distribuir e rodar no sistema do usuário, contendo dependências embarcadas ou lincadas estaticamente quando necessário (ex: nlohmann/json e Raylib via FetchContent).
4. **Comunicação Confiável**: Estabelecer um cliente JSON-IPC robusto em C++ via UNIX sockets que consiga despachar eventos livre de deadlocks e threads bloqueadas.
