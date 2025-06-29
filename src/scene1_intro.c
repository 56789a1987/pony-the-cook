#include "scene.h"

static Scene *(*updateGameIntroScene(Scene *scene, int delta, Uint64 time))(SDL_Renderer *) {
    AnimatedImage *animation = scene->animation;
    if (scene->fadeOutStart) {
        if (processFadeOut(scene, time)) {
            return createGameScene;
        }
    }

    if (isAnimationEnded(animation, delta)) {
        startFadeOut(scene, time);
    }
    else {
        updateAnimation(animation, delta);
    }

    return NULL;
}

Scene *createIntroScene(SDL_Renderer *renderer) {
    Scene *scene = SDL_malloc(sizeof(Scene));
    SDL_zerop(scene);

    AnimatedImage *animation = loadAnimationWebp(renderer, "images/intro.webp");
    if (!animation) {
        return NULL;
    }

    scene->animation = animation;
    scene->music = loadAndPlayMusic("sounds/intro.ogg", -1);
    scene->update = updateGameIntroScene;
    scene->draw = simpleDrawScene;
    scene->free = simpleFreeScene;
    scene->mouseDown = clickSkipSceneHandler;

    SDL_SetCursor(g_handCursor);
    return scene;
}
