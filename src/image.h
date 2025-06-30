#ifndef APP_IMAGE_h
#define APP_IMAGE_h

#include <SDL2/SDL.h>

typedef struct {
    int width;
    int height;
    SDL_Texture *texture;
} StaticImage;

StaticImage *loadImageWebp(SDL_Renderer *renderer, const char *file);
void freeImage(StaticImage *image);

typedef struct {
    int width;
    int height;
    int frameCount;
    int *delays;
    SDL_Texture **textures;
    int currentFrame;
    int currentDelayLeft;
} AnimatedImage;

AnimatedImage *loadAnimationWebp(SDL_Renderer *renderer, const char *file);
void setAnimationFrame(AnimatedImage *animation, int frame);
void resetAnimation(AnimatedImage *animation);
void freeAnimation(AnimatedImage *animation);
int updateAnimation(AnimatedImage *animation, int delta);
int isAnimationEnded(AnimatedImage *animation, int delta);

#endif
