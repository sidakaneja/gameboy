INCLUDES = -I /opt/homebrew/include -I ./include
LINK = -L /opt/homebrew/lib -lSDL2
FLAGS = -g -Wall -Wextra 
OBJECTS = ./src/emulator.c ./src/cpu.c ./src/memory.c
all: clean
	gcc ${FLAGS} ${INCLUDES} ${LINK} ${OBJECTS} ./src/main.c -o ./bin/main
clean:
	rm -rf ./bin/*