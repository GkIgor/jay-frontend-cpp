module;
#include <raylib.h>
#include <math.h>
#include <string>
#include <iostream>
#include <memory>
export module avatar_renderer;

import app_state;
import theme;

export namespace jay {

class AvatarRenderer {
public:
  AvatarRenderer() = default;

  void Draw(const std::shared_ptr<ApplicationState>& state, int screenWidth, int screenHeight) {
    State currentState = state->GetState();
    const int cx = screenWidth / 2;
    const int cy = screenHeight / 2 + 25; // Compensa a tab bar

    // Sine-wave pulse baseada no tempo do jogo/programa
    float time = GetTime();
    float pulse = sinf(time * 4.0f) * 8.0f;
    float pulseFast = sinf(time * 12.0f) * 12.0f;

    Color coreColor = Theme::Idle;
    std::string stateLabel = "JAY (IDLE)";
    float radius = 60.0f;
    float pulseAmount = pulse;

    switch (currentState) {
      case State::Idle:
        coreColor = Theme::Idle;
        stateLabel = "JAY (PRONTO)";
        pulseAmount = pulse;
        break;
      case State::Thinking:
        coreColor = Theme::Thinking;
        stateLabel = "PENSANDO...";
        pulseAmount = pulseFast;
        break;
      case State::Executing:
        coreColor = Theme::Executing;
        stateLabel = "EXECUTANDO AÇÃO";
        pulseAmount = pulseFast * 1.5f;
        break;
      case State::Sleeping:
        coreColor = Theme::Sleeping;
        stateLabel = "DORMINDO";
        pulseAmount = sinf(time * 1.5f) * 4.0f; // Pulso super lento
        break;
    }

    // Desenha efeito de brilho com círculos concêntricos e transparência decrescente
    DrawCircle(cx, cy, radius + pulseAmount + 40, ColorAlpha(coreColor, 0.05f));
    DrawCircle(cx, cy, radius + pulseAmount + 20, ColorAlpha(coreColor, 0.15f));
    DrawCircle(cx, cy, radius + pulseAmount, ColorAlpha(coreColor, 0.40f));
    DrawCircle(cx, cy, radius - 10, coreColor); // Núcleo sólido

    // Texto de status do Avatar
    int fontSize = 20;
    int textLen = MeasureText(stateLabel.c_str(), fontSize);
    DrawText(stateLabel.c_str(), cx - textLen / 2, cy + radius + 60, fontSize, Theme::TextMain);

    // Consome animações e imprime no log do console se houver
    if (auto anim = state->ConsumeNextAnimation()) {
      std::cout << "Avatar executou animação: " << *anim << "\n";
    }
  }
};

} // namespace jay
