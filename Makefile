
SHELL = /bin/sh

# ----------------------------------------------------------------------
# COMPILE OPTIONS
# ----------------------------------------------------------------------

# Compiler to be used.
CC = gcc

# External headers.
INCLUDE = -I./include

# Options to the compiler.
CFLAGS = -Wall -lrt -lm
ALL_FLAGS = $(INCLUDE) $(CFLAGS)

# Libraries.
LIB_ALLEGRO = -lpthread `allegro-config --libs`
LIB_PTASK = -L./lib -lptask

LIBS = $(LIB_PTASK) $(LIB_ALLEGRO)

# ----------------------------------------------------------------------
# FILES AND DIRECTORIES
# ----------------------------------------------------------------------

# Directory with output compiled files.
OUT_BUILD = ./build

# Source files.
SRC = ./src

# Target filename.
MAIN = patriots

# Files to compile.
BASE_FILES = $(MAIN) gestor launchers
SOURCE_FILES = $(addsuffix .c, $(addprefix $(SRC)/, $(BASE_FILES)))
OUT_FILES = $(addsuffix .o, $(addprefix $(OUT_BUILD)/, $(BASE_FILES)))

# ----------------------------------------------------------------------
# TARGETS
# ----------------------------------------------------------------------

# Default.
all: clean build

#	# ---------------------
# BUILD
#	# ---------------------

# Check if the build directory exists and creates it if it doesn't.
check-build:
	if ! [ -d $(OUT_BUILD) ]; then mkdir $(OUT_BUILD); fi

# Build.
build: check-build compile link 

# Compile all specified source files.
compile: $(SOURCE_FILES)
	$(foreach f, $^, \
		$(CC) -g -c $f -o $(OUT_BUILD)/$(basename $(notdir $f)).o $(ALL_FLAGS);)

# Link all builded source files and create executable.
link: $(OUT_FILES)
	$(CC) -o $(OUT_BUILD)/$(MAIN) $(OUT_FILES) $(LIBS) $(ALL_FLAGS)

#	# ---------------------
# CLEAN
#	# ---------------------

# Command to clean: make clean
clean:
	rm -f $(OUT_BUILD)/*

#	# ---------------------
# SECTION: RUN
#	# ---------------------

# Check if the current environment has display support.
check-env:
ifeq ($(DISPLAY),)
	$(error Display mode NOT supported.)
else
	$(info Display mode supported.)
endif

# Clean, build and run as superuser (in order to use ptask).
run: check-env all
	$(info Executing PATRIOTS (as superuser)...)
	sudo $(OUT_BUILD)/$(MAIN)
