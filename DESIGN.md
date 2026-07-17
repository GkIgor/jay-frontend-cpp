# Tokens de Design do Jay Frontend (C++)

Este arquivo documenta as diretrizes visuais e tokens de design adotados no frontend C++ para garantir uma interface de usuário premium e coesa.

## 1. Cores (Dark Mode Premium)

- **Fundo Principal (Background)**: `#0D1117` (Deep slate dark)
- **Fundo de Painéis (Panel Background)**: `#161B22` (Transparente/translúcido ou slate mais claro)
- **Bordas (Borders)**: `#30363D` (Cinza escuro para divisões discretas)
- **Texto Principal**: `#C9D1D9` (Cinza claro suave)
- **Texto Secundário**: `#8B949E` (Cinza escuro médio para metadados/timestamps)
- **Acento do Usuário (Chat Bubble)**: `#1F6FEB` (Azul vibrante HSL adaptado para o modo escuro)
- **Acento da IA (Chat Bubble)**: `#21262D` (Cinza profundo com borda sutil)
- **Brilho Ativo (Cyan Glow)**: `#58A6FF` (Azul claro brilhante)
- **Permissão (Verde/Allow)**: `#238636`
- **Negar (Vermelho/Deny)**: `#DA3633`

## 2. Tipografia e Layout

- **Fonte**: Fonte padrão do Raylib (carregando suporte básico UTF-8).
- **Tamanho da Janela**: `600 x 550` pixels.
- **Top Tab Bar**: Altura fixa de `50` pixels.
- **Input Bar**: Altura fixa de `60` pixels ao fundo.
- **Feed de Chat**: Altura do scroll dinâmico ocupando a maior parte da área útil.

## 3. Estados Visuais do Avatar (Nuvem Digital Pulsante)

- **Idle**: Cor `#58A6FF` (Cyan), pulsação lenta e harmônica.
- **Thinking**: Cor `#D29922` (Laranja/Gold), rotação orbital ou pulsação rápida.
- **Executing**: Cor `#30A14E` (Verde), emissão de anéis concêntricos extras.
- **Sleeping**: Cor `#1F6FEB` (Azul), intensidade reduzida (Alpha esmaecido).
