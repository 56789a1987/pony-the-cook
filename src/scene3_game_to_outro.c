#include "scene.h"

static Scene *(*updateGameToOutroScene(Scene *scene, int delta, Uint64 time))(SDL_Renderer *) {
    AnimatedImage *animation = scene->animation;
    if (scene->fadeOutStart) {
        if (processFadeOut(scene, time)) {
            return createOutroScene;
        }
    }

    if (isAnimationEnded(animation, delta)) {
        setAnimationFrame(animation, animation->currentFrame - 9);

        if (!scene->music || !Mix_PlayingMusic()) {
            startFadeOut(scene, time);
        }
    }
    else {
        updateAnimation(animation, delta);
    }

    return NULL;
}

Scene *createGameToOutroScene(SDL_Renderer *renderer) {
    Scene *scene = SDL_malloc(sizeof(Scene));
    SDL_zerop(scene);

    AnimatedImage *animation = loadAnimationWebp(renderer, "images/cooking_end.webp");
    if (!animation) {
        return NULL;
    }

    scene->animation = animation;
    scene->music = loadAndPlayMusic("sounds/working_end.ogg", 0);
    scene->update = updateGameToOutroScene;
    scene->draw = simpleDrawScene;
    scene->free = simpleFreeScene;
    scene->mouseDown = clickSkipSceneHandler;

    SDL_SetCursor(g_handCursor);
    return scene;
}
