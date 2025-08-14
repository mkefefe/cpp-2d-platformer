#include <SDL2/SDL.h>
#include <cstdint>
#include <array>
#include <vector>
#include <cmath>
#include <memory>

//----------------------------------------------------------------------------
// 2D Platformer Implementation Skeleton with Camera
//----------------------------------------------------------------------------
// This updated skeleton adds a simple camera system that smoothly follows the
// player using a critically damped spring (Section 2).  World rendering is
// offset by the camera so that the player remains near the center of the
// screen.  Other systems (physics, input, parallax) remain as in the
// original skeleton, providing a foundation for further development.
//

// Section 0 – Global Targets & Constraints
constexpr float FIXED_DT        = 1.0f / 60.0f;     // Deterministic timestep (dt = 1/60 s)
constexpr int   TILE_SIZE       = 16;               // World units = pixels; tile = 16×16 px
constexpr int   NATIVE_W        = 480;              // Native render target width
constexpr int   NATIVE_H        = 270;              // Native render target height
constexpr float GRAVITY         = 2100.0f;          // Gravity (Section 7/8)
constexpr float MAX_RUN_SPEED   = 220.0f;           // Player Vmax (Section 7)
constexpr float GROUND_ACCEL    = 2400.0f;
constexpr float GROUND_DECEL    = 2800.0f;
constexpr float AIR_ACCEL       = 1400.0f;
constexpr float AIR_DECEL       = 1000.0f;
constexpr float JUMP_V0         = -620.0f;          // Primary jump velocity (Section 8)

// Section 13 – Collision Layers (bitmasks)
enum CollisionLayer : uint8_t {
    TILE_LAYER         = 0,
    PLAYER_HURT_LAYER  = 1,
    PLAYER_ATTACK_LAYER= 2,
    ENEMY_HURT_LAYER   = 3,
    ENEMY_ATTACK_LAYER = 4,
    SENSOR_LAYER       = 5
};

// Simple 2‑D vector for positions/velocities
struct Vec2 {
    float x{}, y{};
};

// Section 5 – Player Visual Design (simple silhouette)
static constexpr int PLAYER_W = 22;
static constexpr int PLAYER_H = 32;
static const std::array<uint32_t, PLAYER_W * PLAYER_H> PLAYER_PIXELS = []{
    std::array<uint32_t, PLAYER_W * PLAYER_H> data{};
    for (int i = 0; i < PLAYER_W * PLAYER_H; ++i) {
        data[i] = 0xFFFFFFFFu;
    }
    return data;
}();

// Section 6 – Player State Machine (skeleton enumerations)
enum class PlayerState {
    Idle,
    Run,
    JumpRise,
    JumpApex,
    Fall,
    Land,
    Dash,
    Hurt,
    Dead
    // Additional states can be added here
};

// Section 15 – Level Definition (tile map)
static constexpr int LEVEL_WIDTH  = 32;
static constexpr int LEVEL_HEIGHT = 16;
static const std::array<uint8_t, LEVEL_WIDTH * LEVEL_HEIGHT> LEVEL_DATA = []{
    std::array<uint8_t, LEVEL_WIDTH * LEVEL_HEIGHT> data{};
    for (int y = 0; y < LEVEL_HEIGHT; ++y) {
        for (int x = 0; x < LEVEL_WIDTH; ++x) {
            data[y * LEVEL_WIDTH + x] = (y == LEVEL_HEIGHT - 1) ? 1 : 0;
        }
    }
    return data;
}();

uint8_t getTile(int x, int y) {
    if (x < 0 || y < 0 || x >= LEVEL_WIDTH || y >= LEVEL_HEIGHT) return 1;
    return LEVEL_DATA[y * LEVEL_WIDTH + x];
}

