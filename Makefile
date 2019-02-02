# Directory with output compiled files
OUT_BUILD = ./build
#Directory with output for test purposes
OUT_TEST = ./test

# Source files
SRC = ./src
SOURCE_WILDCARD = $(wildcard $(SRC)/*.c)
SOURCE_FILE_NAMES = $(basename $(notdir $(SOURCE_WILDCARD)))

# Build files - Test files filtered out
SOURCE_WILDCARD_FILTERED = $(filter-out $(wildcard $(SRC)/*.test.c), $(SOURCE_WILDCARD))
BUILD_OUTPUT_FILES = $(addprefix $(OUT_BUILD)/, $(SOURCE_WILDCARD_FILTERED))
BUILD_FILES = $(addprefix $(SRC)/, $(SOURCE_WILDCARD_FILTERED))

# Test files
TEST_WILDCARD = $(wildcard $(SRC)/*.test.c)
TEST_FILES = $(basename $(notdir $(TEST_WILDCARD)))
TEST_OUTPUT_FILES = $(addprefix $(OUT_TEST)/, $(TEST_FILES))

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

# ---------------------
# SECTION: BUILD
# ---------------------

# Default command to build: make
build: clean compile link 

compile: $(SOURCE_WILDCARD_FILTERED)
	$(foreach f, $^, \
		$(CC) -g -c $f -o $(OUT_BUILD)/$(basename $(notdir $f)).o $(CFLAGS);)

link: $(addsuffix .o, $(BUILD_OUTPUT_FILES))
	$(CC) $(DEBUG) -DNDEBUG -o $(OUT_BUILD)/$(MAIN) $(OUT_BUILD)/$(MAIN).o \
		$^ $(LIBS) $(CFLAGS)

# ---------------------
# SECTION: TEST
# ---------------------

# Command build and run tests: make test
test: clean-test build-test
	$(foreach f, $(TEST_OUTPUT_FILES),./$f > $f.output.txt;)

# Build test files: make build-test
build-test: compile-all-test link-test-objs

compile-all-test: $(SOURCE_WILDCARD)
	$(foreach f, $^, \
		$(CC) -g -c $f -o $(OUT_TEST)/$(basename $(notdir $f)).o $(CFLAGS);)

link-test-objs: $(addsuffix .o, $(TEST_OUTPUT_FILES))
	$(foreach f, $^, \
		$(CC) -g -o $(basename $f) $(addsuffix .o, $(basename $(basename $f))) \
		$(basename $f).o $(LIBS) $(CFLAGS);)

# ---------------------
# SECTION: CLEAN
# ---------------------

# Command to clean: make clean
clean:
	make clean-build
	
clean-build:
	rm -f $(OUT_BUILD)/*

clean-test:
	rm -f $(OUT_TEST)/*

# ---------------------
# SECTION: RUN
# ---------------------

# Command to build and run main: make run
run: clean-build build
	$(OUT_BUILD)/$(MAIN)