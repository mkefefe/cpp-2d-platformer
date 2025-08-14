#include <SDL2/SDL.h>
#include <cmath>
#include <array>

// Unified demo: combines parallax background, player movement with dash,
// and a simple enemy AI. This serves as a step toward the complete game.
// It demonstrates deterministic fixed-timestep updates and rendering order.

constexpr int SCREEN_W = 480;
constexpr int SCREEN_H = 270;
constexpr float DT = 1.0f / 60.0f;

// Parallax layer definition
struct ParallaxLayer {
    float speed;
    SDL_Color color;
};

static const std::array<ParallaxLayer,4> PARALLAX = {{
    {0.05f,{135,206,235,255}}, // sky
    {0.2f, {100,149,237,255}}, // far
    {0.45f,{70,130,180,255}}, // mid
    {0.75f,{65,105,225,255}}  // near
}};

// Player with dash
struct Player {
    float x,y;
    float vx, vy;
    bool onGround;
    bool dashing;
    float dashTimer;
    float dashCooldown;
    float dashDirX;
    float dashDirY;
};

// Enemy with simple AI
enum class EnemyState{ Patrol, Telegraph, Attack, Recover };
struct Enemy {
    float x,y;
    float vx;
    EnemyState state;
    float timer;
};

int main(int argc, char* argv[]){
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Unified Demo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_W*2, SCREEN_H*2, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    SDL_RenderSetLogicalSize(renderer, SCREEN_W, SCREEN_H);

    Player player{SCREEN_W/4.0f, SCREEN_H-30.0f, 0.0f, 0.0f, true, false, 0.0f, 0.0f, 1.0f, 0.0f};
    Enemy enemy{SCREEN_W*0.8f, SCREEN_H-30.0f, -40.0f, EnemyState::Patrol, 0.0f};
    float patrolLeft = SCREEN_W*0.6f;
    float patrolRight = SCREEN_W*0.9f;
    float cameraX = 0.0f;

    bool quit=false;
    Uint32 lastTick = SDL_GetTicks();
    float accumulator = 0.0f;
    while(!quit){
        Uint32 now = SDL_GetTicks();
        float frameTime = (now - lastTick)/1000.0f;
        if(frameTime>0.25f) frameTime=0.25f;
        lastTick = now;
        accumulator += frameTime;

        SDL_Event e;
        while(SDL_PollEvent(&e)){
            if(e.type==SDL_QUIT) quit=true;
        }
        const Uint8* keys = SDL_GetKeyboardState(nullptr);

        while(accumulator>=DT){
            // Player movement if not dashing
            if(!player.dashing){
                float accel = 2400.0f;
                float decel = 2800.0f;
                float target = 0.0f;
                if(keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT]) target -= 1.0f;
                if(keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) target += 1.0f;
                if(target != 0.0f){
                    player.vx += target*accel*DT;
                    if(player.vx > 220.0f) player.vx = 220.0f;
                    if(player.vx < -220.0f) player.vx = -220.0f;
                } else {
                    if(player.vx > 0.0f){
                        player.vx -= decel*DT;
                        if(player.vx < 0.0f) player.vx = 0.0f;
                    } else if(player.vx < 0.0f){
                        player.vx += decel*DT;
                        if(player.vx > 0.0f) player.vx = 0.0f;
                    }
                }
            }
            // Jump
            if(!player.dashing && player.onGround && keys[SDL_SCANCODE_SPACE]){
                player.vy = -620.0f;
                player.onGround = false;
            }
            // Gravity
            if(!player.onGround){
                player.vy += 2100.0f * DT;
                if(player.vy > 900.0f) player.vy = 900.0f;
            }
            // Dash input
            bool dashInput = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];
            if(!player.dashing && player.dashCooldown<=0.0f && dashInput){
                float dirX = 0.0f;
                float dirY = 0.0f;
                if(keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP]) dirY -= 1.0f;
                if(keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN]) dirY += 1.0f;
                if(keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT]) dirX -= 1.0f;
                if(keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) dirX += 1.0f;
                if(dirX==0.0f && dirY==0.0f) dirX = (player.vx>=0?1.0f:-1.0f);
                float len = std::sqrt(dirX*dirX + dirY*dirY);
                if(len != 0.0f){ dirX/=len; dirY/=len; }
                player.dashing = true;
                player.dashTimer = 0.16f;
                player.dashCooldown = player.onGround?0.45f:0.65f;
                player.dashDirX = dirX;
                player.dashDirY = dirY;
            }
            // Dash update
            if(player.dashing){
                player.dashTimer -= DT;
                if(player.dashTimer > 0.0f){
                    player.vx = player.dashDirX * 480.0f;
                    player.vy = player.dashDirY * 480.0f;
                } else {
                    player.dashing = false;
                    player.vx = player.dashDirX * 120.0f;
                    if(player.dashDirY > 0.0f) player.vy = 0.0f;
                }
            } else {
                if(player.dashCooldown > 0.0f) player.dashCooldown -= DT;
            }
            // Integrate player
            player.x += player.vx * DT;
            player.y += player.vy * DT;
            if(player.y >= SCREEN_H-30.0f){
                player.y = SCREEN_H-30.0f;
                player.vy = 0.0f;
                player.onGround = true;
            } else {
                player.onGround = false;
            }
            if(player.x < 0.0f) player.x = 0.0f;
            if(player.x > 1024.0f) player.x = 1024.0f;
            // Enemy AI
            float dist = std::abs(enemy.x - player.x);
            switch(enemy.state){
                case EnemyState::Patrol:
                    enemy.x += enemy.vx * DT;
                    if((enemy.vx<0 && enemy.x <= patrolLeft) || (enemy.vx>0 && enemy.x >= patrolRight)){
                        enemy.vx = -enemy.vx;
                    }
                    if(dist < 60.0f){
                        enemy.state = EnemyState::Telegraph;
                        enemy.timer = 0.25f;
                    }
                    break;
                case EnemyState::Telegraph:
                    enemy.timer -= DT;
                    if(enemy.timer <= 0.0f){
                        enemy.state = EnemyState::Attack;
                        enemy.vx = (player.x < enemy.x ? -1.0f : 1.0f) * 300.0f;
                        enemy.timer = 0.12f;
                    }
                    break;
                case EnemyState::Attack:
                    enemy.timer -= DT;
                    enemy.x += enemy.vx * DT;
                    if(enemy.timer <= 0.0f){
                        enemy.state = EnemyState::Recover;
                        enemy.vx = (enemy.x < (patrolLeft+patrolRight)/2 ? 40.0f : -40.0f);
                        enemy.timer = 0.4f;
                    }
                    break;
                case EnemyState::Recover:
                    enemy.timer -= DT;
                    enemy.x += enemy.vx * DT;
                    if(enemy.timer <= 0.0f){
                        enemy.state = EnemyState::Patrol;
                        enemy.vx = (enemy.x < (patrolLeft+patrolRight)/2 ? 40.0f : -40.0f);
                    }
                    break;
            }
            // Camera follow
            float targetCam = player.x - SCREEN_W*0.5f;
            cameraX += (targetCam - cameraX) * 0.1f;
            accumulator -= DT;
        }
        // Render
        SDL_SetRenderDrawColor(renderer, 0,0,0,255);
        SDL_RenderClear(renderer);
        // Draw parallax layers
        for(const auto& layer: PARALLAX){
            float offset = fmodf(cameraX * layer.speed, (float)SCREEN_W);
            if(offset < 0) offset += SCREEN_W;
            SDL_SetRenderDrawColor(renderer, layer.color.r, layer.color.g, layer.color.b, layer.color.a);
            SDL_Rect rect1{ (int)-offset, 0, SCREEN_W, SCREEN_H};
            SDL_Rect rect2{ rect1.x + SCREEN_W, 0, SCREEN_W, SCREEN_H};
            SDL_RenderFillRect(renderer, &rect1);
            SDL_RenderFillRect(renderer, &rect2);
        }
        // Ground
        SDL_SetRenderDrawColor(renderer, 50,205,50,255);
        SDL_Rect ground{0, SCREEN_H-20, SCREEN_W, 20};
        SDL_RenderFillRect(renderer, &ground);
        // Enemy
        if(enemy.state == EnemyState::Telegraph){
            SDL_SetRenderDrawColor(renderer, 255,165,0,255);
        } else if(enemy.state == EnemyState::Attack){
            SDL_SetRenderDrawColor(renderer, 255,0,0,255);
        } else {
            SDL_SetRenderDrawColor(renderer, 139,0,0,255);
        }
        SDL_Rect eRect{ (int)(enemy.x - cameraX) - 10, (int)(enemy.y) -20, 20, 40};
        SDL_RenderFillRect(renderer, &eRect);
        // Player
        SDL_SetRenderDrawColor(renderer, 70,130,180,255);
        SDL_Rect pRect{ (int)(player.x - cameraX) - 8, (int)(player.y) -20, 16, 40};
        SDL_RenderFillRect(renderer, &pRect);
        // Dash cooldown bar
        SDL_SetRenderDrawColor(renderer,80,80,80,255);
        SDL_Rect barBg{10, SCREEN_H-15, 100, 5};
        SDL_RenderFillRect(renderer, &barBg);
        float dashRatio = (player.dashCooldown>0.0f? (1.0f - player.dashCooldown/(player.onGround?0.45f:0.65f)):1.0f);
        SDL_SetRenderDrawColor(renderer,30,144,255,255);
        SDL_Rect bar{10, SCREEN_H-15, (int)(dashRatio * 100.0f), 5};
        SDL_RenderFillRect(renderer, &bar);
        SDL_RenderPresent(renderer);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
