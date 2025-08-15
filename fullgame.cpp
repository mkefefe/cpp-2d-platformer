#include <SDL2/SDL.h>
#include <vector>
#include <algorithm>
#include <cmath>

const int SCREEN_W = 480;
const int SCREEN_H = 270;
const float DT = 1.0f / 60.0f;

struct ParallaxLayer {
    float speed;
    SDL_Color color;
};

static const ParallaxLayer LAYERS[] = {
    {0.05f, {40, 60, 90, 255}},
    {0.2f,  {60, 90, 120, 255}},
    {0.45f, {80, 120, 150, 255}},
    {0.75f, {100, 150, 180, 255}}
};

struct Player {
    float x, y;
    float vx, vy;
    bool onGround;
    float dashTimer;
    float dashCooldown;
    bool dashing;
    int hpSegments;
};

int main(int argc, char** argv) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow("Full Game Demo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_W * 2, SCREEN_H * 2, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    Player player;
    player.x = 50.0f;
    player.y = SCREEN_H - 40.0f;
    player.vx = 0.0f;
    player.vy = 0.0f;
    player.onGround = true;
    player.dashTimer = 0.0f;
    player.dashCooldown = 0.0f;
    player.dashing = false;
    player.hpSegments = 20; // 5 hearts * 4 segments

    std::vector<SDL_Rect> coins;
    coins.push_back({200, SCREEN_H - 60, 12, 12});
    coins.push_back({300, SCREEN_H - 60, 12, 12});
    coins.push_back({380, SCREEN_H - 60, 12, 12});
    int coinCount = 0;

    float cameraX = 0.0f;

    Uint32 lastTick = SDL_GetTicks();
    float accumulator = 0.0f;
    bool running = true;

    bool prevJump = false;

    while (running) {
        Uint32 now = SDL_GetTicks();
        float frameTime = (now - lastTick) / 1000.0f;
        lastTick = now;
        accumulator += frameTime;

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
        }
        const Uint8* keys = SDL_GetKeyboardState(NULL);
        while (accumulator >= DT) {
            accumulator -= DT;
            // Update dash timers
            if (player.dashTimer > 0.0f) {
                player.dashTimer -= DT;
                if (player.dashTimer <= 0.0f) {
                    player.dashTimer = 0.0f;
                    player.dashing = false;
                }
            }
            if (player.dashCooldown > 0.0f) {
                player.dashCooldown -= DT;
                if (player.dashCooldown < 0.0f) player.dashCooldown = 0.0f;
            }
            // Movement input
            float ax = 0.0f;
            if (!player.dashing) {
                if (keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A]) ax -= (player.onGround ? 2400.0f : 1400.0f);
                if (keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D]) ax += (player.onGround ? 2400.0f : 1400.0f);
            }
            // Dash input
            if ((keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT]) && player.dashCooldown <= 0.0f && !player.dashing) {
                float dx = 0.0f, dy = 0.0f;
                if (keys[SDL_SCANCODE_UP] || keys[SDL_SCANCODE_W]) dy -= 1.0f;
                if (keys[SDL_SCANCODE_DOWN] || keys[SDL_SCANCODE_S]) dy += 1.0f;
                if (keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A]) dx -= 1.0f;
                if (keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D]) dx += 1.0f;
                if (dx != 0.0f || dy != 0.0f) {
                    float len = std::sqrt(dx * dx + dy * dy);
                    dx /= len;
                    dy /= len;
                    player.vx = dx * 480.0f;
                    player.vy = dy * 480.0f;
                    player.dashTimer = 0.16f;
                    player.dashCooldown = player.onGround ? 0.45f : 0.65f;
                    player.dashing = true;
                }
            }
            if (!player.dashing) {
                player.vx += ax * DT;
                // Ground or air friction
                float decel = player.onGround ? 2800.0f : 1000.0f;
                if (ax == 0.0f) {
                    float friction = decel * DT;
                    if (player.vx > 0.0f) {
                        player.vx -= friction;
                        if (player.vx < 0.0f) player.vx = 0.0f;
                    } else if (player.vx < 0.0f) {
                        player.vx += friction;
                        if (player.vx > 0.0f) player.vx = 0.0f;
                    }
                }
                // Clamp speed
                float vmax = 220.0f;
                if (player.vx > vmax) player.vx = vmax;
                if (player.vx < -vmax) player.vx = -vmax;
            }
            // Jump logic
            bool jumpPressed = keys[SDL_SCANCODE_SPACE];
            if (jumpPressed && !prevJump && player.onGround && !player.dashing) {
                player.vy = -620.0f;
                player.onGround = false;
            }
            prevJump = jumpPressed;
            // Gravity
            if (!player.dashing) {
                player.vy += 2100.0f * DT;
                if (player.vy > 900.0f) player.vy = 900.0f;
            }
            // Integrate position
            player.x += player.vx * DT;
            player.y += player.vy * DT;
            // Boundary conditions
            if (player.x < 0.0f) { player.x = 0.0f; player.vx = 0.0f; }
            if (player.x + 20.0f > SCREEN_W) { player.x = SCREEN_W - 20.0f; player.vx = 0.0f; }
            // Ground collision
            float groundY = SCREEN_H - 20.0f;
            if (player.y + 20.0f >= groundY) {
                player.y = groundY - 20.0f;
                player.vy = 0.0f;
                player.onGround = true;
            } else {
                player.onGround = false;
            }
            // Coin collection
            for (auto it = coins.begin(); it != coins.end(); ) {
                SDL_Rect pRect = { (int)player.x, (int)player.y, 20, 20 };
                if (SDL_HasIntersection(&pRect, &*it)) {
                    coinCount++;
                    it = coins.erase(it);
                } else {
                    ++it;
                }
            }
            // Camera follow
            float targetCam = player.x - SCREEN_W * 0.5f;
            cameraX += (targetCam - cameraX) * 0.1f;
        }
        // Render scene
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        // Parallax layers
        for (const auto &layer : LAYERS) {
            float offset = std::fmod(cameraX * layer.speed, (float)SCREEN_W);
            if (offset < 0.0f) offset += SCREEN_W;
            SDL_SetRenderDrawColor(renderer, layer.color.r, layer.color.g, layer.color.b, layer.color.a);
            SDL_Rect r1 = { (int)-offset, 0, SCREEN_W, SCREEN_H };
            SDL_Rect r2 = { (int)-offset + SCREEN_W, 0, SCREEN_W, SCREEN_H };
            SDL_RenderFillRect(renderer, &r1);
            SDL_RenderFillRect(renderer, &r2);
        }
        // Ground
        SDL_SetRenderDrawColor(renderer, 50, 35, 25, 255);
        SDL_Rect ground = { 0, SCREEN_H - 20, SCREEN_W, 20 };
        SDL_RenderFillRect(renderer, &ground);
        // Coins
        SDL_SetRenderDrawColor(renderer, 255, 223, 0, 255);
        for (const auto &c : coins) {
            SDL_Rect draw = c;
            draw.x -= (int)cameraX;
            SDL_RenderFillRect(renderer, &draw);
        }
        // Player
        SDL_SetRenderDrawColor(renderer, 100, 200, 255, 255);
        SDL_Rect plRect = { (int)(player.x - cameraX), (int)player.y, 20, 20 };
        SDL_RenderFillRect(renderer, &plRect);
        // HUD Hearts
        int hearts = player.hpSegments / 4;
        for (int i = 0; i < hearts; ++i) {
            int filled = std::max(0, std::min(4, player.hpSegments - i * 4));
            for (int s = 0; s < 4; ++s) {
                SDL_Rect seg = { 10 + i * 22 + s * 4, 10, 4, 10 };
                if (s < filled) SDL_SetRenderDrawColor(renderer, 200, 30, 30, 255);
                else SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
                SDL_RenderFillRect(renderer, &seg);
            }
        }
        // Coin HUD icons (top right)
        SDL_SetRenderDrawColor(renderer, 255, 223, 0, 255);
        for (int i = 0; i < coinCount && i < 5; ++i) {
            SDL_Rect icon = { SCREEN_W - 10 - i * 12, 10, 8, 8 };
            SDL_RenderFillRect(renderer, &icon);
        }
        // Dash cooldown bar
        float ratio = 0.0f;
        if (player.dashCooldown > 0.0f) {
            float denom = player.onGround ? 0.45f : 0.65f;
            ratio = player.dashCooldown / denom;
            if (ratio > 1.0f) ratio = 1.0f;
        }
        int barW = 100;
        int barH = 6;
        SDL_Rect barBg = { (SCREEN_W - barW) / 2, SCREEN_H - barH - 6, barW, barH };
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        SDL_RenderFillRect(renderer, &barBg);
        SDL_Rect barFill = { barBg.x, barBg.y, (int)(barW * (1.0f - ratio)), barH };
        SDL_SetRenderDrawColor(renderer, 30, 144, 255, 255);
        SDL_RenderFillRect(renderer, &barFill);

        SDL_RenderPresent(renderer);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
