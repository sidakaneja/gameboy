INCLUDES = -I /opt/homebrew/include -I ./include
LINK = -L /opt/homebrew/lib -lSDL2
FLAGS = -g -Wall -Wextra 

all: 
	gcc ${FLAGS} ${INCLUDES} ${LINK} ./src/main.c -o ./bin/main