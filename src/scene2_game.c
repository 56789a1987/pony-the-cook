#include "scene.h"
#include <time.h>

#define INGREDIENT_MAX_X 240
#define INGREDIENT_WIDTH 50
#define INGREDIENT_PLAIN 10 // How many ingredients are not rare
#define INGREDIENT_QUEUE 4
#define INGREDIENT_DELAY 1500
static const SDL_Rect dragTargetRect = { 50, 120, 160, 60 };

typedef struct {
    int hidden;
    int pressed;
    SDL_Rect rect;
    StaticImage *image;
} UIButton;

typedef struct {
    int type;
    int accurateX;
    int waveType;
    SDL_Rect rect;
} GameSceneIngredient;

typedef struct {
    int finished;
    int ingredientsCount;
    int ingredientId;
    int typesQueueIndex;
    int *counts;
    int *typesQueue; // Use a pre-generated queue to avoid repeated items in a round
    Uint64 lastIngredientGenerateTime;
    UIButton cookButton;
    StaticImage *eyesSheet;
    StaticImage *ingredientsSheet;
    AnimatedImage *idleAnimation;
    AnimatedImage *actionAnimation;
    GameSceneIngredient *ingredients[INGREDIENT_QUEUE];
    GameSceneIngredient *draggingIngredient;

    int isHandCursor;
    int isAltIdleImage;
    SDL_Point cursor;
    SDL_Point dragOffset;
} GameSceneParams;

static void createUIButton(UIButton *button, StaticImage *image, int x, int y) {
    button->image = image;
    button->rect.x = x;
    button->rect.y = y;
    button->rect.w = image->surface->w >> 1;
    button->rect.h = image->surface->h;
}

static void drawUIButton(SDL_Renderer *renderer, UIButton *button) {
    SDL_Rect srcRect = { button->pressed ? button->rect.w : 0, 0, button->rect.w, button->rect.h };
    SDL_RenderCopy(renderer, button->image->texture, &srcRect, &button->rect);
}

static void generateTypesQueue(int *queue, int count) {
    // initialize the array
    for (int i = 0; i < count; i++) {
        queue[i] = i;
    }
    // shuffle the array
    for (int i = 0; i < count - 1; i++) {
        int j = i + rand() / (RAND_MAX / (count - i) + 1);
        int temp = queue[j];
        queue[j] = queue[i];
        queue[i] = temp;
    }
}

static GameSceneIngredient *generateGameSceneIngredient(GameSceneParams *params) {
    GameSceneIngredient *item = SDL_malloc(sizeof(GameSceneIngredient));
    SDL_zerop(item);
    item->waveType = params->ingredientId % 2 == 0 ? 1 : -1;
    item->accurateX = INGREDIENT_MAX_X * 16;
    item->rect.x = INGREDIENT_MAX_X;
    item->rect.w = INGREDIENT_WIDTH;
    item->rect.h = params->ingredientsSheet->surface->h;

    int rareTypesCount = params->ingredientsCount - INGREDIENT_PLAIN;
    // Possible to generate a rare ingredient every 3 items
    if (params->ingredientId % 3 == 0) {
        int randValue = rand() / (RAND_MAX / 100 + 1);
        if (randValue < rareTypesCount) {
            item->type = INGREDIENT_PLAIN + randValue;
        }
    }

    if (item->type == 0) {
        // The current queue is used up, generate next queue
        if (params->typesQueueIndex >= INGREDIENT_PLAIN) {
            params->typesQueueIndex = 0;
            generateTypesQueue(params->typesQueue, INGREDIENT_PLAIN);
        }
        item->type = params->typesQueue[params->typesQueueIndex];
        params->typesQueueIndex++;
    }
    params->ingredientId++;

    return item;
}

static void unsetGameSceneIngredient(GameSceneParams *params, int index) {
    if (params->ingredients[index]) {
        SDL_free(params->ingredients[index]);
        params->ingredients[index] = NULL;
    }
}

