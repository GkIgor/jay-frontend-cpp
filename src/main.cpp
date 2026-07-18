#include <iostream>
import application;

int main() {
  std::cout << "=== Inicializando Jay Frontend (C++) ===" << std::endl;
  jay::Application app;
  app.Run("/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf");
  return 0;
}
