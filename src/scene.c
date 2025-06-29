#include "scene.h"

int g_enableAudio;
SDL_Cursor *g_handCursor;

Mix_Music *loadAndPlayMusic(const char *file, int loops) {
    Mix_Music *music = NULL;
    if (g_enableAudio) {
        music = Mix_LoadMUS(file);
        if (music) {
            Mix_PlayMusic(music, loops);
        }
    }
    return music;
}

void simpleFreeScene(Scene *scene) {
    freeAnimation(scene->animation);
    if (scene->music) {
        Mix_FreeMusic(scene->music);
    }
    SDL_SetCursor(SDL_GetDefaultCursor());
    SDL_free(scene);
}

void simpleDrawScene(SDL_Renderer *renderer, Scene *scene) {
    SDL_Texture *texture = scene->animation->textures[scene->animation->currentFrame];
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    if (scene->fadeOutStart) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255 - scene->alpha);
        SDL_RenderFillRect(renderer, NULL);
    }
}

void startFadeOut(Scene *scene, Uint64 time) {
    if (!scene->fadeOutStart) {
        if (scene->music) {
            Mix_FadeOutMusic(1000);
        }
        scene->alpha = 255;
        scene->fadeOutStart = time;
    }
}

int processFadeOut(Scene *scene, Uint64 time) {
    int alpha = 255 - ((time - scene->fadeOutStart) >> 2);
    if (alpha < 0) {
        return alpha <= -128;
    }
    else {
        scene->alpha = (alpha >> 5) << 5;
        return 0;
    }
}

void clickSkipSceneHandler(Scene *scene, int x, int y) {
    startFadeOut(scene, SDL_GetTicks64());
}
