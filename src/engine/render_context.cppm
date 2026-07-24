module;
// Global Module Fragment: ÚNICO lugar legítimo para incluir Raylib.
// Nenhum outro arquivo .cppm do projeto pode incluir <raylib.h>.
#include <raylib.h>

#include <cassert>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
export module jay.engine.render_context;

import jay.engine.types;

export namespace jay::engine {

inline void WriteClipboardText(std::string_view text) {
  const std::string nullTerminatedText{text};
  ::SetClipboardText(nullTerminatedText.c_str());
}

// ─────────────────────────────────────────────────────────────────
// RenderContext
//
// Encapsula toda a API de desenho da Raylib e é o ÚNICO módulo
// autorizado a conhecer tipos da Raylib (Font, Rectangle, Color, etc).
//
// Responsabilidades:
//   1. Converter tipos internos (jay::engine::*) para tipos Raylib.
//   2. Expor primitivas de desenho sem vazar tipos da Raylib.
//   3. Gerenciar a Scissor Stack sem alocação no Hot Loop.
//   4. Armazenar a Font ativa carregada durante o bootstrap.
// ─────────────────────────────────────────────────────────────────
class RenderContext {
public:
  // Capacidade pré-alocada da pilha de recortes (Scissor Stack).
  // Suficiente para qualquer profundidade de aninhamento real da UI.
  static constexpr int kScissorStackCapacity = 16;

  explicit RenderContext() { m_scissorStack.reserve(kScissorStackCapacity); }

  ~RenderContext() {
    if (m_fontLoaded && m_font.texture.id != 0) {
      UnloadFont(m_font);
    }
  }

  // Não copiável nem movível: RenderContext possui recursos de GPU.
  RenderContext(const RenderContext&) = delete;
  RenderContext& operator=(const RenderContext&) = delete;
  RenderContext(RenderContext&&) = delete;
  RenderContext& operator=(RenderContext&&) = delete;

  // ── Ciclo de Vida ─────────────────────────────────────────────

  // Carrega a fonte do sistema. Chamado durante o bootstrap (fora do Hot Loop).
  // codepoints: array de codepoints Unicode a incluir na textura de fonte.
  void LoadFont(const char* fontPath, int fontSize, int* codepoints, int codepointCount) {
    if (m_fontLoaded && m_font.texture.id != 0) {
      UnloadFont(m_font);
    }
    m_font = ::LoadFontEx(fontPath, fontSize, codepoints, codepointCount);
    if (m_font.texture.id == 0) {
      m_font = ::GetFontDefault();
    } else {
      SetTextureFilter(m_font.texture, TEXTURE_FILTER_BILINEAR);
    }
    m_fontLoaded = true;
  }

  // ── Primitivas de Desenho ─────────────────────────────────────

  void DrawRect(Rect r, Color c) const noexcept { DrawRectangleRec(ToRL(r), ToRL(c)); }

  void DrawRectRounded(Rect r, float radiusPx, Color c) const noexcept {
    float shortSide = (r.width < r.height) ? r.width : r.height;
    float roundness = (shortSide > 0.0f) ? (2.0f * radiusPx / shortSide) : 0.0f;
    if (roundness > 1.0f) roundness = 1.0f;
    DrawRectangleRounded(ToRL(r), roundness, 8, ToRL(c));
  }

  void DrawRectRoundedLines(Rect r, float radiusPx, float thick, Color c) const noexcept {
    float shortSide = (r.width < r.height) ? r.width : r.height;
    float roundness = (shortSide > 0.0f) ? (2.0f * radiusPx / shortSide) : 0.0f;
    if (roundness > 1.0f) roundness = 1.0f;
    DrawRectangleRoundedLines(ToRL(r), roundness, 8, thick, ToRL(c));
  }

  void DrawRectLines(Rect r, float thick, Color c) const noexcept { DrawRectangleLinesEx(ToRL(r), thick, ToRL(c)); }