static void freeGameScene(Scene *scene) {
    GameSceneParams *params = scene->params;
    freeImage(params->cookButton.image);
    freeImage(params->eyesSheet);
    freeImage(params->ingredientsSheet);
    // Restore frame count to correctly free all textures
    params->idleAnimation->frameCount *= 2;
    freeAnimation(params->idleAnimation);
    freeAnimation(params->actionAnimation);
    for (int i = 0; i < INGREDIENT_QUEUE; i++) {
        unsetGameSceneIngredient(params, i);
    }
    SDL_free(params->counts);
    SDL_free(params->typesQueue);
    SDL_free(params);

    if (scene->music) {
        Mix_FreeMusic(scene->music);
    }

    SDL_SetCursor(SDL_GetDefaultCursor());
    SDL_free(scene);
}

static void drawGameScene(SDL_Renderer *renderer, Scene *scene) {
    GameSceneParams *params = scene->params;
    // White background
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, NULL);

    // Look at the item being dragged
    GameSceneIngredient *lookAtItem = params->draggingIngredient;
    if (!lookAtItem) {
        for (int i = 0; i < INGREDIENT_QUEUE; i++) {
            GameSceneIngredient *item = params->ingredients[i];
            if (item) {
                if (item->type >= INGREDIENT_PLAIN) {
                    // Or look at the rare item
                    lookAtItem = item;
                    break;
                }
                else if (!lookAtItem || item->accurateX > lookAtItem->accurateX) {
                    // Or look at the right-most item
                    lookAtItem = item;
                }
            }
        }
    }

    if (lookAtItem) {
        SDL_Rect *itemRect = &lookAtItem->rect;
        int eyeWidth = params->eyesSheet->surface->w;
        int offsetY = params->isAltIdleImage ? 3 : 0;

        // Left eye
        double dx = (double)(itemRect->x + itemRect->w / 2 - 130 - offsetY);
        double dy = (double)(itemRect->y + itemRect->h / 2 - 100);
        double angle = SDL_atan2(dy, dx);
        double distance = SDL_sqrt(dx * dx + dy * dy);
        double clampedX = SDL_cos(angle) * SDL_min(distance, 8);
        double clampedY = SDL_sin(angle) * SDL_min(distance, 8);

        SDL_Rect eyeSrc = { 0, 0, eyeWidth, 32 };
        SDL_Rect eyeDst = { 0, 0, eyeWidth, 32 };
        SDL_Texture *texture = params->eyesSheet->texture;
        eyeDst.x = 119 + (int)clampedX;
        eyeDst.y = 82 + offsetY + (int)clampedY;
        SDL_RenderCopy(renderer, texture, &eyeSrc, &eyeDst);

        eyeSrc.y = 32;
        eyeDst.x = 119 + (int)(clampedX * 1.25);
        eyeDst.y = 82 + offsetY + (int)(clampedY * 1.5);
        SDL_RenderCopy(renderer, texture, &eyeSrc, &eyeDst);

        //  Left eye highlights
        eyeSrc.y = 64;
        eyeDst.x = 119 + (int)clampedX;
        eyeDst.y = 82 + offsetY + (int)clampedY;
        SDL_RenderCopy(renderer, texture, &eyeSrc, &eyeDst);
        eyeDst.x = 132 + 8 + (int)(clampedX * 2);
        SDL_RenderCopy(renderer, texture, &eyeSrc, &eyeDst);

        // Right eye
        dx = (double)(itemRect->x + itemRect->w / 2 - 160 - offsetY);
        dy = (double)(itemRect->y + itemRect->h / 2 - 100);
        angle = SDL_atan2(dy, dx);
        distance = SDL_sqrt(dx * dx + dy * dy);
        clampedX = SDL_cos(angle) * SDL_min(distance, 8);
        clampedY = SDL_sin(angle) * SDL_min(distance, 8);

        eyeSrc.y = 0;
        eyeDst.x = 150 + (int)(clampedX);
        eyeDst.y = 82 + offsetY + (int)(clampedY);
        SDL_RenderCopy(renderer, texture, &eyeSrc, &eyeDst);

        eyeSrc.y = 32;
        eyeDst.x = 150 + (int)(clampedX * 1.25);
        eyeDst.y = 82 + offsetY + (int)(clampedY * 1.5);
        SDL_RenderCopy(renderer, texture, &eyeSrc, &eyeDst);

        // Right eye highlights
        eyeSrc.y = 64;
        eyeDst.x = 150 + (int)clampedX;
        eyeDst.y = 82 + offsetY + (int)clampedY;
        SDL_RenderCopy(renderer, texture, &eyeSrc, &eyeDst);
        eyeDst.x = 163 + 8 + (int)(clampedX * 2);
        SDL_RenderCopy(renderer, texture, &eyeSrc, &eyeDst);
    }

    // Scene
    int frame = scene->animation->currentFrame;
    if (scene->animation == params->idleAnimation && params->isAltIdleImage) {
        // Apply the beat animation for the idle animation
        frame += params->idleAnimation->frameCount;
    }
    SDL_RenderCopy(renderer, scene->animation->textures[frame], NULL, NULL);

    // Ingredients
    for (int i = 0; i < INGREDIENT_QUEUE; i++) {
        GameSceneIngredient *item = params->ingredients[i];
        if (item) {
            SDL_Rect srcRect = { item->type * INGREDIENT_WIDTH, 0, item->rect.w, item->rect.h };
            SDL_RenderCopy(renderer, params->ingredientsSheet->texture, &srcRect, &item->rect);
        }
    }

    // Continue button
    if (!params->cookButton.hidden) {
        drawUIButton(renderer, &params->cookButton);
    }

    // Fade out mask
    if (scene->fadeOutStart) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255 - scene->alpha);
        SDL_RenderFillRect(renderer, NULL);
    }
}

