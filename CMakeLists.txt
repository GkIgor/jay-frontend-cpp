cmake_minimum_required(VERSION 3.28)
project(jay-frontend-cpp CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_ASAN "Enable AddressSanitizer in Debug mode" ON)

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
    src/engine/types.cppm
    src/engine/render_context.cppm
    src/engine/widget.cppm
    src/engine/layout_engine.cppm
    src/engine/input_dispatcher.cppm
    src/engine/focus_manager.cppm
    src/engine/animation_engine.cppm
    src/state/actions.cppm
    src/state/state_store.cppm
    src/ipc/protocol.cppm
    src/usecases/use_case.cppm
    src/usecases/send_message_usecase.cppm
    src/usecases/create_chat_usecase.cppm
    src/usecases/register_client_usecase.cppm
    src/usecases/approve_permission_usecase.cppm
    src/shared/widgets/label.cppm
    src/shared/widgets/button.cppm
    src/shared/widgets/panel.cppm
    src/shared/widgets/layout_primitives.cppm
    src/features/avatar/avatar_viewmodel.cppm
    src/features/avatar/avatar_widget.cppm
    src/features/chat/message_bubble_widget.cppm
    src/features/chat/tool_group_widget.cppm
    src/features/chat/chat_viewmodel.cppm
    src/features/chat/chat_widget.cppm
    src/features/chat/chat_input_widget.cppm
    src/features/permissions/permissions_widget.cppm
    src/app/theme.cppm
    src/features/avatar/avatar_state.cppm
    src/features/chat/chat_state.cppm
    src/features/permissions/permission_state.cppm
    src/app/app_state.cppm
    src/shared/unicode/unicode.cppm
    src/shared/layout/text_layout.cppm
    src/shared/widgets/text_editor.cppm
    src/shared/widgets/text_input.cppm
    src/shared/scroll/scroll_controller.cppm
    src/shared/scroll/scrollbar.cppm
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
    src/app/main_window.cppm
    src/app/shell.cppm
    src/app/application.cppm
)
target_link_libraries(jay-frontend PRIVATE raylib nlohmann_json::nlohmann_json pthread)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_options(jay-frontend PRIVATE -g)
  if(ENABLE_ASAN)
    target_compile_options(jay-frontend PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(jay-frontend PRIVATE -fsanitize=address)
  endif()
endif()
