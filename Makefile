CC=gcc
CLIBS=-lcurl
CFLAGS=-Wall -Wextra -g -fsanitize=address
CRELEASEFLAGS=-Wall -Wextra -O3 -s
SRC=${wildcard ./src/*.c}

.PHONY: build run clean

build: $(SRC)
	mkdir -p build
	$(CC) $(CFLAGS) $(CLIBS) -o build/immich-desktop-sync $(SRC)

release: $(SRC)
	mkdir -p build
	$(CC) $(CRELEASEFLAGS) $(CLIBS) -o build/immich-desktop-sync $(SRC)

run: build
	ASAN_OPTIONS=detect_leaks=1 LSAN_OPTIONS=suppressions=address-sanitizer-suppressions.txt ./build/immich-desktop-sync

runrelease: release
	./build/immich-desktop-sync

debug: build
	gdb --eval-command=run ./build/immich-desktop-sync

clean:
	rm -rf build
