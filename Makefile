# Directory with output compiled files
OUT_BUILD = ./build
#Directory with output for test purposes
OUT_TEST = ./test
# Source files
SRC = ./src
# Headers
INCLUDE = ./include

# Target file to be compiled
MAIN = patriots
# Compiler to be used
CC = gcc
# Options to the compiler
CFLAGS = -I$(INCLUDE) -Wall -lrt -lm
# Libraries
LIB_ALLEGRO = -lpthread `allegro-config --libs`
LIB_PTASK = -L./lib -lptask

LIBS = $(LIB_PTASK) $(LIB_ALLEGRO)

# Default command to build: make
build: $(OUT_BUILD)/$(MAIN)

$(OUT_BUILD)/$(MAIN): $(OUT_BUILD)/$(MAIN).o
	$(CC) $(DEBUG) -o $(OUT_BUILD)/$(MAIN) $(OUT_BUILD)/$(MAIN).o $(LIBS) $(CFLAGS)

$(OUT_BUILD)/$(MAIN).o: $(SRC)/$(MAIN).c
	$(CC) $(DEBUG) -c $(SRC)/$(MAIN).c -o $(OUT_BUILD)/$(MAIN).o $(CFLAGS)

# Command to test
test: $(OUT_TEST)/$(MAIN)
	$(OUT_TEST)/$(MAIN) > $(OUT_TEST)/output.txt

$(OUT_TEST)/$(MAIN): $(OUT_TEST)/$(MAIN).o
	$(CC) -g -DTEST -o $(OUT_TEST)/$(MAIN) $(OUT_TEST)/$(MAIN).o $(LIBS) $(CFLAGS)

$(OUT_TEST)/$(MAIN).o: $(SRC)/$(MAIN).c
	$(CC) -g -DTEST -c $(SRC)/$(MAIN).c -o $(OUT_TEST)/$(MAIN).o $(CFLAGS)

# Command to clean inline: make clean
clean-build:
	rm -f $(OUT_BUILD)/*

clean-test:
	rm -f $(OUT_TEST)/*

run:
	make clean-build
	make build
	$(OUT_BUILD)/$(MAIN)