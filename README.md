Windows prebuilt binaries can be found in Actions

## Build on Linux

### Dependencies

- Ubuntu and other Debian based `build-essential libwebp-dev libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev`

- Fedora and other RPM based `gcc-c++ libwebp-devel SDL2-devel SDL2_image-devel SDL2_mixer-devel`

- Arch based `base-devel libwebp sdl2 sdl2_image sdl2_mixer`

### Build the source code

```sh
make
```

## Build on Windows

### Dependencies

- Install [MSYS2](https://www.msys2.org/)

- Start mingw32 and install `i686-w64-mingw32-gcc`
  ```sh
  pacman -S --needed i686-w64-mingw32-gcc
  ```

- **libwebp** ([releases](https://github.com/webmproject/libwebp/tags)), extract to `deps/libwebp`

- **SDL2** ([releases](https://github.com/libsdl-org/SDL/releases))
  - **Make sure you're downloading SDL2 not SDL3**
  - Download `SDL2-devel-2.x.x-mingw.tar.gz` and extract to `deps/SDL2`
  - Download `SDL2-2.x.x-win32-x86.zip` and extract `SDL2.dll` to the working directory

- **SDL2 image** ([releases](https://github.com/libsdl-org/SDL_image/releases))
  - Download `SDL2_image-devel-2.x.x-mingw.tar.gz` and extract to `deps/SDL2_image`
  - Download `SDL2_image-2.x.x-win32-x86.zip` and extract `SDL2_image.dll` to the working directory

- **SDL2 mixer** ([releases](https://github.com/libsdl-org/SDL_mixer/releases))
  - Download `SDL2_mixer-devel-2.x.x-mingw.tar.gz` and extract to `deps/SDL2_mixer`
  - Download `SDL2_mixer-2.x.x-win32-x86.zip` and extract `SDL2_mixer.dll` and `optional/libogg-0.dll` to the working directory

### Build the source code

Start mingw32, navigate to the working directory and run
```sh
./build-mingw32.sh
```

### Debugging

To enable the console window and show outputs, edit `build-mingw32.sh` and remove `-mwindows` gcc args.

### Distributing

To distribute the binary, also add *.dll, the images directory and the sounds directory to the archive.
