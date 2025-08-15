#include <SDL2/SDL.h>
#include <vector>
#include <cmath>

const int SCREEN_W = 480;
const int SCREEN_H = 270;
const float DT = 1.0f / 60.0f;

struct Player {
    float x, y;
    float vx, vy;
    bool onGround;
};

int main(int argc, char** argv) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Coin Demo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_W * 2, SCREEN_H * 2, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    Player player;
    player.x = SCREEN_W / 4.0f;
    player.y = SCREEN_H - 40.0f;
    player.vx = 0.0f;
    player.vy = 0.0f;
    player.onGround = true;

    // coins positions
    std::vector<SDL_Rect> coins;
    coins.push_back({200, SCREEN_H - 60, 16, 16});
    coins.push_back({300, SCREEN_H - 60, 16, 16});
    coins.push_back({400, SCREEN_H - 60, 16, 16});
    int coinCount = 0;

    Uint32 lastTick = SDL_GetTicks();
    float accumulator = 0.0f;
    bool running = true;

    while (running) {
        Uint32 now = SDL_GetTicks();
        float frameTime = (now - lastTick) / 1000.0f;
        lastTick = now;
        accumulator += frameTime;

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = false;
            }
        }

        const Uint8* keys = SDL_GetKeyboardState(NULL);

        while (accumulator >= DT) {
            accumulator -= DT;
            float ax = 0.0f;
            if (keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A]) ax = -600.0f;
            if (keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D]) ax = 600.0f;

            player.vx += ax * DT;
            if (player.vx > 200.0f) player.vx = 200.0f;
            if (player.vx < -200.0f) player.vx = -200.0f;

            // friction
            if (ax == 0.0f && player.onGround) {
                player.vx *= 0.8f;
                if (std::fabs(player.vx) < 5.0f) player.vx = 0.0f;
            }

            // jump
            if ((keys[SDL_SCANCODE_SPACE] || keys[SDL_SCANCODE_W]) && player.onGround) {
                player.vy = -550.0f;
                player.onGround = false;
            }

            // gravity
            player.vy += 2100.0f * DT;

            // integrate
            player.x += player.vx * DT;
            player.y += player.vy * DT;

            // bounds
            if (player.x < 0.0f) {
                player.x = 0.0f;
                player.vx = 0.0f;
            }
            if (player.x + 20.0f > SCREEN_W) {
                player.x = SCREEN_W - 20.0f;
                player.vx = 0.0f;
            }

            // ground collision
            if (player.y + 20.0f >= SCREEN_H - 20.0f) {
                player.y = SCREEN_H - 20.0f - 20.0f;
                player.vy = 0.0f;
                player.onGround = true;
            }

            // coin collision
            for (auto it = coins.begin(); it != coins.end(); ) {
                SDL_Rect pRect = { (int)player.x, (int)player.y, 20, 20 };
                if (SDL_HasIntersection(&pRect, &*it)) {
                    coinCount++;
                    it = coins.erase(it);
                } else {
                    ++it;
                }
            }
        }

        // Render
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // draw ground
        SDL_SetRenderDrawColor(renderer, 70, 60, 50, 255);
        SDL_Rect ground = { 0, SCREEN_H - 20, SCREEN_W, 20 };
        SDL_RenderFillRect(renderer, &ground);

        // draw player
        SDL_SetRenderDrawColor(renderer, 100, 200, 255, 255);
        SDL_Rect pRect = { (int)player.x, (int)player.y, 20, 20 };
        SDL_RenderFillRect(renderer, &pRect);

        // draw coins
        SDL_SetRenderDrawColor(renderer, 255, 223, 0, 255);
        for (const auto& c : coins) {
            SDL_RenderFillRect(renderer, &c);
        }

        // draw coin count icons at top right (limit 5)
        SDL_SetRenderDrawColor(renderer, 255, 223, 0, 255);
        for (int i = 0; i < coinCount && i < 5; ++i) {
            SDL_Rect icon = { SCREEN_W - 10 - i * 12, 10, 8, 8 };
            SDL_RenderFillRect(renderer, &icon);
        }

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
