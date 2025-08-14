#include <SDL2/SDL.h>
#include <cstdint>
#include <array>
#include <vector>
#include <cmath>
#include <memory>

//----------------------------------------------------------------------------
// 2D Platformer Implementation Skeleton
//----------------------------------------------------------------------------
// This file is a C++20 skeleton for a complete 2D side‑scrolling platformer.
// It follows the design described in the specification.  The comments in this
// file reference the numbered sections of the spec (e.g., Section 0, 1, 2, …).
// The goal is to provide a deterministic, fixed time step engine with all
// logic, physics and rendering authored in C++ without external assets.  The
// code below sets up an SDL2 window, implements a fixed timestep loop (Section 0)
// and defines basic data structures for the player, physics and rendering.
// Many details are omitted for brevity; however, the scaffolding is ready for
// incremental development of the full game.
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
// In a full implementation, segments would be stored separately and animated.
// For this skeleton we embed a tiny placeholder sprite (white box).
static constexpr int PLAYER_W = 22;
static constexpr int PLAYER_H = 32;
static const std::array<uint32_t, PLAYER_W * PLAYER_H> PLAYER_PIXELS = []{
    std::array<uint32_t, PLAYER_W * PLAYER_H> data{};
    for (int i = 0; i < PLAYER_W * PLAYER_H; ++i) {
        // Fill with opaque white (RGBA).  Real art would be defined here.
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
    // Additional states (e.g., attacks, wall slide) can be added here
};

// Section 15 – Level Definition (tile map)
// Here we embed a tiny level as a 2‑D array.  Each integer corresponds to a tile enum.
static constexpr int LEVEL_WIDTH  = 32;
static constexpr int LEVEL_HEIGHT = 16;
static const std::array<uint8_t, LEVEL_WIDTH * LEVEL_HEIGHT> LEVEL_DATA = []{
    // Construct a simple flat terrain: bottom row solid, others air.
    std::array<uint8_t, LEVEL_WIDTH * LEVEL_HEIGHT> data{};
    for (int y = 0; y < LEVEL_HEIGHT; ++y) {
        for (int x = 0; x < LEVEL_WIDTH; ++x) {
            data[y * LEVEL_WIDTH + x] = (y == LEVEL_HEIGHT - 1) ? 1 /*SOLID*/ : 0 /*AIR*/;
        }
    }
    return data;
}();

// Helper to sample a tile at (x,y)
uint8_t getTile(int x, int y) {
    if (x < 0 || y < 0 || x >= LEVEL_WIDTH || y >= LEVEL_HEIGHT) return 1; // Treat out of bounds as solid
    return LEVEL_DATA[y * LEVEL_WIDTH + x];
}

// Section 7/8 – Player Movement & Jumping (basic implementation)
struct Player {
    Vec2 position{};
    Vec2 velocity{};
    PlayerState state{ PlayerState::Idle };
    bool onGround{ false };
    float jumpBufferTimer{ 0.0f };
    float coyoteTimer{ 0.0f };

    void update(float dt) {
        // Advance timers (Section 4)
        if (jumpBufferTimer > 0.0f) jumpBufferTimer -= dt;
        if (coyoteTimer     > 0.0f) coyoteTimer     -= dt;

        // Input sampling (placeholder – replace with actual keyboard handling)
        const Uint8* keys = SDL_GetKeyboardState(nullptr);
        bool left  = keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A];
        bool right = keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D];
        bool jump  = keys[SDL_SCANCODE_SPACE];

        // Horizontal movement
        float desiredAccel = 0.0f;
        if (left ^ right) {
            desiredAccel = (left ? -1.0f : 1.0f) * (onGround ? GROUND_ACCEL : AIR_ACCEL);
        } else {
            // Decelerate toward zero
            if (onGround) {
                if (velocity.x > 0.0f) {
                    desiredAccel = -GROUND_DECEL;
                } else if (velocity.x < 0.0f) {
                    desiredAccel = GROUND_DECEL;
                }
            } else {
                if (velocity.x > 0.0f) {
                    desiredAccel = -AIR_DECEL;
                } else if (velocity.x < 0.0f) {
                    desiredAccel = AIR_DECEL;
                }
            }
        }
        velocity.x += desiredAccel * dt;
        // Clamp speed
        if (velocity.x >  MAX_RUN_SPEED) velocity.x =  MAX_RUN_SPEED;
        if (velocity.x < -MAX_RUN_SPEED) velocity.x = -MAX_RUN_SPEED;

        // Jump buffering & coyote (Section 8)
        if (jump) {
            jumpBufferTimer = 0.09f; // 90 ms buffer
        }
        if (jumpBufferTimer > 0.0f && (onGround || coyoteTimer > 0.0f)) {
            // Launch jump
            velocity.y = JUMP_V0;
            onGround = false;
            jumpBufferTimer = 0.0f;
            coyoteTimer = 0.0f;
        }

        // Apply gravity
        velocity.y += GRAVITY * dt;
        // Integrate position
        Vec2 newPos = position;
        newPos.x += velocity.x * dt;
        newPos.y += velocity.y * dt;

        // Collision detection (very simple AABB vs tile; Section 3/14)
        // Sweep Y
        if (velocity.y > 0.0f) {
            // Falling – check tiles below
            int bottom = (int)((newPos.y + PLAYER_H) / TILE_SIZE);
            int leftTile  = (int)((newPos.x)            / TILE_SIZE);
            int rightTile = (int)((newPos.x + PLAYER_W - 1) / TILE_SIZE);
            bool collides = false;
            for (int tx = leftTile; tx <= rightTile; ++tx) {
                if (getTile(tx, bottom) == 1) {
                    collides = true;
                    break;
                }
            }
            if (collides) {
                // Snap to tile boundary
                newPos.y = bottom * TILE_SIZE - PLAYER_H;
                velocity.y = 0.0f;
                onGround = true;
                coyoteTimer = 0.1f; // allow jump shortly after leaving
            }
        } else if (velocity.y < 0.0f) {
            // Rising – check head collisions (omitted for brevity)
        }

        // Sweep X (very naive; Section 14)
        int top = (int)((newPos.y) / TILE_SIZE);
        int bottomY = (int)((newPos.y + PLAYER_H - 1) / TILE_SIZE);
        if (velocity.x > 0.0f) {
            int rightTile = (int)((newPos.x + PLAYER_W) / TILE_SIZE);
            bool collides = false;
            for (int ty = top; ty <= bottomY; ++ty) {
                if (getTile(rightTile, ty) == 1) {
                    collides = true;
                    break;
                }
            }
            if (collides) {
                newPos.x = rightTile * TILE_SIZE - PLAYER_W;
                velocity.x = 0.0f;
            }
        } else if (velocity.x < 0.0f) {
            int leftTile = (int)(newPos.x / TILE_SIZE);
            bool collides = false;
            for (int ty = top; ty <= bottomY; ++ty) {
                if (getTile(leftTile, ty) == 1) {
                    collides = true;
                    break;
                }
            }
            if (collides) {
                newPos.x = (leftTile + 1) * TILE_SIZE;
                velocity.x = 0.0f;
            }
        }

        position = newPos;

        // Update state machine (simplified)
        if (!onGround) {
            state = (velocity.y < 0.0f) ? PlayerState::JumpRise : PlayerState::Fall;
        } else {
            if (std::abs(velocity.x) > 1.0f) {
                state = PlayerState::Run;
            } else {
                state = PlayerState::Idle;
            }
        }
    }

    void draw(SDL_Renderer* renderer, float scale) const {
        // Render the embedded pixel data (Section 1)
        SDL_Rect dst;
        dst.x = static_cast<int>(position.x * scale);
        dst.y = static_cast<int>(position.y * scale);
        dst.w = static_cast<int>(PLAYER_W * scale);
        dst.h = static_cast<int>(PLAYER_H * scale);
        // Create an SDL surface from the pixel array
        SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(
            (void*)PLAYER_PIXELS.data(),
            PLAYER_W, PLAYER_H,
            32,
            PLAYER_W * sizeof(uint32_t),
            0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000
        );
        SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
        SDL_RenderCopy(renderer, tex, nullptr, &dst);
        SDL_DestroyTexture(tex);
    }
};

