# Não Objetivos (Non-Goals)

Para evitar inflar o escopo, **o Frontend definitivamente NÃO fará**:

1. **Processamento de LLM**: Inferências e execução de modelos de IA grandes ocorrem via Core, não via Frontend.
2. **Manipulação de Sistema**: Arquivos locais do projeto, comandos bash ou execução de plugins da IA não serão gerenciados pelo C++. Todo poder destrutivo e executável reside no Core em Go (via OpenClaw/ToolBus). O Frontend apenas repassa permissões.
3. **Persistência de Dados Complexa**: Não usaremos SQLite ou CoreData no frontend. Se o frontend precisa de histórico ou cache local, deve requisitar ao Core.
4. **Motor Completo de Jogos (Game Engine Completa)**: Embora utilizemos Raylib, não transformaremos a base de código em uma hierarquia gigantesca de ECS (Entity Component System) com colisões complexas. A abordagem será a mais simples e direta possível para desenhar as interfaces (GUI).