// Section 2 – Camera (smooth follow)
struct Camera {
    Vec2 position{};
    Vec2 velocity{};
    // Update camera using a critically damped spring toward target
    void update(const Vec2& target, float dt) {
        // Compute desired position so that target appears near center of screen
        Vec2 desired{ target.x - NATIVE_W * 0.5f + PLAYER_W * 0.5f,
                      target.y - NATIVE_H * 0.5f + PLAYER_H * 0.5f };
        float stiffness = 60.0f;      // spring stiffness
        float damping   = 2.0f * std::sqrt(stiffness); // critical damping
        // Spring force
        Vec2 diff{ desired.x - position.x, desired.y - position.y };
        velocity.x += diff.x * stiffness * dt;
        velocity.y += diff.y * stiffness * dt;
        // Damping
        velocity.x *= std::exp(-damping * dt);
        velocity.y *= std::exp(-damping * dt);
        // Integrate
        position.x += velocity.x * dt;
        position.y += velocity.y * dt;
        // Clamp to level bounds (Section 2 – Bounds)
        if (position.x < 0) position.x = 0;
        if (position.y < 0) position.y = 0;
        float maxX = LEVEL_WIDTH * TILE_SIZE - NATIVE_W;
        float maxY = LEVEL_HEIGHT * TILE_SIZE - NATIVE_H;
        if (position.x > maxX) position.x = maxX;
        if (position.y > maxY) position.y = maxY;
    }
};

// Section 7/8 – Player Movement & Jumping
struct Player {
    Vec2 position{};
    Vec2 velocity{};
    PlayerState state{ PlayerState::Idle };
    bool onGround{ false };
    float jumpBufferTimer{ 0.0f };
    float coyoteTimer{ 0.0f };

    void update(float dt) {
        if (jumpBufferTimer > 0.0f) jumpBufferTimer -= dt;
        if (coyoteTimer     > 0.0f) coyoteTimer     -= dt;
        const Uint8* keys = SDL_GetKeyboardState(nullptr);
        bool left  = keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A];
        bool right = keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D];
        bool jump  = keys[SDL_SCANCODE_SPACE];
        float desiredAccel = 0.0f;
        if (left ^ right) {
            desiredAccel = (left ? -1.0f : 1.0f) * (onGround ? GROUND_ACCEL : AIR_ACCEL);
        } else {
            if (onGround) {
                if (velocity.x > 0.0f) desiredAccel = -GROUND_DECEL;
                else if (velocity.x < 0.0f) desiredAccel = GROUND_DECEL;
            } else {
                if (velocity.x > 0.0f) desiredAccel = -AIR_DECEL;
                else if (velocity.x < 0.0f) desiredAccel = AIR_DECEL;
            }
        }
        velocity.x += desiredAccel * dt;
        if (velocity.x >  MAX_RUN_SPEED) velocity.x =  MAX_RUN_SPEED;
        if (velocity.x < -MAX_RUN_SPEED) velocity.x = -MAX_RUN_SPEED;
        if (jump) {
            jumpBufferTimer = 0.09f;
        }
        if (jumpBufferTimer > 0.0f && (onGround || coyoteTimer > 0.0f)) {
            velocity.y = JUMP_V0;
            onGround = false;
            jumpBufferTimer = 0.0f;
            coyoteTimer = 0.0f;
        }
        velocity.y += GRAVITY * dt;
        Vec2 newPos = position;
        newPos.x += velocity.x * dt;
        newPos.y += velocity.y * dt;
        // Y collisions
        if (velocity.y > 0.0f) {
            int bottom = (int)((newPos.y + PLAYER_H) / TILE_SIZE);
            int leftTile  = (int)(newPos.x / TILE_SIZE);
            int rightTile = (int)((newPos.x + PLAYER_W - 1) / TILE_SIZE);
            bool collides = false;
            for (int tx = leftTile; tx <= rightTile; ++tx) {
                if (getTile(tx, bottom) == 1) { collides = true; break; }
            }
            if (collides) {
                newPos.y = bottom * TILE_SIZE - PLAYER_H;
                velocity.y = 0.0f;
                onGround = true;
                coyoteTimer = 0.1f;
            }
        } else if (velocity.y < 0.0f) {
            // upward collision (omitted)
        }
        // X collisions
        int top = (int)(newPos.y / TILE_SIZE);
        int bottomY = (int)((newPos.y + PLAYER_H - 1) / TILE_SIZE);
        if (velocity.x > 0.0f) {
            int rightTile = (int)((newPos.x + PLAYER_W) / TILE_SIZE);
            bool collides = false;
            for (int ty = top; ty <= bottomY; ++ty) {
                if (getTile(rightTile, ty) == 1) { collides = true; break; }
            }
            if (collides) {
                newPos.x = rightTile * TILE_SIZE - PLAYER_W;
                velocity.x = 0.0f;
            }
        } else if (velocity.x < 0.0f) {
            int leftTile = (int)(newPos.x / TILE_SIZE);
            bool collides = false;
            for (int ty = top; ty <= bottomY; ++ty) {
                if (getTile(leftTile, ty) == 1) { collides = true; break; }
            }
            if (collides) {
                newPos.x = (leftTile + 1) * TILE_SIZE;
                velocity.x = 0.0f;
            }
        }
        position = newPos;
        // State update
        if (!onGround) {
            state = (velocity.y < 0.0f) ? PlayerState::JumpRise : PlayerState::Fall;
        } else {
            state = (std::abs(velocity.x) > 1.0f) ? PlayerState::Run : PlayerState::Idle;
        }
    }

    void draw(SDL_Renderer* renderer, float scale, Vec2 cam) const {
        SDL_Rect dst;
        dst.x = (int)((position.x - cam.x) * scale);
        dst.y = (int)((position.y - cam.y) * scale);
        dst.w = (int)(PLAYER_W * scale);
        dst.h = (int)(PLAYER_H * scale);
        SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(
            (void*)PLAYER_PIXELS.data(),
            PLAYER_W, PLAYER_H,
            32,
            PLAYER_W * sizeof(uint32_t),
            0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
        SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
        SDL_RenderCopy(renderer, tex, nullptr, &dst);
        SDL_DestroyTexture(tex);
    }
};