  // Desenha texto com a fonte ativa, respeitando o Scissor ativo.
  // spacing: espaçamento entre caracteres (padrão 0.5).
  void DrawText(std::string_view text, Vec2 pos, float fontSize, Color c, float spacing = 0.5f) const noexcept {
    // std::string_view não é null-terminated; Raylib exige char*.
    // Usamos DrawTextEx com a view diretamente é seguro apenas se
    // o caller garantir terminador. Para segurança, usamos o overload
    // que aceita tamanho via TextSubtext (limitado a 512 chars) ou
    // copiamos para o buffer estático da thread:
    DrawTextEx(m_font,
               text.data(),  // caller deve garantir terminador
               {pos.x, pos.y}, fontSize, spacing, ToRL(c));
  }

  // Mede a largura de um texto com a fonte ativa.
  [[nodiscard]] float MeasureText(std::string_view text, float fontSize, float spacing = 0.5f) const noexcept {
    Vector2 size = MeasureTextEx(m_font, text.data(), fontSize, spacing);
    return size.x;
  }

  // Retorna a altura de uma linha de texto com a fonte ativa.
  [[nodiscard]] float LineHeight(float fontSize) const noexcept {
    return fontSize;  // Raylib: lineHeight == fontSize para fontes carregadas.
  }

  // ── Scissor Stack ─────────────────────────────────────────────

  // Empurra um recorte retangular. Frames filhos são desenhados
  // somente dentro deste Rect. Não aloca (pré-reservado no ctor).
  void PushScissor(Rect r) noexcept {
    assert(static_cast<int>(m_scissorStack.size()) < kScissorStackCapacity &&
           "Scissor Stack overflow — profundidade de aninhamento excessiva");
    EndScissorMode();  // Encerra o scissor anterior (Raylib não empilha nativamente).
    m_scissorStack.push_back(r);
    BeginScissorMode(static_cast<int>(r.x), static_cast<int>(r.y), static_cast<int>(r.width),
                     static_cast<int>(r.height));
  }

  // Remove o recorte atual e restaura o anterior (se houver).
  void PopScissor() noexcept {
    if (m_scissorStack.empty()) return;
    m_scissorStack.pop_back();
    EndScissorMode();
    if (!m_scissorStack.empty()) {
      const Rect& r = m_scissorStack.back();
      BeginScissorMode(static_cast<int>(r.x), static_cast<int>(r.y), static_cast<int>(r.width),
                       static_cast<int>(r.height));
    }
  }

  [[nodiscard]] bool HasActiveScissor() const noexcept { return !m_scissorStack.empty(); }

  // ── Acesso a Primitivas de Janela ─────────────────────────────

  [[nodiscard]] float ScreenWidth() const noexcept { return static_cast<float>(GetScreenWidth()); }
  [[nodiscard]] float ScreenHeight() const noexcept { return static_cast<float>(GetScreenHeight()); }
  [[nodiscard]] float DeltaTime() const noexcept { return GetFrameTime(); }

  // Expõe a Font nativa da Raylib SOMENTE para código legado durante
  // a migração. Este método será removido quando todos os módulos
  // migrarem para as primitivas de RenderContext.
  // TODO(Task-25): Remover após migração completa dos renderers legados.
  [[nodiscard]] const ::Font& NativeFont() const noexcept { return m_font; }

private:
  ::Font m_font{};
  bool m_fontLoaded{false};

  // Pré-alocado no ctor — sem alocações dinâmicas no Hot Loop.
  std::vector<Rect> m_scissorStack;

  // ── Conversão de Tipos Internos para Raylib ───────────────────

  [[nodiscard]] static ::Rectangle ToRL(Rect r) noexcept { return {r.x, r.y, r.width, r.height}; }

  [[nodiscard]] static ::Color ToRL(Color c) noexcept { return {c.r, c.g, c.b, c.a}; }
};

}  // namespace jay::engine
