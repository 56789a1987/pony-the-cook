game:
	gcc -Os -s src/*.c -o game -lwebpdemux -lSDL2 -lSDL2_image -lSDL2_mixer

.PHONY: clean
clean:
	rm game
