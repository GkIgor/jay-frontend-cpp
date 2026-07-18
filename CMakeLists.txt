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
    src/app/theme.cppm
    src/features/avatar/avatar_state.cppm
    src/features/chat/chat_state.cppm
    src/features/permissions/permission_state.cppm
    src/app/app_state.cppm
    src/shared/widgets/text_input.cppm
    src/shared/widgets/scroll_view.cppm
    src/shared/widgets/tab_bar.cppm
    src/ipc/ipc_client.cppm
    src/ipc/event_dispatcher.cppm
    src/features/avatar/avatar_renderer.cppm
    src/features/chat/message_bubble.cppm
    src/features/chat/tool_group_bubble.cppm
    src/features/chat/bubble_registry.cppm
    src/features/chat/chat_events.cppm
    src/features/chat/chat_input.cppm
    src/features/chat/chat_renderer.cppm
    src/features/permissions/permission_modal.cppm
    src/app/renderer.cppm
    src/app/main_window.cppm
    src/app/application.cppm
)
target_link_libraries(jay-frontend PRIVATE raylib nlohmann_json::nlohmann_json pthread)