static void gameSceneMouseDown(Scene *scene, int x, int y) {
    GameSceneParams *params = scene->params;
    params->cursor.x = x;
    params->cursor.y = y;
    if (!params->cookButton.hidden && SDL_PointInRect(&params->cursor, &params->cookButton.rect)) {
        // Is pressing the continue button
        params->cookButton.pressed = 1;
    }
    else {
        for (int i = INGREDIENT_QUEUE - 1; i >= 0; i--) {
            GameSceneIngredient *item = params->ingredients[i];
            if (item && SDL_PointInRect(&params->cursor, &item->rect)) {
                // Is pressing a floating ingredient, start dragging
                params->draggingIngredient = item;
                params->dragOffset.x = x - item->rect.x;
                params->dragOffset.y = y - item->rect.y;
                break;
            }
        }
    }
}

static void gameSceneMouseUp(Scene *scene, int x, int y) {
    GameSceneParams *params = scene->params;
    if (params->cookButton.pressed) {
        params->cookButton.pressed = 0;
        if (SDL_PointInRect(&params->cursor, &params->cookButton.rect)) {
            // Released on the continue button
            params->finished = 1;
        }
    }
    else if (params->draggingIngredient) {
        // Released an dragging ingredient
        GameSceneIngredient *item = params->draggingIngredient;
        // Make sure to not exceed the right boundary of the screen
        if (item->rect.x > INGREDIENT_MAX_X) {
            item->rect.x = INGREDIENT_MAX_X;
        }

        SDL_Point point = { item->rect.x, item->rect.y };
        if (scene->animation == params->idleAnimation && SDL_PointInRect(&point, &dragTargetRect)) {
            // On the cutting board, remove the dragging item, hide the continue button, and switch to the cutting animation
            params->counts[item->type]++;
            params->cookButton.hidden = 1;
            for (int i = 0; i < INGREDIENT_QUEUE; i++) {
                if (params->ingredients[i] == item) {
                    unsetGameSceneIngredient(params, i);
                    break;
                }
            }
            resetAnimation(params->actionAnimation);
            scene->animation = params->actionAnimation;
        }
        else {
            // Not on the cutting board, restore position
            item->accurateX = item->rect.x * 16;
        }
        params->draggingIngredient = NULL;
    }
}

static void gameSceneMouseMove(Scene *scene, int x, int y) {
    GameSceneParams *params = scene->params;
    params->cursor.x = x;
    params->cursor.y = y;

    GameSceneIngredient *item = params->draggingIngredient;
    if (item) {
        item->rect.x = x - params->dragOffset.x;
        item->rect.y = y - params->dragOffset.y;
    }
}

