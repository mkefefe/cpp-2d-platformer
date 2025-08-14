#include <SDL2/SDL.h>
#include <algorithm>

const int SCREEN_W = 480;
const int SCREEN_H = 270;
const float DT = 1.0f / 60.0f;

int main(int argc, char** argv) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("HUD Demo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_W*2, SCREEN_H*2, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    const int maxSegments = 20; // 5 hearts * 4 segments each
    int hpSegments = maxSegments; // start full

    Uint32 lastTicks = SDL_GetTicks();
    float accumulator = 0.0f;
    bool running = true;

    while (running) {
        Uint32 now = SDL_GetTicks();
        float frameTime = (now - lastTicks) / 1000.0f;
        lastTicks = now;
        accumulator += frameTime;

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = false;
            } else if (e.type == SDL_KEYDOWN) {
                // press H to take damage, J to heal
                if (e.key.keysym.sym == SDLK_h) {
                    if (hpSegments > 0) hpSegments--;
                } else if (e.key.keysym.sym == SDLK_j) {
                    if (hpSegments < maxSegments) hpSegments++;
                }
            }
        }

        while (accumulator >= DT) {
            // nothing to update; we only simulate time for deterministic behavior
            accumulator -= DT;
        }

        // Render
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Draw hearts (top-left)
        int hearts = maxSegments / 4;
        int xStart = 10;
        int yStart = 10;
        int segmentWidth = 4;
        int segmentHeight = 10;
        int heartSpacing = 6;
        for (int i = 0; i < hearts; ++i) {
            int filled = std::max(0, std::min(4, hpSegments - i * 4));
            for (int s = 0; s < 4; ++s) {
                SDL_Rect segRect = { xStart + i * (segmentWidth * 4 + heartSpacing) + s * segmentWidth, yStart, segmentWidth, segmentHeight };
                if (s < filled) {
                    SDL_SetRenderDrawColor(renderer, 200, 30, 30, 255);
                } else {
                    SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
                }
                SDL_RenderFillRect(renderer, &segRect);
            }
        }

        // Draw dash meter at bottom-center as example ability meter
        float ratio = 0.5f; // half full
        int barW = 100;
        int barH = 6;
        int barX = (SCREEN_W - barW) / 2;
        int barY = SCREEN_H - barH - 10;
        SDL_Rect bg = { barX, barY, barW, barH };
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        SDL_RenderFillRect(renderer, &bg);
        SDL_Rect fill = { barX, barY, (int)(barW * ratio), barH };
        SDL_SetRenderDrawColor(renderer, 30, 144, 255, 255);
        SDL_RenderFillRect(renderer, &fill);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
