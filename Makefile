# Directory with output compiled files
OUT = ./build
# Source files
SRC = ./src
# Headers
INCLUDE = ./include

# Target file to be compiled
MAIN = patriots
# Compiler to be used
CC = gcc
# Options to the compiler
CFLAGS = -I${INCLUDE} -Wall -lrt -lm
# Libraries
LIB_ALLEGRO = -lpthread `allegro-config --libs`
LIB_PTASK = -L./lib

LIBS = ${LIB_PTASK} ${LIB_ALLEGRO}

# Default command to build: make
$(MAIN): $(MAIN).o
	$(CC) $(CFLAGS) $(LIBS) -o $(OUT)/$(MAIN) ${OUT}/$(MAIN).o

$(MAIN).o: ${SRC}/$(MAIN).c
	$(CC) $(CFLAGS) -c ${SRC}/$(MAIN).c -o ${OUT}/$(MAIN).o

# Command to clean inline: make clean
clean:
	rm $(OUT)/*