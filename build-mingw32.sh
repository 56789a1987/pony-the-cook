#!/bin/bash

LIBWEBP_DIR=deps/libwebp
SDL2_DIR=deps/SDL2/i686-w64-mingw32
SDL2_IMAGE_DIR=deps/SDL2_image/i686-w64-mingw32
SDL2_MIXER_DIR=deps/SDL2_mixer/i686-w64-mingw32

mkdir -p build
windres src/resources.rc -o build/resources.o

i686-w64-mingw32-gcc -Os -s src/*.c build/resources.o \
    $LIBWEBP_DIR/src/dec/*.c $LIBWEBP_DIR/src/dsp/*.c $LIBWEBP_DIR/src/demux/*.c $LIBWEBP_DIR/src/utils/*.c \
    -o game.exe \
    -I $LIBWEBP_DIR -I $LIBWEBP_DIR/src \
    -I $SDL2_DIR/include -I $SDL2_DIR/include/SDL2 -L $SDL2_DIR/lib \
    -I $SDL2_IMAGE_DIR/include -L $SDL2_IMAGE_DIR/lib \
    -I $SDL2_MIXER_DIR/include -L $SDL2_MIXER_DIR/lib \
    -mwindows -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_mixer
