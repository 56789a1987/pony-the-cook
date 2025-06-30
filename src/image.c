#include "image.h"
#include <webp/demux.h>

static void *readFile(const char *file, size_t *sizeOut) {
    SDL_RWops *fileRW = SDL_RWFromFile(file, "rb");
    if (!fileRW) {
        return NULL;
    }

    Sint64 size = SDL_RWsize(fileRW);
    if (size < 0) {
        return NULL;
    }

    void *buffer = SDL_malloc(size);
    if (SDL_RWread(fileRW, buffer, 1, size) == 0) {
        return NULL;
    }
    SDL_RWclose(fileRW);

    *sizeOut = size;
    return buffer;
}

StaticImage *loadImageWebp(SDL_Renderer *renderer, const char *file) {
    size_t fileSize;
    void *buffer = readFile(file, &fileSize);
    if (!buffer) {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Couldn't open image %s: %s", file, SDL_GetError());
        return NULL;
    }

    int width, height;
    uint8_t *rgba = WebPDecodeRGBA(buffer, fileSize, &width, &height);
    if (!rgba) {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Couldn't decode image: %s", file);
        return NULL;
    }

    StaticImage *image = SDL_malloc(sizeof(StaticImage));
    SDL_zerop(image);
    image->width = width;
    image->height = height;

    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, width, height);
    if (!texture || SDL_UpdateTexture(texture, NULL, rgba, width * 4) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Couldn't create texture for image %s: %s", file, SDL_GetError());
        return NULL;
    }
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    image->texture = texture;

    WebPFree(rgba);
    SDL_free(buffer);

    return image;
}

void freeImage(StaticImage *image) {
    SDL_DestroyTexture(image->texture);
    SDL_free(image);
}

AnimatedImage *loadAnimationWebp(SDL_Renderer *renderer, const char *file) {
    size_t fileSize;
    void *buffer = readFile(file, &fileSize);
    if (!buffer) {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Couldn't open image %s: %s", file, SDL_GetError());
        return NULL;
    }

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

    int width = webpInfo.canvas_width;
    int height = webpInfo.canvas_height;
    int frames = webpInfo.frame_count;
    image->width = width;
    image->height = height;
    image->frameCount = frames;
    image->delays = SDL_malloc(sizeof(int) * frames);
    image->textures = SDL_malloc(sizeof(SDL_Surface *) * frames);

    int frame = 0;
    int lastTimestamp = 0;
    while (WebPAnimDecoderHasMoreFrames(webpDecoder)) {
        int timestamp;
        uint8_t *rgba;
        if (!WebPAnimDecoderGetNext(webpDecoder, &rgba, &timestamp)) {
            SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Couldn't decode image %s frame %d", file, frame);
            return NULL;
        }

        SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, width, height);
        if (!texture || SDL_UpdateTexture(texture, NULL, rgba, width * 4) < 0) {
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
