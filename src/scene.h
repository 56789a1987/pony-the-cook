#ifndef APP_SCENE_h
#define APP_SCENE_h

#include <SDL2/SDL_mixer.h>
#include "image.h"

extern int g_enableAudio;
extern SDL_Cursor *g_handCursor;

typedef struct Scene Scene;
struct Scene {
    AnimatedImage *animation;
    Mix_Music *music;
    Uint64 startTime;
    Uint64 fadeOutStart;
    int alpha;

    Scene *(*(*update)(Scene *, int, Uint64))(SDL_Renderer *);
    void (*draw)(SDL_Renderer *, Scene *);
    void (*free)(Scene *);

    void (*mouseDown)(Scene *, int, int);
    void (*mouseMove)(Scene *, int, int);
    void (*mouseUp)(Scene *, int, int);

    void *params;
};

Mix_Music *loadAndPlayMusic(const char *file, int loops);
void simpleFreeScene(Scene *scene);
void simpleDrawScene(SDL_Renderer *renderer, Scene *scene);
void startFadeOut(Scene *scene, Uint64 time);
int processFadeOut(Scene *scene, Uint64 time);
void clickSkipSceneHandler(Scene *scene, int x, int y);

Scene *createIntroScene(SDL_Renderer *renderer);
Scene *createOutroScene(SDL_Renderer *renderer);
Scene *createGameScene(SDL_Renderer *renderer);
Scene *createGameToOutroScene(SDL_Renderer *renderer);

#endif
