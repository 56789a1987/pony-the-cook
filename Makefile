CFLAGS=-g -Wall -Wextra -Wno-unused-parameter `pkg-config --cflags libwebpdemux sdl2 SDL2_image SDL2_mixer`
LDFLAGS=`pkg-config --libs libwebpdemux sdl2 SDL2_image SDL2_mixer`

game:
	gcc $(CFLAGS) src/*.c -o game $(LDFLAGS)

.PHONY: run
run: game
	./game

.PHONY: clean
clean:
	rm game