// Entry point (Section 0 – Global Targets & Constraints)
int main(int, char**) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }
    // RAII wrappers for window and renderer (Section 0 – Style: RAII)
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

    bool running = true;
    float accumulator = 0.0f;
    Uint64 prevTicks = SDL_GetPerformanceCounter();
    while (running) {
        Uint64 currentTicks = SDL_GetPerformanceCounter();
        float frameTime = (currentTicks - prevTicks) / (float)SDL_GetPerformanceFrequency();
        prevTicks = currentTicks;
        accumulator += frameTime;
        // Handle events
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) {
                running = false;
            }
        }
        // Fixed update loop (never drop logic frames; Section 0)
        while (accumulator >= FIXED_DT) {
            player.update(FIXED_DT);
            accumulator -= FIXED_DT;
        }
        // Render
        SDL_SetRenderDrawColor(renderer.get(), 92, 148, 252, 255); // background sky color
        SDL_RenderClear(renderer.get());
        // Draw simple ground (Section 16 – Parallax & Atmosphere omitted)
        SDL_SetRenderDrawColor(renderer.get(), 70, 70, 70, 255);
        for (int y = 0; y < LEVEL_HEIGHT; ++y) {
            for (int x = 0; x < LEVEL_WIDTH; ++x) {
                if (getTile(x,y) == 1) {
                    SDL_Rect r;
                    r.x = x * TILE_SIZE * 2;
                    r.y = y * TILE_SIZE * 2;
                    r.w = TILE_SIZE * 2;
                    r.h = TILE_SIZE * 2;
                    SDL_RenderFillRect(renderer.get(), &r);
                }
            }
        }
        player.draw(renderer.get(), 2.0f); // Upscale by 2x for clarity
        SDL_RenderPresent(renderer.get());
    }
    SDL_Quit();
    return 0;
}
