module;
#include <memory>
#include <iostream>
#include <raylib.h>
export module renderer;

import avatar;

export namespace jay {
class Renderer {
public:
    explicit Renderer(std::shared_ptr<Avatar> avatar) : m_avatar(std::move(avatar)) {}
    ~Renderer() { if (IsWindowReady()) CloseWindow(); }
    void Run() {
        InitWindow(400, 400, "Jay Frontend");
        SetTargetFPS(60);
        while (!WindowShouldClose()) {
            BeginDrawing(); ClearBackground(RAYWHITE);
            DrawAvatarState();
            EndDrawing();
        }
    }
private:
    std::shared_ptr<Avatar> m_avatar;
    void DrawAvatarState() {
        State currentState = m_avatar->GetState();
        const int cx = GetScreenWidth() / 2, cy = GetScreenHeight() / 2;
        switch (currentState) {
            case State::Idle: DrawCircle(cx, cy, 50.0f, LIGHTGRAY); DrawText("Jay (Idle)", cx - 40, cy + 70, 20, DARKGRAY); break;
            case State::Thinking: DrawCircle(cx, cy, 50.0f, ORANGE); DrawText("Jay (Thinking...)", cx - 70, cy + 70, 20, DARKGRAY); break;
            case State::Executing: DrawCircle(cx, cy, 50.0f, GREEN); DrawText("Jay (Executing)", cx - 60, cy + 70, 20, DARKGRAY); break;
            case State::Sleeping: DrawCircle(cx, cy, 50.0f, DARKBLUE); DrawText("Jay (Sleeping)", cx - 60, cy + 70, 20, LIGHTGRAY); break;
        }
        if (auto anim = m_avatar->ConsumeNextAnimation()) std::cout << "Anim: " << *anim << "\n";
    }
};
}
