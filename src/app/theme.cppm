module;
#include <raylib.h>
export module theme;

export namespace jay::Theme {
  // Tokens Semânticos do Sistema de Temas (ADR-001)
  inline const Color Background = GetColor(0x0D1117FF);
  inline const Color Surface = GetColor(0x161B22FF);
  inline const Color Border = GetColor(0x30363DFF);

  inline const Color Primary = GetColor(0x58A6FFFF);
  inline const Color PrimaryDark = GetColor(0x1F6FEBFF);
  inline const Color Success = GetColor(0x30A14EFF);
  inline const Color Warning = GetColor(0xD29922FF);
  inline const Color Danger = GetColor(0xDA3633FF);

  inline const Color TextPrimary = GetColor(0xC9D1D9FF);
  inline const Color TextSecondary = GetColor(0x8B949EFF);

  // Aliases de compatibilidade e legibilidade
  inline const Color Panel = Surface;
  inline const Color Glow = Primary;
  inline const Color UserBubble = PrimaryDark;
  inline const Color JayBubble = GetColor(0x21262DFF);
  inline const Color TextMain = TextPrimary;
  inline const Color TextSec = TextSecondary;
  inline const Color Idle = Primary;
  inline const Color Thinking = Warning;
  inline const Color Executing = Success;
  inline const Color Sleeping = GetColor(0x1F6FEB66);
  inline const Color AllowBtn = Success;
  inline const Color DenyBtn = Danger;

  // Constantes de Layout (compartilhadas por todos os módulos visuais)
  inline constexpr float StepY             = 18.0f;   // altura de linha do input de texto
  inline constexpr float BubbleStepY       = 24.0f;   // altura de linha das bolhas de chat
  inline constexpr float BubbleCornerRadius = 12.0f;  // raio fixo dos cantos dos balões
  inline constexpr float ScrollbarWidth    = 8.0f;    // largura do track+thumb
  inline constexpr float ScrollbarMinThumb = 20.0f;   // altura mínima do thumb
  inline constexpr float WheelSensChat     = 140.0f;  // pixels por notch do mouse no chat
  inline constexpr float WheelSensInput    = 54.0f;   // pixels por notch do mouse no input
  inline constexpr float InputPaddingH     = 12.0f;   // margem horizontal interna do input
} // namespace jay::Theme
