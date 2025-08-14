#include <SDL2/SDL.h>
#include <cmath>
#include <array>

// Parallax Demo implementing Section 1 (Rendering & Assets) and Section 2 (Camera)
// of the design specification. This program demonstrates a simple parallax
// background consisting of four layers, each moving at a different relative
// speed as the camera follows the player. The player can move left/right,
// and the camera smoothly follows using a basic smoothing factor. All logic
// is handled in C++ with no external assets, meeting the deterministic
// requirements described in Section 0.

constexpr int NATIVE_W = 480;   // Native render target width (Section 0)
constexpr int NATIVE_H = 270;   // Native render target height (Section 0)
constexpr float DT      = 1.0f / 60.0f; // Fixed timestep (60 Hz)

// Structure representing a parallax layer (Section 1)
struct ParallaxLayer {
    float speed;      // Relative speed factor (0..1)
    SDL_Color color;  // Solid fill color for this layer
};

// Define four parallax layers with increasing speeds (Section 1)
static const std::array<ParallaxLayer, 4> PARALLAX = {{
    {0.05f, {135, 206, 235, 255}}, // Sky layer (back)
    {0.20f, {100, 149, 237, 255}}, // Far clouds
    {0.45f, {70, 130, 180, 255}},  // Mid trees
    {0.75f, {65, 105, 225, 255}}   // Near foliage
}};

int main(int argc, char* argv[]) {
    // Initialize SDL (Section 1)
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("SDL_Init Error: %s", SDL_GetError());
        return 1;
    }
    // Create window scaled by 2x for clarity
    SDL_Window* window = SDL_CreateWindow(
        "Parallax Demo",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        NATIVE_W * 2,
        NATIVE_H * 2,
        SDL_WINDOW_SHOWN);
    if (!window) {
        SDL_Log("SDL_CreateWindow Error: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        SDL_Log("SDL_CreateRenderer Error: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    // Set logical size to native resolution so scaling is automatic (Section 0)
    SDL_RenderSetLogicalSize(renderer, NATIVE_W, NATIVE_H);

    float playerX  = NATIVE_W / 2.0f;
    const float playerY  = NATIVE_H - 40.0f;  // Stand on ground near bottom
    float cameraX = 0.0f;
    bool quit = false;
    Uint32 lastTick = SDL_GetTicks();
    float accumulator = 0.0f;

    while (!quit) {
        Uint32 now = SDL_GetTicks();
        float frameTime = (now - lastTick) / 1000.0f;
        if (frameTime > 0.25f) frameTime = 0.25f; // Clamp to avoid spiral
        lastTick = now;
        accumulator += frameTime;

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
        }
        // Input state (Section 4)
        const Uint8* keystate = SDL_GetKeyboardState(nullptr);

        // Fixed timestep update loop (Section 0)
        while (accumulator >= DT) {
            // Horizontal movement
            const float moveSpeed = 120.0f; // pixels per second
            if (keystate[SDL_SCANCODE_A] || keystate[SDL_SCANCODE_LEFT]) {
                playerX -= moveSpeed * DT;
            }
            if (keystate[SDL_SCANCODE_D] || keystate[SDL_SCANCODE_RIGHT]) {
                playerX += moveSpeed * DT;
            }
            // Clamp player within a simple world extents
            if (playerX < 0.0f) playerX = 0.0f;
            if (playerX > 1024.0f) playerX = 1024.0f;

            // Smooth camera follow (Section 2)
            float targetCamX = playerX - NATIVE_W * 0.5f;
            // Simple smoothing factor for critical damping approximation
            const float smoothFactor = 0.10f;
            cameraX += (targetCamX - cameraX) * smoothFactor;

            accumulator -= DT;
        }

        // Render (Section 1)
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Draw parallax layers (Section 1)
        for (const auto& layer : PARALLAX) {
            // Compute horizontal offset based on camera position and layer speed
            float offset = fmodf(cameraX * layer.speed, static_cast<float>(NATIVE_W));
            if (offset < 0) offset += NATIVE_W;
            // Set layer color
            SDL_SetRenderDrawColor(renderer, layer.color.r, layer.color.g, layer.color.b, layer.color.a);
            // Draw two rectangles to tile the layer across the screen
            SDL_Rect rect1{ static_cast<int>(-offset), 0, NATIVE_W, NATIVE_H };
            SDL_Rect rect2{ rect1.x + NATIVE_W, 0, NATIVE_W, NATIVE_H };
            SDL_RenderFillRect(renderer, &rect1);
            SDL_RenderFillRect(renderer, &rect2);
        }

        // Draw simple ground
        SDL_SetRenderDrawColor(renderer, 34, 139, 34, 255); // green
        SDL_Rect groundRect{ 0, static_cast<int>(playerY + 20), NATIVE_W, NATIVE_H - static_cast<int>(playerY + 20) };
        SDL_RenderFillRect(renderer, &groundRect);

        // Draw player as a red rectangle
        SDL_SetRenderDrawColor(renderer, 220, 20, 60, 255);
        SDL_Rect playerRect{
            static_cast<int>(playerX - cameraX) - 10,
            static_cast<int>(playerY) - 20,
            20,
            40
        };
        SDL_RenderFillRect(renderer, &playerRect);

        SDL_RenderPresent(renderer);
    }

    // Cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
