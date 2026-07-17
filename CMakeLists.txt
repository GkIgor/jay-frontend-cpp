cmake_minimum_required(VERSION 3.28)
project(jay-frontend-cpp CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(FetchContent)
FetchContent_Declare(nlohmann_json GIT_REPOSITORY https://github.com/nlohmann/json.git GIT_TAG v3.11.3)
set(JSON_BuildTests OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(nlohmann_json)
FetchContent_Declare(raylib GIT_REPOSITORY https://github.com/raysan5/raylib.git GIT_TAG 5.0)
set(BUILD_EXAMPLES OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(raylib)
add_executable(jay-frontend src/main.cpp)
target_sources(jay-frontend
  PUBLIC
    FILE_SET CXX_MODULES FILES
    src/app_state.cppm
    src/avatar.cppm
    src/theme.cppm
    src/text_input.cppm
    src/tab_bar_renderer.cppm
    src/avatar_renderer.cppm
    src/chat_renderer.cppm
    src/event_dispatcher.cppm
    src/ipc_client.cppm
    src/renderer.cppm
)
target_link_libraries(jay-frontend PRIVATE raylib nlohmann_json::nlohmann_json pthread)
