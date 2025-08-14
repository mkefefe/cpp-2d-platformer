#include <SDL2/SDL.h>
#include <cmath>

// Simple Enemy Demo implementing a grunt swordsman with patrol and attack telegraph.
// This demonstrates Sections 11 and 12.1 of the specification. The enemy patrols
// between two points and, when the player comes within range, telegraphs an attack
// before lunging forward. Collisions are simplified for demonstration purposes.

constexpr int SCREEN_W = 480;
constexpr int SCREEN_H = 270;
constexpr float DT = 1.0f / 60.0f;

struct Player {
    float x, y;
    float vx, vy;
    bool onGround;
};

enum class EnemyState { Patrol, Telegraph, Attack, Recover };

struct Enemy {
    float x, y;
    float vx;
    EnemyState state;
    float timer;
};

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Enemy Demo",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_W * 2, SCREEN_H * 2, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_RenderSetLogicalSize(renderer, SCREEN_W, SCREEN_H);

    Player player{ SCREEN_W/4.0f, SCREEN_H - 30.0f, 0.0f, 0.0f, true };
    Enemy enemy{ SCREEN_W*0.75f, SCREEN_H - 30.0f, -40.0f, EnemyState::Patrol, 0.0f };
    float patrolLeft = SCREEN_W*0.6f;
    float patrolRight = SCREEN_W*0.9f;
    float cameraX = 0.0f;

    bool quit = false;
    Uint32 lastTick = SDL_GetTicks();
    float accumulator = 0.0f;

    while (!quit) {
        Uint32 now = SDL_GetTicks();
        float frameTime = (now - lastTick) / 1000.0f;
        if (frameTime > 0.25f) frameTime = 0.25f;
        lastTick = now;
        accumulator += frameTime;

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) quit = true;
        }

        const Uint8* keys = SDL_GetKeyboardState(nullptr);

        while (accumulator >= DT) {
            // Player movement input
            float moveAccel = 200.0f;
            float decel = 300.0f;
            float target = 0.0f;
            if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT]) target -= 1.0f;
            if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) target += 1.0f;
            if (target != 0.0f) {
                player.vx += target * moveAccel * DT;
                if (player.vx > 200.0f) player.vx = 200.0f;
                if (player.vx < -200.0f) player.vx = -200.0f;
            } else {
                if (player.vx > 0.0f) {
                    player.vx -= decel * DT;
                    if (player.vx < 0.0f) player.vx = 0.0f;
                } else if (player.vx < 0.0f) {
                    player.vx += decel * DT;
                    if (player.vx > 0.0f) player.vx = 0.0f;
                }
            }
            // Jump
            if (player.onGround && keys[SDL_SCANCODE_SPACE]) {
                player.vy = -620.0f;
                player.onGround = false;
            }
            // Gravity
            if (!player.onGround) {
                player.vy += 2100.0f * DT;
                if (player.vy > 900.0f) player.vy = 900.0f;
            }
            // Integrate player position
            player.x += player.vx * DT;
            player.y += player.vy * DT;
            if (player.y >= SCREEN_H - 30.0f) {
                player.y = SCREEN_H - 30.0f;
                player.vy = 0.0f;
                player.onGround = true;
            } else {
                player.onGround = false;
            }
            // Enemy AI
            float dist = std::abs(enemy.x - player.x);
            switch (enemy.state) {
                case EnemyState::Patrol:
                    enemy.x += enemy.vx * DT;
                    if ((enemy.vx < 0 && enemy.x <= patrolLeft) || (enemy.vx > 0 && enemy.x >= patrolRight)) {
                        enemy.vx = -enemy.vx;
                    }
                    if (dist < 60.0f) {
                        enemy.state = EnemyState::Telegraph;
                        enemy.timer = 0.25f;
                    }
                    break;
                case EnemyState::Telegraph:
                    enemy.timer -= DT;
                    if (enemy.timer <= 0.0f) {
                        enemy.state = EnemyState::Attack;
                        enemy.vx = (player.x < enemy.x ? -1.0f : 1.0f) * 300.0f;
                        enemy.timer = 0.12f;
                    }
                    break;
                case EnemyState::Attack:
                    enemy.timer -= DT;
                    enemy.x += enemy.vx * DT;
                    if (enemy.timer <= 0.0f) {
                        enemy.state = EnemyState::Recover;
                        enemy.vx = (enemy.x < (patrolLeft+patrolRight)/2 ? 40.0f : -40.0f);
                        enemy.timer = 0.4f;
                    }
                    break;
                case EnemyState::Recover:
                    enemy.timer -= DT;
                    enemy.x += enemy.vx * DT;
                    if (enemy.timer <= 0.0f) {
                        enemy.state = EnemyState::Patrol;
                        enemy.vx = (enemy.x < (patrolLeft+patrolRight)/2 ? 40.0f : -40.0f);
                    }
                    break;
            }
            // Clamp player x within world
            if (player.x < 0.0f) player.x = 0.0f;
            if (player.x > 1024.0f) player.x = 1024.0f;
            // Camera follow
            float targetCam = player.x - SCREEN_W * 0.5f;
            cameraX += (targetCam - cameraX) * 0.1f;
            accumulator -= DT;
        }
        // Render scene
        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        SDL_RenderClear(renderer);
        // Ground
        SDL_SetRenderDrawColor(renderer, 70, 80, 50, 255);
        SDL_Rect ground{ 0, SCREEN_H - 20, SCREEN_W, 20 };
        SDL_RenderFillRect(renderer, &ground);
        // Player
        SDL_SetRenderDrawColor(renderer, 64, 164, 223, 255);
        SDL_Rect pRect{ static_cast<int>(player.x - cameraX) - 8,
                        static_cast<int>(player.y) - 20,
                        16, 40 };
        SDL_RenderFillRect(renderer, &pRect);
        // Enemy color based on state
        if (enemy.state == EnemyState::Telegraph) {
            SDL_SetRenderDrawColor(renderer, 255, 165, 0, 255);
        } else if (enemy.state == EnemyState::Attack) {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 139, 0, 0, 255);
        }
        SDL_Rect eRect{ static_cast<int>(enemy.x - cameraX) - 10,
                        static_cast<int>(enemy.y) - 20,
                        20, 40 };
        SDL_RenderFillRect(renderer, &eRect);
        SDL_RenderPresent(renderer);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
