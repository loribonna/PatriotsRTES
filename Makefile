
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

# Build.
build: compile link 

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

# Clean, build and run as superuser (in order to use ptask).
run: all
	sudo $(OUT_BUILD)/$(MAIN)