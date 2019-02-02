# Directory with output compiled files
OUT_BUILD = ./build
#Directory with output for test purposes
OUT_TEST = ./test
# Source files
SRC = ./src
# Test files
TEST_WILDCARD = $(wildcard $(SRC)/*.test.c)
TEST_FILES = $(basename $(notdir $(TEST_WILDCARD)))
TEST_OUTPUT_FILES = $(addprefix $(OUT_TEST)/, $(TEST_FILES))
TEST_SOURCE_FILES = $(addprefix $(SRC)/, $(TEST_FILES))

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
build: $(SRC)/$(MAIN)
	$(OUT_BUILD)/$(MAIN): $(OUT_BUILD)/$(MAIN).o
		$(CC) $(DEBUG) -o $(OUT_BUILD)/$(MAIN) $(OUT_BUILD)/$(MAIN).o $(LIBS) $(CFLAGS)

	$(OUT_BUILD)/$(MAIN).o: $(SRC)/$(MAIN).c
		$(CC) $(DEBUG) -c $(SRC)/$(MAIN).c -o $(OUT_BUILD)/$(MAIN).o $(CFLAGS)

# Command build and run tests: make test
test: clean-test build-test
	$(foreach f, $(TEST_OUTPUT_FILES),./$f > $f.output.txt;)

# Build test files: make build-test
build-test: create-test-objs build-test-objs

create-test-objs: $(addsuffix .c, $(TEST_SOURCE_FILES))
	$(foreach f, $^,$(CC) -g -DTEST -c $f \
		-o $(OUT_TEST)/$(basename $(notdir $f)).o $(CFLAGS);)

build-test-objs: $(addsuffix .o, $(TEST_OUTPUT_FILES))
	$(foreach f, $^,$(CC) -g -DTEST -o $(basename $f) $f $(CFLAGS);)

# Command to clean: make clean
clean:
	make clean-build
	
clean-build:
	rm -f $(OUT_BUILD)/*

clean-test:
	rm -f $(OUT_TEST)/*

# Command to build and run main: make run
run: clean-build build
	$(OUT_BUILD)/$(MAIN)