module;
#include <raylib.h>
#include <string>
export module tab_bar_renderer;

import theme;

export namespace jay {

class TabBarRenderer {
public:
  TabBarRenderer() = default;

  // Renderiza a barra de abas superior e retorna o novo índice selecionado caso haja clique
  int UpdateAndDraw(int currentTab, int screenWidth, Font font) {
    const int tabHeight = 60; // Aumentado proporcionalmente para a nova tela
    const int tabWidth = screenWidth / 2;

    // Fundo da barra
    DrawRectangle(0, 0, screenWidth, tabHeight, Theme::Panel);
    DrawLine(0, tabHeight - 1, screenWidth, tabHeight - 1, Theme::Border);

    int clickedTab = currentTab;
    Vector2 mousePos = GetMousePosition();

    for (int i = 0; i < 2; ++i) {
      Rectangle tabRect = {(float)(i * tabWidth), 0.0f, (float)tabWidth, (float)tabHeight};
      bool isHovered = CheckCollisionPointRec(mousePos, tabRect);
      bool isActive = (currentTab == i);

      // Trata cliques
      if (isHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        clickedTab = i;
      }

      // Desenha fundo hover/ativo
      if (isActive) {
        DrawRectangleRec(tabRect, GetColor(0x1F293755)); // Fundo leve escuro
        // Desenha indicador de aba ativa inferior brilhante
        DrawRectangle(i * tabWidth + 30, tabHeight - 4, tabWidth - 60, 4, Theme::Glow);
      } else if (isHovered) {
        DrawRectangleRec(tabRect, GetColor(0x1F293733)); // Hover leve
      }

      // Texto da Aba
      std::string text = (i == 0) ? "AVATAR" : "CHAT";
      float fontSize = 20.0f; // Fonte maior para a tela maior
      Vector2 textDim = MeasureTextEx(font, text.c_str(), fontSize, 1.0f);
      int tx = i * tabWidth + (tabWidth / 2) - ((int)textDim.x / 2);
      int ty = (tabHeight / 2) - ((int)textDim.y / 2);

      Vector2 textPos = {(float)tx, (float)ty};
      DrawTextEx(font, text.c_str(), textPos, fontSize, 1.0f, isActive ? Theme::TextMain : Theme::TextSec);
    }

    return clickedTab;
  }
};

} // namespace jay
