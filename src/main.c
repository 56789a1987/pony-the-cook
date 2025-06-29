#include "image.h"
#include "scene.h"

#define VIDEO_WIDTH  240
#define VIDEO_HEIGHT 180
#define MAX_FPS      60

int gameLoop(SDL_Window *window, SDL_Renderer *renderer, SDL_Texture *renderTarget, Scene *scene) {
    SDL_Event event;
    int windowWidth, windowHeight;
    Uint64 lastTicks = SDL_GetTicks64();
    SDL_GetWindowSize(window, &windowWidth, &windowHeight);
    SDL_GetWindowSizeInPixels;

    for (;;) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_MOUSEBUTTONDOWN:
                if (scene->mouseDown && event.button.button == SDL_BUTTON_LEFT) {
                    scene->mouseDown(scene, event.button.x * VIDEO_WIDTH / windowWidth, event.button.y * VIDEO_HEIGHT / windowHeight);
                }
                break;
            case SDL_MOUSEBUTTONUP:
                if (scene->mouseUp && event.button.button == SDL_BUTTON_LEFT) {
                    scene->mouseUp(scene, event.button.x * VIDEO_WIDTH / windowWidth, event.button.y * VIDEO_HEIGHT / windowHeight);
                }
                break;
            case SDL_MOUSEMOTION:
                if (scene->mouseMove) {
                    scene->mouseMove(scene, event.button.x * VIDEO_WIDTH / windowWidth, event.button.y * VIDEO_HEIGHT / windowHeight);
                }
                break;
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    windowWidth = event.window.data1;
                    windowHeight = event.window.data2;
                }
                break;
            case SDL_QUIT:
                scene->free(scene);
                return 0;
            }
        }

        Uint64 currentTicks = SDL_GetTicks64();
        int delta = currentTicks - lastTicks;
        if (delta < 1000 / MAX_FPS) {
            // Wait for next frame or event so we don't exhaust CPU
            SDL_WaitEventTimeout(NULL, 1000 / MAX_FPS - delta);
            continue;
        }
        lastTicks = currentTicks;

        Scene *(*createNextScene)(SDL_Renderer *) = scene->update(scene, delta, currentTicks);
        if (createNextScene) {
            // If the update function returns a function that creates a new scene, switch scene
            scene->free(scene);
            scene = createNextScene(renderer);
            if (!scene) {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize next scene");
                return -1;
            }
            scene->startTime = currentTicks;
        }
        else {
            // Draw the scene on the target texture
            SDL_SetRenderTarget(renderer, renderTarget);
            scene->draw(renderer, scene);
            // Draw the target texture on the window
            SDL_SetRenderTarget(renderer, NULL);
            SDL_RenderCopy(renderer, renderTarget, NULL, NULL);
            SDL_RenderPresent(renderer);
        }
    }
}

int main(int argc, char *argv[]) {
    g_enableAudio = 1;

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "Couldn't initialize SDL: %s", SDL_GetError());
        return -1;
    }

    // Initialize SDL image
    if (IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG) {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Couldn't initialize SDL image: %s", IMG_GetError());
        return -1;
    }

    // Initialize SDL mixer. If failed, disable audio and continue
    if (Mix_Init(MIX_INIT_OGG) != MIX_INIT_OGG) {
        g_enableAudio = 0;
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Couldn't initialize SDL mixer: %s", Mix_GetError());
    }

    // Create the window and renderer
    // SDL_WINDOW_RESIZABLE - the window is resizable
    // SDL_WINDOW_ALLOW_HIGHDPI - avoid blurry pixels on high DPI screen
    SDL_Window *window = SDL_CreateWindow("Cook", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, VIDEO_WIDTH * 2, VIDEO_HEIGHT * 2, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window) {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Couldn't create window: %s", SDL_GetError());
        return -1;
    }

    // SDL_RENDERER_TARGETTEXTURE - allow rendering to a texture
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
    if (!renderer) {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Couldn't create accelerated renderer: %s", SDL_GetError());

        // If we couldn't create an accelerated renderer, try falling-back to the software renderer
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE | SDL_RENDERER_TARGETTEXTURE);
        if (!renderer) {
            SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Couldn't create renderer: %s", SDL_GetError());
            return -1;
        }
    }

    // Create a fixed size texture as the render target to draw everything, then draw it on the window
    // When the window is resized, we keep the viewport fill the window and don't have to reposition everything
    SDL_Texture *renderTarget = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, VIDEO_WIDTH, VIDEO_HEIGHT);
    if (!renderTarget) {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Couldn't create render target texture: %s", SDL_GetError());
        return -1;
    }

    if (g_enableAudio && Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 4096) < 0) {
        g_enableAudio = 0;
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Couldn't open audio: %s", Mix_GetError());
    }

    // Store the cursor as global variable
    g_handCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);

    Scene *scene = createIntroScene(renderer);
    if (!scene) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize the first scene");
        return -1;
    }

    int status = gameLoop(window, renderer, renderTarget, scene);

    // Release everything
    IMG_Quit();
    if (g_enableAudio) {
        Mix_CloseAudio();
    }
    SDL_FreeCursor(g_handCursor);
    SDL_DestroyTexture(renderTarget);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return status;
}
