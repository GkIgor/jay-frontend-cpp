module;
#include <vector>
#include <functional>
#include <cstdint>
#include <memory>
export module jay.engine.widget;

import jay.engine.types;
import jay.engine.render_context;

export namespace jay::engine {

// ─────────────────────────────────────────────────────────────────
// InputEvent — struct imutável passada por valor no pipeline de eventos.
// Nenhuma alocação dinâmica. Todos os membros são escalares.
// ─────────────────────────────────────────────────────────────────
enum class InputEventKind : uint8_t {
    MouseMove,
    MousePress,
    MouseRelease,
    MouseScroll,
    KeyPress,
    KeyRepeat,
    KeyRelease,
    CharInput,
};

struct InputEvent {
    InputEventKind kind{InputEventKind::MouseMove};
    // Coordenadas de mouse (em px, relativas à janela)
    float mouseX{0.0f};
    float mouseY{0.0f};
    // Scroll delta (notch positivo = cima, negativo = baixo)
    float scrollDelta{0.0f};
    // Tecla (código Raylib KEY_*)
    int   key{0};
    // Caractere Unicode digitado (em CharInput)
    int   codepoint{0};
    // Modificadores
    bool  ctrl{false};
    bool  shift{false};
    bool  alt{false};
    // Consumido por um widget — interrompe a propagação
    mutable bool handled{false};
};

// ─────────────────────────────────────────────────────────────────
// Widget — classe base abstrata de todos os componentes visuais.
//
// Ciclo de vida por frame (Main Thread):
//   1. Init()    — uma única vez após construção.
//   2. Layout()  — chamado pelo pai quando m_layoutDirty == true.
//   3. Update()  — avança animações e estado transitório de UI.
//   4. Render()  — emite primitivas via RenderContext (const — zero side-effects).
//   5. OnEvent() — processa InputEvent; retorna true se consumiu.
// ─────────────────────────────────────────────────────────────────
class Widget {
public:
    virtual ~Widget() = default;

    // Não copiável — widgets têm identidade e ownership único.
    Widget(const Widget&)            = delete;
    Widget& operator=(const Widget&) = delete;

    // Inicialização pós-construção (pode alocar — ocorre fora do Hot Loop).
    virtual void Init() {}

    // Avança estado transitório de UI (hover, animações, cursor).
    // NÃO executa chamadas de desenho.
    virtual void Update(float /*deltaTime*/) {}

    // Calcula a geometria do widget dentro das restrições do pai.
    // Propaga para filhos se necessário. Chamado somente quando dirty.
    virtual void Layout(const BoxConstraints& constraints) {
        m_bounds.width  = constraints.maxW;
        m_bounds.height = constraints.maxH;
        m_layoutDirty   = false;
    }

    // Emite primitivas gráficas via RenderContext.
    // PURO: nunca muta estado de negócio, ViewModel ou StateStore.
    virtual void Render(RenderContext& ctx) const = 0;

    // Processa um InputEvent. Retorna true se consumiu o evento.
    virtual bool OnEvent(const InputEvent& /*event*/) { return false; }

    // ── Gestão de Invalidação de Layout ───────────────────────────

    // Marca este nó e propaga a invalidação para o pai (se registrado).
    void MarkLayoutDirty() noexcept {
        m_layoutDirty = true;
        if (m_onDirty) m_onDirty();
    }

    [[nodiscard]] bool IsLayoutDirty() const noexcept { return m_layoutDirty; }

    // Registra callback de propagação de dirty para o pai.
    // Chamado pelo Container ao adicionar este widget como filho.
    void SetDirtyCallback(std::function<void()> callback) noexcept {
        m_onDirty = std::move(callback);
    }

    // ── Geometria ─────────────────────────────────────────────────

    void SetBounds(Rect bounds) noexcept {
        m_bounds = bounds;
    }

    [[nodiscard]] Rect GetBounds() const noexcept { return m_bounds; }

    // ── Visibilidade e Estado ─────────────────────────────────────

    void SetVisible(bool visible) noexcept { m_visible = visible; }
    [[nodiscard]] bool IsVisible() const noexcept { return m_visible; }

    void SetEnabled(bool enabled) noexcept { m_enabled = enabled; }
    [[nodiscard]] bool IsEnabled() const noexcept { return m_enabled; }

    // tabIndex: ordem de navegação por teclado (Tab/Shift+Tab).
    // -1 = não focável.
    int tabIndex{-1};

protected:
    Widget() = default;

    Rect  m_bounds{};
    bool  m_layoutDirty{true};
    bool  m_visible{true};
    bool  m_enabled{true};

    // Callback registrado pelo Container pai para propagação de dirty.
    std::function<void()> m_onDirty;
};

// ─────────────────────────────────────────────────────────────────
// ContainerWidget — Widget que possui filhos (unique_ptr).
//
// Regras de ownership:
//   - O container é o ÚNICO proprietário dos filhos.
//   - Filhos recebem referência não-proprietária para o callback
//     de propagação de dirty do container.
// ─────────────────────────────────────────────────────────────────
class ContainerWidget : public Widget {
public:
    ContainerWidget() {
        // Pré-alocação para evitar realocações no Hot Loop.
        // Capacidade de 16 filhos é suficiente para painéis reais.
        m_children.reserve(16);
    }

    // Adiciona um filho e registra o callback de propagação de dirty.
    void AddChild(std::unique_ptr<Widget> child) {
        child->SetDirtyCallback([this]() { MarkLayoutDirty(); });
        child->Init();
        m_children.push_back(std::move(child));
    }

    void Update(float deltaTime) override {
        for (auto& child : m_children) {
            if (child->IsVisible()) child->Update(deltaTime);
        }
    }

    void Render(RenderContext& ctx) const override {
        for (const auto& child : m_children) {
            if (child->IsVisible()) child->Render(ctx);
        }
    }

    bool OnEvent(const InputEvent& event) override {
        // Bubbling: propaga da frente para trás (última camada = topo visual).
        for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
            auto& child = *it;
            if (child->IsVisible() && child->IsEnabled()) {
                if (child->OnEvent(event)) return true;
            }
        }
        return false;
    }

protected:
    // unique_ptr: ownership estrito. Pré-alocado no ctor.
    std::vector<std::unique_ptr<Widget>> m_children;
};

} // namespace jay::engine
