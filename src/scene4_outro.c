#include "scene.h"

static Scene *(*updateGameOutroScene(Scene *scene, int delta, Uint64 time))(SDL_Renderer *) {
    AnimatedImage *animation = scene->animation;
    if (scene->fadeOutStart) {
        if (processFadeOut(scene, time)) {
            return createIntroScene;
        }
    }
    else if (isAnimationEnded(animation, delta)) {
        startFadeOut(scene, time);
    }
    else {
        updateAnimation(animation, delta);
    }

    return NULL;
}

Scene *createOutroScene(SDL_Renderer *renderer) {
    Scene *scene = SDL_malloc(sizeof(Scene));
    SDL_zerop(scene);

    AnimatedImage *animation = loadAnimationWebp(renderer, "images/end_poisonous.webp");
    if (!animation) {
        return NULL;
    }

    scene->animation = animation;
    scene->music = loadAndPlayMusic("sounds/outro.ogg", 0);
    scene->update = updateGameOutroScene;
    scene->draw = simpleDrawScene;
    scene->free = simpleFreeScene;
    return scene;
}
