# Estrutura do Projeto C++

A organização das pastas no `jay-frontend-cpp` foca em agrupar a configuração do CMake, módulos (C++20 Modules) e arquivos-fonte.

- `CMakeLists.txt`: Configuração raiz de build, incluindo dependências como Raylib e nlohmann_json.
- `src/`: A raiz dos códigos.
  - `src/main.cpp`: Ponto de entrada do executável. Onde o Raylib e o Event Loop habitam.
  - `src/ipc/`: Subpasta para a biblioteca ou módulos de rede (threads, polling).
  - `src/ui/`: Subpasta para renderização e lógica do Raylib.
  - (Obs: Como estamos utilizando módulos (`.cppm`), arquivos de cabeçalhos `#include <.h>` legados podem existir apenas em camadas finas para APIs clássicas de C (ex: Socket), enquanto lógicas internas usam `export module X;` e `import Y;`).

- `build/`: Gerado pelo CMake, nunca comitado.
- `scripts/`: Helpers (bash) se necessário.
