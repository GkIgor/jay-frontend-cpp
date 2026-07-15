# Índice de Documentação - Jay Frontend C++

Este é o índice vivo da documentação do Frontend (C++).

Para protocolos e formatos de comunicação IPC globais, consulte a [Wiki do Core](../../jay-ia/wiki/index.md).

## 1. Visão (Vision)
Diretrizes e restrições do frontend visual:
- [`vision/principles.md`](vision/principles.md): Princípios de desenvolvimento visual (performance, separação).
- [`vision/goals.md`](vision/goals.md): Objetivos do Frontend.
- [`vision/non-goals.md`](vision/non-goals.md): O que o Frontend C++ definitivamente não é.

## 2. Arquitetura (Architecture)
Detalhes internos de como o código C++ é estruturado:
- [`architecture/overview.md`](architecture/overview.md): Arquitetura geral do frontend.
- [`architecture/ipc-client.md`](architecture/ipc-client.md): Thread de background e leitura do socket UNIX.
- [`architecture/state-machine.md`](architecture/state-machine.md): Máquina de estados visual do avatar (Idle, Listening, Thinking, Speaking).

## 3. Desenvolvimento (Development)
Como configurar e contribuir com código:
- [`development/project-structure.md`](development/project-structure.md): Estrutura de pastas, submódulos e CMake.
- [`development/build-instructions.md`](development/build-instructions.md): Compilando com CMake e C++20 Modules.

## 4. Decisões (Decisions - ADRs)
Decisões técnicas locais consolidadas:
- [`decisions/0001-cxx20-modules-and-cmake.md`](decisions/0001-cxx20-modules-and-cmake.md): Adoção de C++ Modules.
- [`decisions/0002-raylib-for-rendering.md`](decisions/0002-raylib-for-rendering.md): Adoção da biblioteca Raylib.
- [`decisions/0003-decoupled-ipc-thread.md`](decisions/0003-decoupled-ipc-thread.md): Isolar I/O de rede da thread principal (60 FPS).

## 5. Planejamento (Phases & PRDs)
- [`phases/`](phases/): Planejamento de entregas de desenvolvimento em andamento.
- [`prds/`](prds/): Requisitos de produto para a interface.

## 6. Conhecimento e Guias (Knowledge)
- [`knowledge/`](knowledge/): Guias específicos de C++ e UI.
