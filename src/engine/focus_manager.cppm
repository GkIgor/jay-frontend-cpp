module;
#include <raylib.h>
#include <vector>
export module jay.engine.focus_manager;

import jay.engine.widget;

export namespace jay::engine {

// ─────────────────────────────────────────────────────────────────
// FocusManager
//
// Gerencia o foco de teclado entre widgets registrados.
//
// Regras de ownership:
//   - FocusManager NÃO possui nenhum Widget.
//   - Todos os ponteiros são não-proprietários (Widget*).
//   - O lifetime dos widgets é de responsabilidade exclusiva dos
//     seus Container pais.
//
// Garantias:
//   - Apenas um widget possui o foco por vez.
//   - Navegação via Tab/Shift+Tab na lista ordenada por tabIndex.
//   - AdvanceFocus nunca aloca: navega em vetor pré-construído.
// ─────────────────────────────────────────────────────────────────
class FocusManager {
public:
    FocusManager() {
        // Pré-alocação para evitar realocações durante SetFocusable().
        m_focusables.reserve(32);
    }

    // Registra um widget como elegível para receber foco via teclado.
    // Deve ser chamado durante Init (fora do Hot Loop).
    // tabIndex define a ordem de navegação; -1 = não focável.
    void Register(Widget* widget) {
        if (!widget || widget->tabIndex < 0) return;
        m_focusables.push_back(widget);
        // Mantém a lista sempre ordenada por tabIndex.
        // std::sort é aceitável aqui pois ocorre durante Init.
        SortFocusables();
    }

    // Remove um widget da lista de focáveis (ex: ao destruir uma tela).
    void Unregister(Widget* widget) noexcept {
        for (auto it = m_focusables.begin(); it != m_focusables.end(); ++it) {
            if (*it == widget) {
                m_focusables.erase(it);
                if (m_focusedWidget == widget) m_focusedWidget = nullptr;
                return;
            }
        }
    }

    // Define o widget em foco diretamente (ex: clique do mouse via hit-test).
    void SetFocus(Widget* widget) noexcept {
        m_focusedWidget = widget;
    }

    // Avança o foco para o próximo widget elegível (Tab).
    // reverse == true: navega em ordem inversa (Shift+Tab).
    // Nunca aloca — percorre o vetor pré-alocado.
    void AdvanceFocus(bool reverse = false) noexcept {
        if (m_focusables.empty()) return;

        if (!m_focusedWidget) {
            m_focusedWidget = reverse ? m_focusables.back() : m_focusables.front();
            return;
        }

        // Encontra o índice do widget focado atual.
        int currentIdx = -1;
        for (int i = 0; i < static_cast<int>(m_focusables.size()); ++i) {
            if (m_focusables[i] == m_focusedWidget) {
                currentIdx = i;
                break;
            }
        }

        if (currentIdx == -1) {
            m_focusedWidget = m_focusables.front();
            return;
        }

        int count = static_cast<int>(m_focusables.size());
        int nextIdx = reverse
            ? (currentIdx - 1 + count) % count
            : (currentIdx + 1) % count;

        m_focusedWidget = m_focusables[nextIdx];
    }

    // Processa Tab/Shift+Tab automaticamente a partir da Raylib.
    // Chamado no início do passo PollInputs do Game Loop.
    void ProcessTabNavigation() noexcept {
        if (IsKeyPressed(KEY_TAB)) {
            bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
            AdvanceFocus(shift);
        }
    }

    // Realiza hit-test de um clique de mouse e atribui foco ao widget clicado.
    // Percorre a lista de focáveis — não percorre a árvore de widgets.
    void HitTest(float mouseX, float mouseY) noexcept {
        for (Widget* w : m_focusables) {
            if (w->IsVisible() && w->IsEnabled()) {
                if (w->GetBounds().Contains({mouseX, mouseY})) {
                    m_focusedWidget = w;
                    return;
                }
            }
        }
    }

    [[nodiscard]] Widget* GetFocused() const noexcept { return m_focusedWidget; }

    // Limpa todos os registros (ex: ao trocar de tela).
    void Clear() noexcept {
        m_focusables.clear();
        m_focusedWidget = nullptr;
    }

private:
    Widget* m_focusedWidget{nullptr};     // Non-owning pointer.
    std::vector<Widget*> m_focusables;    // Non-owning pointers. Pré-alocado.

    void SortFocusables() noexcept {
        // Insertion sort manual para evitar dependência de <algorithm>
        // em módulos do engine. Para até 32 widgets é O(n²) aceitável.
        int n = static_cast<int>(m_focusables.size());
        for (int i = 1; i < n; ++i) {
            Widget* key = m_focusables[i];
            int j = i - 1;
            while (j >= 0 && m_focusables[j]->tabIndex > key->tabIndex) {
                m_focusables[j + 1] = m_focusables[j];
                --j;
            }
            m_focusables[j + 1] = key;
        }
    }
};

} // namespace jay::engine