static Scene *(*updateGameScene(Scene *scene, int delta, Uint64 time))(SDL_Renderer *) {
    if (scene->fadeOutStart && processFadeOut(scene, time)) {
        return createGameToOutroScene;
    }

    GameSceneParams *params = scene->params;
    // BPM of BGM = 134
    Uint64 relativeTime = time - scene->startTime + 300;
    params->isAltIdleImage = (relativeTime * 2 * 134 / 60000) % 2 == 0;

    if (scene->animation == params->actionAnimation && isAnimationEnded(scene->animation, delta)) {
        // When the cutting animation is finished, show the continue button and switch back to idle animation
        params->cookButton.hidden = 0;
        resetAnimation(params->idleAnimation);
        scene->animation = params->idleAnimation;
    }
    else {
        updateAnimation(scene->animation, delta);
    }

    int isHandCursor = 0;

    if (params->finished) {
        startFadeOut(scene, time);
    }
    else {
        // Hovering on the continue button
        if (!params->cookButton.hidden) {
            if (params->cookButton.pressed || SDL_PointInRect(&params->cursor, &params->cookButton.rect)) {
                isHandCursor = 1;
            }
        }

        int freeIngredientIndex = -1;
        for (int i = 0; i < INGREDIENT_QUEUE; i++) {
            GameSceneIngredient *item = params->ingredients[i];
            if (!item) {
                // This slot is free, can generate a new item here later
                freeIngredientIndex = i;
                continue;
            }

            if (item == params->draggingIngredient) {
                // When dragging, keep the hand cursor
                isHandCursor = 1;
            }
            else {
                // Use an integer for x position
                item->accurateX -= delta;
                item->rect.x = item->accurateX / 16;
                item->rect.y = (int)(17 - item->waveType * SDL_sin((double)relativeTime * 3.14159265 * 134 / 60000) * 4);

                if (item->rect.x < -item->rect.w) {
                    // Outside the screen, remove this item
                    unsetGameSceneIngredient(params, i);
                }
                else if (SDL_PointInRect(&params->cursor, &item->rect)) {
                    // Is hovering on this item
                    isHandCursor = 1;
                }
            }
        }

        // Generate a new item at the free slot every INGREDIENT_DELAY ms
        if (time - params->lastIngredientGenerateTime >= INGREDIENT_DELAY && freeIngredientIndex >= 0) {
            params->lastIngredientGenerateTime = time;
            params->ingredients[freeIngredientIndex] = generateGameSceneIngredient(params);
        }
    }

    if (params->isHandCursor != isHandCursor) {
        params->isHandCursor = isHandCursor;
        SDL_SetCursor(isHandCursor ? g_handCursor : SDL_GetDefaultCursor());
    }

    return NULL;
}

Scene *createGameScene(SDL_Renderer *renderer) {
    Scene *scene = SDL_malloc(sizeof(Scene));
    GameSceneParams *params = SDL_malloc(sizeof(GameSceneParams));
    SDL_zerop(scene);
    SDL_zerop(params);

    AnimatedImage *idleAnimation = loadAnimationWebp(renderer, "images/cooking_idle.webp");
    AnimatedImage *actionAnimation = loadAnimationWebp(renderer, "images/cooking_action.webp");
    StaticImage *cookButton = loadImagePng(renderer, "images/button_cook.png");
    StaticImage *eyesSheet = loadImagePng(renderer, "images/eyes_sheet.png");
    StaticImage *ingredientsSheet = loadImagePng(renderer, "images/ingredients_sheet.png");
    if (!idleAnimation || !actionAnimation || !cookButton || !eyesSheet || !ingredientsSheet) {
        return NULL;
    }

    // Use only the first half of the frames to update the animation
    // The second half of the frames is for the beat animation
    idleAnimation->frameCount /= 2;
    params->eyesSheet = eyesSheet;
    params->ingredientsSheet = ingredientsSheet;
    params->ingredientsCount = ingredientsSheet->surface->w / INGREDIENT_WIDTH;
    params->counts = SDL_malloc(params->ingredientsCount * sizeof(*params->counts));
    params->typesQueue = SDL_malloc(INGREDIENT_PLAIN * sizeof(int));
    params->idleAnimation = idleAnimation;
    params->actionAnimation = actionAnimation;
    createUIButton(&params->cookButton, cookButton, 0, 0);

    scene->animation = idleAnimation;
    scene->music = loadAndPlayMusic("sounds/working_loop.ogg", -1);
    scene->update = updateGameScene;
    scene->draw = drawGameScene;
    scene->free = freeGameScene;
    scene->mouseDown = gameSceneMouseDown;
    scene->mouseUp = gameSceneMouseUp;
    scene->mouseMove = gameSceneMouseMove;
    scene->params = params;

    srand(time(NULL));
    generateTypesQueue(params->typesQueue, INGREDIENT_PLAIN);
    return scene;
}
