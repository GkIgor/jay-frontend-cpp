module;
#include <raylib.h>
#include <string>
export module main_window;

export namespace jay {

class MainWindow {
public:
  static void Init(int width, int height, const std::string& title) {
    InitWindow(width, height, title.c_str());
    SetTargetFPS(60);
  }

  static void Close() {
    CloseWindow();
  }

  static bool ShouldClose() {
    return WindowShouldClose();
  }
};

} // namespace jay
