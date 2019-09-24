CFLAGS += -std=c11 -Wall -Wextra -gdwarf-2 -g3 -O0 -fno-omit-frame-pointer -Wno-unused-function -MMD -MP
GAME_LIBRARIES = -lm
PLATFORM_LIBRARIES = -lX11 -lXext -ldl

all: build/libgame.so build/platform

build/game.o: src/game.c
	$(CC) $(CFLAGS) -fPIC  -c src/game.c -o $@

build/assets.o: assets/triangle.bmp
	ld -r -b binary -o build/triangle.o assets/triangle.bmp
	ld -r build/triangle.o -o build/assets.o

build/libgame.so: build/assets.o build/game.o
	$(CC) $(CFLAGS) -fPIC -shared $^ -o $@ $(GAME_LIBRARIES)

build/platform: src/linux_x11.c
	$(CC) $(CFLAGS) src/linux_x11.c -o $@ $(PLATFORM_LIBRARIES) -Wl,--export-dynamic

clean:
	rm -f build/*

dir:
	mkdir -p build

-include build/*.d

.PHONY: all clean dir
