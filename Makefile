CFLAGS=-g -Wall -Wextra -Wno-unused-parameter `pkg-config --cflags libwebpdecoder libwebpdemux sdl2 SDL2_mixer`
LDFLAGS=`pkg-config --libs libwebpdecoder libwebpdemux sdl2 SDL2_mixer`

game:
	gcc $(CFLAGS) src/*.c -o game $(LDFLAGS)

.PHONY: run
run: game
	./game

.PHONY: clean
clean:
	rm game
