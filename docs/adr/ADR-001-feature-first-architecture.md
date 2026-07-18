# ADR-001: Feature-First Architecture no Frontend C++ (Jay)

## Status
**Aprovado com ajustes** - 17 de Julho de 2026

## Contexto
O frontend C++ (`jay-frontend-cpp`) está crescendo de forma contínua com a adição de novos elementos visuais, comportamentos dinâmicos e novos fluxos do sistema (como gerenciamento de permissões, logs e configurações). A antiga estrutura plana de arquivos na raiz do diretório `src/` gerava acoplamento excessivo e dificultava a escalabilidade e a componentização dos elementos gráficos.

## Decisão
Adotamos uma estrutura baseada em **Feature-First Architecture** associada a uma separação estrita de responsabilidades inspirada nos padrões reativos modernos (como Flutter e Angular).

### Diretrizes de Pastas e Componentes
1. **`src/app/`**: Contém o orquestrador principal do ciclo de vida da janela Raylib (`Application` e `MainWindow`), o compositor global de telas (`Renderer`) e o sistema de estilo unificado.
2. **`src/ipc/`**: Encapsula as conexões e recepção de eventos via sockets, mantendo o tráfego de rede isolado do desenho.
3. **`src/shared/`**: Bibliotecas de UI (`widgets/`) e recursos comuns (animações, fontes) sem regras de negócio ou de features.
4. **`src/features/`**: Funcionalidades independentes organizadas por domínio. Cada feature agrupa seu próprio Renderer, State e Input Handlers.

### Princípios Arquiteturais Definidos
* **Organização por Feature**: Todos os arquivos relacionados a um domínio de negócio (ex: chat, avatar, permissões) vivem juntos no seu diretório respectivo.
* **Widgets Compartilhados em `shared/`**: Componentes puramente visuais e genéricos (ex: `TextInput`, `ScrollView`, `Button`) residem em `shared/` e não devem possuir qualquer dependência ou regra de negócio de features específicas.
* **Features Independentes do IPC**: As features operam sobre dados reativos expostos em seus estados respectivos e não fazem chamadas diretas ou acopladas de rede para o IPC.
* **Renderizadores Apenas Desenham**: A função dos arquivos `*_renderer.cppm` é ler os estados das features e renderizar primitivas geométricas ou widgets, sem carregar lógica de negócios ou mutações internas de estado.
* **Estados Apenas Armazenam Dados**: Os estados de cada feature (`*_state.cppm`) contêm dados puros e métodos thread-safe para leitura/escrita, e são agrupados no agregador leve `ApplicationState`.
* **Tema por Tokens Semânticos**: As cores e estilos visuais são obtidos através de tokens semânticos (ex: `Background`, `Surface`, `Primary`, `Success`, `Danger`, `TextPrimary`, `TextSecondary`), isolando a paleta visual do desenho das telas.

---

## Estrutura Física Detalhada

```text
src/
├── app/                  # Orquestração da aplicação, ciclo de vida e tema global
│   ├── application.cppm   # Classe principal que roda o app
│   ├── main_window.cppm   # Inicializa e gerencia a janela do Raylib
│   ├── renderer.cppm      # Renderizador compositor global das abas/features
│   └── theme.cppm         # Tokens de cor semânticos do tema visual
│
├── ipc/                  # Módulos de rede e roteamento de eventos
│   ├── ipc_client.cppm    # Cliente UNIX domain socket assíncrono
│   └── event_dispatcher.cppm # Distribui payloads para os estados das features
│
├── shared/               # Widgets de UI reutilizáveis e recursos comuns
│   └── widgets/
│       ├── button.cppm       # Botão padrão com estados hover/click
│       ├── tab_bar.cppm      # Barra superior de abas deslizantes
│       ├── text_input.cppm   # Caixa de digitação multi-linha escalável
│       └── scroll_view.cppm  # Gerenciador de scroll e desenho de scrollbar
│
├── features/             # Recursos de funcionalidade isoladas (feature-first)
│   ├── avatar/
│   │   ├── avatar_state.cppm    # Estado e emoções do avatar
│   │   └── avatar_renderer.cppm # Renderização geométrica do núcleo digital
│   │
│   ├── chat/
│   │   ├── chat_state.cppm      # Mensagens, feed e locks de sincronização
│   │   ├── chat_renderer.cppm   # Compositor do feed e input de texto
│   │   ├── chat_input.cppm      # Caixa expansível e tratador de enter
│   │   ├── chat_events.cppm     # Capturador de cliques, scroll e seleção
│   │   └── message_bubble.cppm  # Balões de chat e cópias rápidas
│   │
│   └── permissions/
│       ├── permission_state.cppm # Estado de permissões de ferramentas pendentes
│       └── permission_modal.cppm # Modal de consentimento e aprovação
│
└── main.cpp              # Ponto de entrada básico do app
```
