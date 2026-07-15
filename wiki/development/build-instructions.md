# Instruções de Build (C++)

Como este projeto utiliza módulos C++20 e depende do Raylib, utilizamos o **CMake**.

## Requisitos
- GCC-14+ ou Clang 18+ (que possua suporte forte a Módulos C++20).
- CMake (>= 3.28 para suporte a `cxx_modules`).
- Ninja (Sistema de build recomendado e geralmente necessário para módulos no CMake).
- Dependências X11/Wayland/OpenGL instaladas no sistema hospedeiro (para o Raylib compilar via source).

## Compilando
Sempre que fizer alterações no código, siga este fluxo:

1. **Geração (Configuração)** (necessário rodar apenas na primeira vez ou quando mudar o `CMakeLists.txt`):
   ```bash
   cmake -B build -S . -G Ninja
   ```

2. **Compilação**:
   ```bash
   cmake --build build
   ```

3. **Execução**:
   ```bash
   ./build/jay-frontend
   ```

## Dependências Automáticas
A biblioteca JSON (`nlohmann_json`) e o motor gráfico (`Raylib`) são gerenciados de forma embarcada através do comando `FetchContent` do CMake. Você não precisa instalá-los globalmente no seu computador. A primeira etapa `cmake -B build` efetuará o download e configuração estática no seu diretório `build`.
