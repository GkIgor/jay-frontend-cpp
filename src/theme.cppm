module;
#include <raylib.h>
export module theme;

export namespace jay::Theme {
  // Cores de fundo e painel (Dark Mode Premium)
  inline const Color Background = GetColor(0x0D1117FF);
  inline const Color Panel = GetColor(0x161B22FF);
  inline const Color Border = GetColor(0x30363DFF);

  // Cores de texto
  inline const Color TextMain = GetColor(0xC9D1D9FF);
  inline const Color TextSec = GetColor(0x8B949EFF);

  // Acentos visuais
  inline const Color UserBubble = GetColor(0x1F6FEBFF); // Azul
  inline const Color JayBubble = GetColor(0x21262DFF);  // Cinza Escuro
  inline const Color Glow = GetColor(0x58A6FFFF);       // Cyan

  // Cores de estado / ação
  inline const Color Idle = GetColor(0x58A6FFFF);
  inline const Color Thinking = GetColor(0xD29922FF);
  inline const Color Executing = GetColor(0x30A14EFF);
  inline const Color Sleeping = GetColor(0x1F6FEB66); // Alpha reduzido

  // Modais de segurança
  inline const Color AllowBtn = GetColor(0x238636FF);
  inline const Color DenyBtn = GetColor(0xDA3633FF);
}
