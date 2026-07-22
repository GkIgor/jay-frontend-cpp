module;
#include <raylib.h>
#include <vector>
export module jay.engine.input_dispatcher;

import jay.engine.types;
import jay.engine.widget;

export namespace jay::engine {

// ─────────────────────────────────────────────────────────────────
// InputDispatcher
//
// Responsabilidades:
//   1. Capturar eventos brutos da Raylib (mouse, teclado, scroll).
//   2. Construir InputEvent imutáveis (tipos de valor).
//   3. Propagar eventos na árvore de widgets via Bubbling (último
//      filho inserido = topo visual) ou direto ao widget focado
//      para eventos de teclado.
//
// Zero alocação: InputEvent passa por valor escalar.
// ─────────────────────────────────────────────────────────────────
class InputDispatcher {
public:
    // Captura todos os eventos da Raylib deste frame e os propaga
    // para a árvore de widgets via `root.OnEvent(event)`.
    // O Widget focado é o destino primário de eventos de teclado.
    static void Poll(Widget& root, Widget* focusedWidget) noexcept {
        // ── Movimento do Mouse ──
        {
            InputEvent ev;
            ev.kind   = InputEventKind::MouseMove;
            ev.mouseX = GetMouseX();
            ev.mouseY = GetMouseY();
            ev.ctrl   = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
            ev.shift  = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
            ev.alt    = IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT);
            root.OnEvent(ev);
        }

        // ── Cliques de Mouse ──
        for (int btn : {MOUSE_BUTTON_LEFT, MOUSE_BUTTON_RIGHT, MOUSE_BUTTON_MIDDLE}) {
            if (IsMouseButtonPressed(btn)) {
                InputEvent ev;
                ev.kind   = InputEventKind::MousePress;
                ev.mouseX = GetMouseX();
                ev.mouseY = GetMouseY();
                ev.key    = btn;
                ev.ctrl   = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
                ev.shift  = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
                ev.alt    = IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT);
                root.OnEvent(ev);
            }
            if (IsMouseButtonReleased(btn)) {
                InputEvent ev;
                ev.kind   = InputEventKind::MouseRelease;
                ev.mouseX = GetMouseX();
                ev.mouseY = GetMouseY();
                ev.key    = btn;
                root.OnEvent(ev);
            }
        }

        // ── Scroll do Mouse ──
        {
            float wheel = GetMouseWheelMove();
            if (wheel != 0.0f) {
                InputEvent ev;
                ev.kind        = InputEventKind::MouseScroll;
                ev.mouseX      = GetMouseX();
                ev.mouseY      = GetMouseY();
                ev.scrollDelta = wheel;
                root.OnEvent(ev);
            }
        }

        // ── Teclas (enviadas ao widget focado, depois ao root) ──
        {
            bool ctrl  = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
            bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
            bool alt   = IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT);

            int key = GetKeyPressed();
            while (key != 0) {
                InputEvent ev;
                ev.kind  = InputEventKind::KeyPress;
                ev.key   = key;
                ev.ctrl  = ctrl;
                ev.shift = shift;
                ev.alt   = alt;

                bool consumed = false;
                if (focusedWidget) consumed = focusedWidget->OnEvent(ev);
                if (!consumed) root.OnEvent(ev);

                key = GetKeyPressed();
            }
        }

        // ── Caracteres Unicode Digitados ──
        {
            int cp = GetCharPressed();
            while (cp != 0) {
                InputEvent ev;
                ev.kind      = InputEventKind::CharInput;
                ev.codepoint = cp;

                bool consumed = false;
                if (focusedWidget) consumed = focusedWidget->OnEvent(ev);
                if (!consumed) root.OnEvent(ev);

                cp = GetCharPressed();
            }
        }
    }
};

} // namespace jay::engine
