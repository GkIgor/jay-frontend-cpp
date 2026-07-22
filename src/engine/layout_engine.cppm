module;
export module jay.engine.layout_engine;

import jay.engine.types;
import jay.engine.widget;

export namespace jay::engine {

// ─────────────────────────────────────────────────────────────────
// LayoutEngine
//
// Executa o algoritmo Box Constraints em dois passos:
//   1. Top-Down: propaga BoxConstraints do pai para cada filho.
//   2. Bottom-Up: filhos reportam seu tamanho preferido.
//
// A resolução de layout de uma subárvore ocorre SOMENTE quando o
// nó raiz dessa subárvore está marcado como m_layoutDirty == true.
// Nós limpos são ignorados, evitando recálculos desnecessários.
// ─────────────────────────────────────────────────────────────────
class LayoutEngine {
public:
    // Resolve o layout da árvore inteira a partir da raiz.
    // constraints: os limites disponíveis para o widget raiz
    //              (normalmente o tamanho da janela).
    static void Resolve(Widget& root, BoxConstraints constraints) noexcept {
        if (!root.IsLayoutDirty()) return;
        root.Layout(constraints);
    }
};

} // namespace jay::engine
