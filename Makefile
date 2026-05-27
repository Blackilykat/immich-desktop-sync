CC=gcc
CLIBS=-lcurl
CFLAGS=-Wall -Wextra -g
SRC=${wildcard ./src/*.c}

.PHONY: build run clean

build: $(SRC)
	mkdir -p build
	$(CC) $(CFLAGS) $(CLIBS) -o build/main $(SRC)

run: build
	./build/main

debug: build
	gdb --eval-command=run ./build/main

clean:
	rm -rf build
