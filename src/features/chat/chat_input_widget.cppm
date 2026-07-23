module;
#include <memory>
#include <string>
export module jay.features.chat.input_widget;

import jay.engine.types;
import jay.engine.render_context;
import jay.engine.widget;
import jay.shared.widgets.button;
import jay.features.chat.viewmodel;

export namespace jay::features::chat {

class ChatInputWidget : public jay::engine::Widget {
public:
    explicit ChatInputWidget(std::shared_ptr<ChatViewModel> viewModel)
        : m_viewModel(std::move(viewModel)) {
        tabIndex = 1; // Interativo e focável por teclado
    }

    void Init() override {
        // Inicializa o botão de envio
        m_sendButton = std::make_unique<jay::shared::widgets::Button>("ENVIAR", [this]() {
            SubmitText();
        });
        m_sendButton->Init();
    }

    void Layout(const jay::engine::BoxConstraints& constraints) override {
        m_bounds.width  = constraints.maxW;
        m_bounds.height = (constraints.maxH > 0.0f) ? constraints.maxH : 50.0f;

        // Posiciona o botão no canto direito da caixa de entrada
        float btnW = 90.0f;
        float btnH = m_bounds.height - 12.0f;
        m_sendButton->Layout(jay::engine::BoxConstraints::Tight(btnW, btnH));
        m_sendButton->SetBounds(jay::engine::Rect{
            m_bounds.x + m_bounds.width - btnW - 6.0f,
            m_bounds.y + 6.0f,
            btnW,
            btnH
        });

        m_layoutDirty = false;
    }

    void Update(float deltaTime) override {
        if (m_sendButton) m_sendButton->Update(deltaTime);
    }

    void Render(jay::engine::RenderContext& ctx) const override {
        if (!m_visible) return;

        // Desenha o fundo da caixa de entrada de texto
        jay::engine::Color bg{22, 27, 34, 255};
        jay::engine::Color border{48, 54, 61, 255};

        ctx.DrawRectRounded(m_bounds, 8.0f, bg);
        ctx.DrawRectLines(m_bounds, 1.0f, border);

        // Desenha o texto digitado ou placeholder
        float fontSize = 15.0f;
        float textX = m_bounds.x + 14.0f;
        float textY = m_bounds.y + (m_bounds.height - fontSize) * 0.5f;

        if (m_buffer.empty()) {
            ctx.DrawText("Digite uma mensagem...", {textX, textY}, fontSize, jay::engine::Color{139, 148, 158, 255});
        } else {
            ctx.DrawText(m_buffer, {textX, textY}, fontSize, jay::engine::Color{201, 209, 217, 255});
        }

        // Desenha o botão de envio
        if (m_sendButton) m_sendButton->Render(ctx);
    }

    bool OnEvent(const jay::engine::InputEvent& event) override {
        if (!m_visible || !m_enabled) return false;

        // Delegar eventos ao botão primeiro
        if (m_sendButton && m_sendButton->OnEvent(event)) {
            return true;
        }

        // Captura digitação de caracteres Unicode
        if (event.kind == jay::engine::InputEventKind::CharInput) {
            if (event.codepoint >= 32) {
                m_buffer += static_cast<char>(event.codepoint);
                MarkLayoutDirty();
                event.handled = true;
                return true;
            }
        }

        // Captura teclas Backspace e Enter
        if (event.kind == jay::engine::InputEventKind::KeyPress) {
            if (event.key == 259) { // KEY_BACKSPACE
                if (!m_buffer.empty()) {
                    m_buffer.pop_back();
                    MarkLayoutDirty();
                    event.handled = true;
                    return true;
                }
            } else if (event.key == 257) { // KEY_ENTER
                SubmitText();
                event.handled = true;
                return true;
            }
        }

        return false;
    }

private:
    std::shared_ptr<ChatViewModel>                 m_viewModel;
    std::unique_ptr<jay::shared::widgets::Button> m_sendButton;
    std::string                                   m_buffer;

    void SubmitText() {
        if (m_buffer.empty() || !m_viewModel) return;
        m_viewModel->SendMessage(m_buffer);
        m_buffer.clear();
        MarkLayoutDirty();
    }
};

} // namespace jay::features::chat
