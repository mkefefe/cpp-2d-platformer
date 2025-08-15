#include <SDL2/SDL.h>
#include <cmath>

const int SCREEN_W = 480;
const int SCREEN_H = 270;
const float DT = 1.0f / 60.0f;

struct Player {
    float x, y;
    float vx, vy;
    bool onGround;
    bool facingRight;
    float attackTimer;
};

struct Enemy {
    float x, y;
    bool alive;
};

int main(int argc, char** argv) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow("Attack Demo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_W * 2, SCREEN_H * 2, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    Player player;
    player.x = SCREEN_W / 4.0f;
    player.y = SCREEN_H - 40.0f;
    player.vx = 0.0f;
    player.vy = 0.0f;
    player.onGround = true;
    player.facingRight = true;
    player.attackTimer = 0.0f;

    Enemy enemy;
    enemy.x = SCREEN_W * 0.6f;
    enemy.y = SCREEN_H - 40.0f;
    enemy.alive = true;

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
            if (e.type == SDL_QUIT) running = false;
        }
        const Uint8* keys = SDL_GetKeyboardState(NULL);
        while (accumulator >= DT) {
            accumulator -= DT;
            float ax = 0.0f;
            if (keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A]) {
                ax = -600.0f;
                player.facingRight = false;
            }
            if (keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D]) {
                ax = 600.0f;
                player.facingRight = true;
            }
            player.vx += ax * DT;
            if (player.vx > 200.0f) player.vx = 200.0f;
            if (player.vx < -200.0f) player.vx = -200.0f;
            if (ax == 0.0f && player.onGround) {
                player.vx *= 0.8f;
                if (std::fabs(player.vx) < 5.0f) player.vx = 0.0f;
            }
            if (keys[SDL_SCANCODE_SPACE] && player.onGround) {
                player.vy = -550.0f;
                player.onGround = false;
            }
            player.vy += 2100.0f * DT;
            player.x += player.vx * DT;
            player.y += player.vy * DT;
            if (player.x < 0.0f) { player.x = 0.0f; player.vx = 0.0f; }
            if (player.x + 20.0f > SCREEN_W) { player.x = SCREEN_W - 20.0f; player.vx = 0.0f; }
            if (player.y + 20.0f >= SCREEN_H - 20.0f) {
                player.y = SCREEN_H - 20.0f - 20.0f;
                player.vy = 0.0f;
                player.onGround = true;
            }
            // Attack input
            if (keys[SDL_SCANCODE_J] && player.attackTimer <= 0.0f) {
                player.attackTimer = 0.18f;
            }
            if (player.attackTimer > 0.0f) {
                player.attackTimer -= DT;
                if (player.attackTimer < 0.0f) player.attackTimer = 0.0f;
            }
            // Check attack hit
            if (player.attackTimer > 0.0f && enemy.alive) {
                SDL_Rect attackRect;
                if (player.facingRight) {
                    attackRect = { (int)(player.x + 20), (int)player.y + 5, 20, 10 };
                } else {
                    attackRect = { (int)(player.x - 20), (int)player.y + 5, 20, 10 };
                }
                SDL_Rect enemyRect = { (int)enemy.x, (int)enemy.y, 20, 20 };
                if (SDL_HasIntersection(&attackRect, &enemyRect)) {
                    enemy.alive = false;
                }
            }
        }
        // Render
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        // ground
        SDL_SetRenderDrawColor(renderer, 70, 60, 50, 255);
        SDL_Rect ground = { 0, SCREEN_H - 20, SCREEN_W, 20 };
        SDL_RenderFillRect(renderer, &ground);
        // enemy
        if (enemy.alive) {
            SDL_SetRenderDrawColor(renderer, 200, 40, 40, 255);
            SDL_Rect eRect = { (int)enemy.x, (int)enemy.y, 20, 20 };
            SDL_RenderFillRect(renderer, &eRect);
        }
        // player
        SDL_SetRenderDrawColor(renderer, 100, 200, 255, 255);
        SDL_Rect pRect = { (int)player.x, (int)player.y, 20, 20 };
        SDL_RenderFillRect(renderer, &pRect);
        // attack box
        if (player.attackTimer > 0.0f) {
            SDL_SetRenderDrawColor(renderer, 255, 200, 50, 255);
            SDL_Rect aRect;
            if (player.facingRight) {
                aRect = { (int)(player.x + 20), (int)player.y + 5, 20, 10 };
            } else {
                aRect = { (int)(player.x - 20), (int)player.y + 5, 20, 10 };
            }
            SDL_RenderFillRect(renderer, &aRect);
        }
        SDL_RenderPresent(renderer);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
