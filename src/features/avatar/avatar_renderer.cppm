module;
#include <raylib.h>
#include <math.h>
#include <string>
#include <iostream>
#include <memory>
export module avatar_renderer;

import app_state;
import avatar_state;
import theme;

export namespace jay {

class AvatarRenderer {
public:
  AvatarRenderer() = default;

  void Draw(const std::shared_ptr<ApplicationState>& state, Font font, int screenWidth, int screenHeight) {
    State currentState = state->avatar.GetState();
    const int cx = screenWidth / 2;
    const int cy = screenHeight / 2 + 30; // Compensa a nova tab bar (60px)

    // Sine-wave pulse baseada no tempo do programa
    float time = GetTime();
    float pulse = sinf(time * 4.0f) * 16.0f;
    float pulseFast = sinf(time * 12.0f) * 24.0f;

    Color coreColor = Theme::Idle;
    std::string stateLabel = "JAY (PRONTO)";
    float radius = 120.0f;
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
        pulseAmount = sinf(time * 1.5f) * 8.0f; // Pulso super lento
        break;
    }

    // Desenha efeito de brilho com círculos concêntricos e transparência decrescente
    DrawCircle(cx, cy, radius + pulseAmount + 80, ColorAlpha(coreColor, 0.05f));
    DrawCircle(cx, cy, radius + pulseAmount + 40, ColorAlpha(coreColor, 0.15f));
    DrawCircle(cx, cy, radius + pulseAmount, ColorAlpha(coreColor, 0.40f));
    DrawCircle(cx, cy, radius - 20, coreColor); // Núcleo sólido

    // Texto de status do Avatar com fontes anti-aliased
    float fontSize = 32.0f;
    Vector2 textDim = MeasureTextEx(font, stateLabel.c_str(), fontSize, 1.0f);
    Vector2 textPos = {(float)(cx - (int)textDim.x / 2), (float)(cy + radius + 100)};
    DrawTextEx(font, stateLabel.c_str(), textPos, fontSize, 1.0f, Theme::TextMain);

    // Consome animações e imprime no log
    if (auto anim = state->avatar.ConsumeNextAnimation()) {
      std::cout << "Avatar executou animação: " << *anim << "\n";
    }
  }
};

} // namespace jay
