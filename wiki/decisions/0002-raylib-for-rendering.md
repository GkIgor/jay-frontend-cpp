# ADR 0002: Adoção de Raylib para Renderização

**Status:** Aceito
**Data:** 14 de Julho de 2026

## Contexto
O Frontend precisa desenhar a interface visual e manipular áudio. O ecossistema C++ possui motores gigantes (Unreal, Godot) ou bibliotecas de baixo nível (OpenGL puro, Vulkan) e toolkits complexos (Qt, GTK). Como nosso frontend requer gráficos leves para desenhar o Avatar (ondas sonoras 2D / shaders básicos / transparências) com baixa curva de complexidade e forte portabilidade.

## Decisão
A interface será construída usando **Raylib**.

## Consequências
- **Positivo**: Instalação fácil (`FetchContent` no CMake sem dezenas de dependências externas como Boost ou Qt), API em C direta ao ponto, loop de rendering (60 FPS) sob total controle da nossa Thread Principal. Muito leve na GPU/CPU, excelente manipulação nativa de Áudio e Texto.
- **Negativo**: Layouts avançados e complexos típicos da Web (ex: CSS flexbox nativo, DOM) não existem; tudo é desenhado geometricamente na tela (immediate-mode), demandando construção manual de elementos de UI.