// Entry point
int main(int, char**) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }
    std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>
        window(SDL_CreateWindow("2D Platformer",
                                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                NATIVE_W * 2, NATIVE_H * 2, SDL_WINDOW_SHOWN),
               &SDL_DestroyWindow);
    std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)>
        renderer(SDL_CreateRenderer(window.get(), -1, SDL_RENDERER_ACCELERATED),
                 &SDL_DestroyRenderer);
    if (!window || !renderer) {
        SDL_Log("Failed to create window or renderer");
        return 1;
    }
    Player player;
    player.position = { 100.0f, 100.0f };
    Camera camera;
    bool running = true;
    float accumulator = 0.0f;
    Uint64 prevTicks = SDL_GetPerformanceCounter();
    while (running) {
        Uint64 currentTicks = SDL_GetPerformanceCounter();
        float frameTime = (currentTicks - prevTicks) / (float)SDL_GetPerformanceFrequency();
        prevTicks = currentTicks;
        accumulator += frameTime;
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) running = false;
        }
        while (accumulator >= FIXED_DT) {
            player.update(FIXED_DT);
            camera.update(player.position, FIXED_DT);
            accumulator -= FIXED_DT;
        }
        SDL_SetRenderDrawColor(renderer.get(), 92, 148, 252, 255);
        SDL_RenderClear(renderer.get());
        // Draw ground offset by camera
        SDL_SetRenderDrawColor(renderer.get(), 70, 70, 70, 255);
        for (int y = 0; y < LEVEL_HEIGHT; ++y) {
            for (int x = 0; x < LEVEL_WIDTH; ++x) {
                if (getTile(x, y) == 1) {
                    SDL_Rect r;
                    r.x = (int)((x * TILE_SIZE - camera.position.x) * 2);
                    r.y = (int)((y * TILE_SIZE - camera.position.y) * 2);
                    r.w = TILE_SIZE * 2;
                    r.h = TILE_SIZE * 2;
                    SDL_RenderFillRect(renderer.get(), &r);
                }
            }
        }
        player.draw(renderer.get(), 2.0f, camera.position);
        SDL_RenderPresent(renderer.get());
    }
    SDL_Quit();
    return 0;
}
