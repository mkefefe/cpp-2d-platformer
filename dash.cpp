#include <SDL2/SDL.h>
#include <cmath>

// Dash Demo implementing player dash mechanics from Section 10.
// This example shows a player that can run and perform an 8-way dash
// with invulnerability frames and cooldown. A simple fixed-timestep
// loop ensures deterministic updates (Section 0).

constexpr int SCREEN_W = 480;
constexpr int SCREEN_H = 270;
constexpr float DT      = 1.0f / 60.0f;

// Player struct holding state for dash
struct Player {
    float x, y;
    float vx, vy;
    bool onGround;
    bool dashing;
    float dashTimer;
    float dashCooldown;
    float dashDirX;
    float dashDirY;
};

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("SDL_Init Error: %s", SDL_GetError());
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow("Dash Demo",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_W * 2, SCREEN_H * 2, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_RenderSetLogicalSize(renderer, SCREEN_W, SCREEN_H);

    Player player{ SCREEN_W / 2.0f, SCREEN_H - 50.0f, 0.0f, 0.0f, true,
                   false, 0.0f, 0.0f, 0.0f, 0.0f };
    float cameraX = 0.0f;

    bool quit = false;
    Uint32 lastTick = SDL_GetTicks();
    float accumulator = 0.0f;

    const float dashDuration   = 0.16f; // seconds (Section 10)
    const float dashSpeed      = 480.0f;
    const float dashCooldownG  = 0.45f;
    const float dashCooldownA  = 0.65f;

    while (!quit) {
        Uint32 now = SDL_GetTicks();
        float frameTime = (now - lastTick) / 1000.0f;
        if (frameTime > 0.25f) frameTime = 0.25f;
        lastTick = now;
        accumulator += frameTime;

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
        }

        const Uint8* keys = SDL_GetKeyboardState(nullptr);

        while (accumulator >= DT) {
            // Horizontal movement if not dashing
            if (!player.dashing) {
                float accel = 2400.0f; // ground acceleration (Section 7)
                float decel = 2800.0f;
                float target = 0.0f;
                if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT]) {
                    target -= 1.0f;
                }
                if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) {
                    target += 1.0f;
                }
                // accelerate towards target
                if (target != 0.0f) {
                    player.vx += target * accel * DT;
                    // clamp velocity
                    if (player.vx > 220.0f) player.vx = 220.0f;
                    if (player.vx < -220.0f) player.vx = -220.0f;
                } else {
                    // apply deceleration
                    if (player.vx > 0.0f) {
                        player.vx -= decel * DT;
                        if (player.vx < 0.0f) player.vx = 0.0f;
                    } else if (player.vx < 0.0f) {
                        player.vx += decel * DT;
                        if (player.vx > 0.0f) player.vx = 0.0f;
                    }
                }
            }

            // Jump (simple, no buffer/coyote)
            if (!player.dashing && player.onGround && (keys[SDL_SCANCODE_SPACE])) {
                player.vy = -620.0f; // jump velocity (Section 8)
                player.onGround = false;
            }

            // Gravity
            const float gravity = 2100.0f;
            if (!player.onGround) {
                player.vy += gravity * DT;
                if (player.vy > 900.0f) player.vy = 900.0f;
            }

            // Dash input (use Shift or K)
            bool dashInput = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT] || keys[SDL_SCANCODE_K];
            if (!player.dashing && player.dashCooldown <= 0.0f && dashInput) {
                // Determine direction from movement keys
                float dirX = 0.0f;
                float dirY = 0.0f;
                if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP]) dirY -= 1.0f;
                if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN]) dirY += 1.0f;
                if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT]) dirX -= 1.0f;
                if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) dirX += 1.0f;
                // default to facing direction if no input
                if (dirX == 0.0f && dirY == 0.0f) {
                    dirX = (player.vx >= 0.0f ? 1.0f : -1.0f);
                }
                // normalize
                float len = std::sqrt(dirX*dirX + dirY*dirY);
                if (len != 0.0f) {
                    dirX /= len;
                    dirY /= len;
                }
                player.dashing = true;
                player.dashTimer = dashDuration;
                player.dashCooldown = player.onGround ? dashCooldownG : dashCooldownA;
                player.dashDirX = dirX;
                player.dashDirY = dirY;
                // shrink hurtbox or set invulnerability here (omitted)
            }

            // Update dash state
            if (player.dashing) {
                player.dashTimer -= DT;
                if (player.dashTimer > 0.0f) {
                    player.vx = player.dashDirX * dashSpeed;
                    player.vy = player.dashDirY * dashSpeed;
                } else {
                    player.dashing = false;
                    // After dash, preserve horizontal momentum but reset vertical
                    player.vx = player.dashDirX * 120.0f;
                    if (player.dashDirY > 0.0f) {
                        player.vy = 0.0f; // don't retain downward dash
                    }
                }
            } else {
                if (player.dashCooldown > 0.0f) {
                    player.dashCooldown -= DT;
                }
            }

            // Integrate positions
            player.x += player.vx * DT;
            player.y += player.vy * DT;

            // Collision with simple floor
            if (player.y >= SCREEN_H - 30.0f) {
                player.y = SCREEN_H - 30.0f;
                player.vy = 0.0f;
                player.onGround = true;
            } else {
                player.onGround = false;
            }

            // Simple world bounds
            if (player.x < 0.0f) player.x = 0.0f;
            if (player.x > 1024.0f) player.x = 1024.0f;

            // Camera follow horizontally
            float targetCam = player.x - SCREEN_W * 0.5f;
            cameraX += (targetCam - cameraX) * 0.1f;

            accumulator -= DT;
        }

        // Render
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);

        // Ground
        SDL_SetRenderDrawColor(renderer, 85, 107, 47, 255);
        SDL_Rect ground{ 0, SCREEN_H - 20, SCREEN_W, 20 };
        SDL_RenderFillRect(renderer, &ground);

        // Player
        SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);
        SDL_Rect pRect{ static_cast<int>(player.x - cameraX) - 10,
                        static_cast<int>(player.y) - 20,
                        20, 40 };
        SDL_RenderFillRect(renderer, &pRect);

        // Dash cooldown bar (HUD Section 17)
        SDL_SetRenderDrawColor(renderer, 70, 130, 180, 255);
        float barWidth = (player.dashCooldown > 0.0f ? (1.0f - player.dashCooldown / (player.onGround ? dashCooldownG : dashCooldownA)) : 1.0f) * 100.0f;
        SDL_Rect barBg{ 10, SCREEN_H - 15, 100, 5 };
        SDL_Rect bar{ 10, SCREEN_H - 15, static_cast<int>(barWidth), 5 };
        SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);
        SDL_RenderFillRect(renderer, &barBg);
        SDL_SetRenderDrawColor(renderer, 30, 144, 255, 255);
        SDL_RenderFillRect(renderer, &bar);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
