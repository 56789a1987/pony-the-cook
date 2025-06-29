#include "image.h"
#include <webp/demux.h>

StaticImage *loadImagePng(SDL_Renderer *renderer, const char *file) {
    SDL_RWops *fileRW = SDL_RWFromFile(file, "rb");
    if (!fileRW) {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Couldn't open image %s: %s", file, SDL_GetError());
        return NULL;
    }

    SDL_Surface *surface = IMG_LoadPNG_RW(fileRW);
    if (!surface) {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Couldn't load image %s: %s", file, IMG_GetError());
        SDL_RWclose(fileRW);
        return NULL;
    }
    SDL_RWclose(fileRW);

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Couldn't create texture for image %s: %s", file, SDL_GetError());
        return NULL;
    }
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

    StaticImage *image = SDL_malloc(sizeof(StaticImage));
    image->surface = surface;
    image->texture = texture;
    return image;
}

void freeImage(StaticImage *image) {
    SDL_DestroyTexture(image->texture);
    SDL_FreeSurface(image->surface);
    SDL_free(image);
}

AnimatedImage *loadAnimationWebp(SDL_Renderer *renderer, const char *file) {
    SDL_RWops *fileRW = SDL_RWFromFile(file, "rb");
    if (!fileRW) {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Couldn't open image file %s: %s", file, SDL_GetError());
        return NULL;
    }

    Sint64 fileSize = SDL_RWsize(fileRW);
    void *buffer = SDL_malloc(fileSize);

    if (SDL_RWread(fileRW, buffer, 1, fileSize) == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Couldn't read image file %s: %s", file, SDL_GetError());
        return NULL;
    }
    SDL_RWclose(fileRW);

    WebPData webpData;
    WebPDataInit(&webpData);
    webpData.bytes = buffer;
    webpData.size = fileSize;
    WebPAnimDecoder *webpDecoder = WebPAnimDecoderNew(&webpData, NULL);
    if (!webpDecoder) {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Couldn't decode image %s", file);
        return NULL;
    }

    WebPAnimInfo webpInfo;
    if (!WebPAnimDecoderGetInfo(webpDecoder, &webpInfo)) {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Couldn't get image info for %s", file);
        return NULL;
    }

    AnimatedImage *image = SDL_malloc(sizeof(AnimatedImage));
    SDL_zerop(image);

    image->width = webpInfo.canvas_width;
    image->height = webpInfo.canvas_height;
    image->frameCount = webpInfo.frame_count;
    image->delays = SDL_malloc(sizeof(int) * webpInfo.frame_count);
    image->textures = SDL_malloc(sizeof(SDL_Surface *) * webpInfo.frame_count);

    int frame = 0;
    int lastTimestamp = 0;
    while (WebPAnimDecoderHasMoreFrames(webpDecoder)) {
        int timestamp;
        uint8_t *frameBuffer;
        if (!WebPAnimDecoderGetNext(webpDecoder, &frameBuffer, &timestamp)) {
            SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Couldn't decode image %s frame %d", file, frame);
            return NULL;
        }

        SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, webpInfo.canvas_width, webpInfo.canvas_height);
        if (SDL_UpdateTexture(texture, NULL, frameBuffer, webpInfo.canvas_width * 4) < 0) {
            SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Couldn't create texture for image %s frame %d: %s", file, frame, SDL_GetError());
            return NULL;
        }
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

        image->textures[frame] = texture;
        image->delays[frame] = timestamp - lastTimestamp;

        frame++;
        lastTimestamp = timestamp;
    }

    WebPAnimDecoderDelete(webpDecoder);
    SDL_free(buffer);

    resetAnimation(image);
    return image;
}

void setAnimationFrame(AnimatedImage *animation, int frame) {
    animation->currentFrame = frame;
    animation->currentDelayLeft = animation->delays[frame];
}

void resetAnimation(AnimatedImage *animation) {
    animation->currentFrame = 0;
    animation->currentDelayLeft = animation->delays[0];
}

void freeAnimation(AnimatedImage *animation) {
    for (int i = 0; i < animation->frameCount; i++) {
        SDL_DestroyTexture(animation->textures[i]);
    }
    SDL_free(animation->delays);
    SDL_free(animation->textures);
    SDL_free(animation);
}

int updateAnimation(AnimatedImage *animation, int delta) {
    int lastFrame = animation->currentFrame;
    animation->currentDelayLeft -= delta;
    if (animation->currentDelayLeft <= 0) {
        animation->currentFrame = (animation->currentFrame + 1) % animation->frameCount;
        animation->currentDelayLeft += animation->delays[animation->currentFrame];
    }
    return animation->currentFrame - lastFrame;
}

int isAnimationEnded(AnimatedImage *animation, int delta) {
    return animation->currentFrame == animation->frameCount - 1 && animation->currentDelayLeft <= delta;
}
