# ADR 0001: Adoção de Módulos C++20 e CMake

**Status:** Aceito
**Data:** 14 de Julho de 2026

## Contexto
Projetos grandes em C++ sofrem com tempos de build exorbitantes e fragilidade através de `#include` macros e vazamento (leak) de namespaces globais. A linguagem inseriu nativamente na especificação 20 o conceito de módulos (`import`, `export module`), modernizando o ciclo de desenvolvimento.

## Decisão
Decidimos que todo código de lógica interna do Frontend será redigido utilizando **C++20 Modules**. Não usaremos arquivos de header obsoletos `.hpp` ou `.h` criados por nós, exceto caso bibliotecas legadas exijam camadas de adaptação ou se houver forte limitação do compilador para APIs de socket em C puro. O sistema de build eleito que provê suporte maduro para compilação e rastreamento de dependências de módulos é o **CMake >= 3.28** com **Ninja generator**.

## Consequências
- **Positivo**: Tempos de compilação melhores, melhor encapsulamento (macros não vazam) e código visivelmente mais moderno (livre de header guards `#ifndef`).
- **Negativo**: Exige compiladores na vanguarda (GCC 14+ ou Clang 18+) e pode dificultar intellisense (LSP) em alguns editores que ainda estão alcançando o padrão de módulos.
