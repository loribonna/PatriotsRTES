# Directory with output compiled files
OUT_BUILD = ./build
#Directory with output for test purposes
OUT_TEST = ./test

# Source files
SRC = ./src

# Target file to be compiled
MAIN = patriots

#Files to compile
BASE_FILES = $(MAIN) gestor launchers
SOURCE_FILES = $(addsuffix .c, $(addprefix $(SRC)/, $(BASE_FILES)))
OUT_FILES = $(addsuffix .o, $(addprefix $(OUT_BUILD)/, $(BASE_FILES)))

# Headers
INCLUDE = -I./include

# Compiler to be used
CC = gcc
# Options to the compiler
CFLAGS = -Wall -lrt -lm
ALL_FLAGS = $(INCLUDE) $(CFLAGS)

# Libraries
LIB_ALLEGRO = -lpthread `allegro-config --libs`
LIB_PTASK = -L./lib -lptask

LIBS = $(LIB_PTASK) $(LIB_ALLEGRO)

# ---------------------
# SECTION: BUILD
# ---------------------

all: clean build

# Default command to build: make
build: compile link 

compile: $(SOURCE_FILES)
	$(foreach f, $^, \
		$(CC) -g -c $f -o $(OUT_BUILD)/$(basename $(notdir $f)).o $(ALL_FLAGS);)

link: $(OUT_FILES)
	$(CC) -o $(OUT_BUILD)/$(MAIN) $(OUT_FILES) $(LIBS) $(ALL_FLAGS)

# ---------------------
# SECTION: CLEAN
# ---------------------

# Command to clean: make clean
clean:
	rm -f $(OUT_BUILD)/*

# ---------------------
# SECTION: RUN
# ---------------------

# Command to build and run main: make run
run: clean-build build
	$(OUT_BUILD)/$(MAIN)