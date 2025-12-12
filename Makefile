# specifes the directory to place the build files.
OUT_DIR = build
INSTALL_DIR = bin
DIST_DIR = dist

#CC specifies which C compiler we're using
CC = gcc

#CPPC specifies which C++ compiler we're using
CPPC = g++

XCOMP = no
TAG = ""
NIGHTLY = no

ifeq ($(OS), Windows_NT)
	ifeq ($(XCOMP), yes)
		FIND = find
	else
		FIND = /bin/find
	endif
else
	FIND = find
endif

#OBJS specifies which files to compile as part of the project
SRCS := $(filter-out %example_implot.cpp, $(shell $(FIND) src -name "*.cpp"))
OBJS = $(SRCS:%=$(OUT_DIR)/%.o)
NATIVE_SRCS = src/tinyfd/tinyfiledialogs.c src/whereami/whereami.c
NATIVE_OBJS = $(NATIVE_SRCS:%=$(OUT_DIR)/%.o)

#BIN_NAME specifies the name of our exectuable
BIN_NAME = GameTankEmulator
ZIP_NAME = GTE_$(OS).zip

WEB_SHELL ?= web/shell.html
WEB_ASSETS ?= web/static/

EXTRA_INCLUDES = -Isrc/imgui -Isrc/imgui/backends -Isrc/imgui/ext/implot -Isrc/whereami

ifeq ($(NIGHTLY), yes)
	TAG = _$(shell date '+%Y%m%d')
endif

OS ?= $(shell uname)

ifneq ($(origin WINDOW_TITLE), undefined)
	DEFINES = -D WINDOW_TITLE="\"$(WINDOW_TITLE)\""
endif

ifeq ($(OS), Windows_NT)
	ifeq ($(XCOMP), yes)
		CC = i686-w64-mingw32-gcc-posix
		CPPC = i686-w64-mingw32-g++-posix
	endif
	BIN_NAME := $(BIN_NAME).exe

	ZIP_NAME = bin/GTE_Win32$(TAG).zip
	SDL_ROOT = ../SDL2-2.26.2/x86_64-w64-mingw32

	#INCLUDE_PATHS specifies the additional include paths we'll need
	EXTRA_INCLUDES += -Iinclude
	INCLUDE_PATHS = -I$(SDL_ROOT)/include/SDL2 $(EXTRA_INCLUDES)

	#LIBRARY_PATHS specifies the additional library paths we'll need
	LIBRARY_PATHS = -L$(SDL_ROOT)/lib

	OBJS += $(NATIVE_OBJS)

	#COMPILER_FLAGS specifies the additional compilation options we're using
	# -Wl,-subsystem,windows gets rid of the console window
	# change subsystem,windows to subsystem,console to get printfs on command line
	COMPILER_FLAGS = -Wl,-subsystem,console
	DEFINES += -D _WIN32
	DEFINES += -DLIBREMIDI_WINMM=1 -DUNICODE=1 -D_UNICODE=1 -DLIBREMIDI_HEADER_ONLY=1

	ifeq ($(WRAPPERMODE), yes)
		COMPILER_FLAGS += -D DEFAULT_ROM_PATH='"gamedata.gtr"' -D WRAPPER_MODE=1
	endif

	#LINKER_FLAGS specifies the libraries we're linking against
	LINKER_FLAGS = -lmingw32 -lSDL2main -lSDL2 -Wl,-Bstatic -mwindows -lm -ldinput8 -ldxguid -ldxerr8 -luser32 -lgdi32 -lwinmm -limm32 -lcomdlg32 -lole32 -loleaut32 -lshell32 -lversion -luuid -static-libgcc -lsetupapi
	LINKER_FLAGS += -lwinmm
else ifeq ($(OS), wasm)
	CC = emcc
	CPPC = emcc

	#Only run this if $PRELOAD_ROM is set
	#This should only be required when building for Nix for now
	ifneq ($(origin PRELOAD_ROM), undefined)
	    COMPILER_FLAGS += --preload-file $(PRELOAD_ROM)
	    LINKER_FLAGS += --preload-file $(PRELOAD_ROM)
	endif

	COMPILER_FLAGS += -s USE_SDL=2 -D WASM_BUILD -D EMBED_ROM_FILE='"$(ROMFILE)"' -s TOTAL_STACK=512mb
	DEFINES += -D DISABLE_ESC
	BIN_NAME = index.html
	LINKER_FLAGS += --embed-file $(ROMFILE) --shell-file $(WEB_SHELL) -s EXPORTED_FUNCTIONS='["_LoadRomFile", "_main", "_SetButtons", "_PauseEmulation", "_ResumeEmulation", "_takeScreenShot"]' -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' -lidbfs.js
	SRCS :=  $(filter-out $(foreach src,$(SRCS),$(if $(findstring imgui,$(src)), $(src))),$(SRCS))
	SRCS := $(filter-out %window.cpp, $(SRCS))
else
	OBJS += $(NATIVE_OBJS)
	COMPILER_FLAGS = -g `sdl2-config --cflags` $(EXTRA_INCLUDES)
	LINKER_FLAGS = `sdl2-config --libs`
endif


DEFINES += -D CPU_6502_STATIC -D CPU_6502_USE_LOCAL_HEADER -D CMOS_INDIRECT_JMP_FIX

#This is the target that compiles our executable
.PHONY: all bin dist install
all: bin dist

bin: $(OUT_DIR)/$(BIN_NAME)
dist: $(OUT_DIR)/$(ZIP_NAME)
	@mkdir -p $(DIST_DIR)
	cp $^ $(DIST_DIR)

install: bin
	@mkdir -p $(INSTALL_DIR)/bin
	install -t $(INSTALL_DIR)/bin $(OUT_DIR)/$(BIN_NAME)
ifeq ($(OS), Windows_NT)
	install -t $(INSTALL_DIR)/bin $(SDL_ROOT)/bin/SDL2.dll
else ifeq ($(OS), wasm)
	@mkdir -p $(INSTALL_DIR)/bin/static
	cp -r $(WEB_ASSETS) $(INSTALL_DIR)/bin/static
	install -t $(INSTALL_DIR)/bin $(OUT_DIR)/index.js
	install -t $(INSTALL_DIR)/bin $(OUT_DIR)/index.wasm
endif

$(OUT_DIR)/$(ZIP_NAME): bin commit_hash.txt
	@mkdir -p $(@D)/img
ifeq ($(OS), Windows_NT)
	cp $(SDL_ROOT)/bin/SDL2.dll $(OUT_DIR)
endif
ifeq ($(OS), wasm) #separate ifblock on purpose
	cd $(OUT_DIR); zip -9 -y -r -q $(ZIP_NAME) $(BIN_NAME) static index.js index.wasm commit_hash.txt
else
	cd $(OUT_DIR); zip -9 -y -r -q $(ZIP_NAME) $(BIN_NAME) SDL2.dll img commit_hash.txt
endif

# Allow manually setting a commit hash
# If MANUAL_COMMIT_HASH is set, it will be used instead of actually querying git
# This is done for Nix reasons :/
commit_hash.txt:
ifeq ($(origin MANUAL_COMMIT_HASH), undefined)
	git rev-parse HEAD > $(OUT_DIR)/commit_hash.txt
else
	echo $(MANUAL_COMMIT_HASH) > $(OUT_DIR)/commit_hash.txt
endif

$(OUT_DIR)/%.cpp.o: %.cpp
	@mkdir -p $(@D)
	$(CPPC) -c $< -o $@ $(INCLUDE_PATHS) $(COMPILER_FLAGS) $(DEFINES) -std=c++20

$(OUT_DIR)/%.c.o: %.c
	@mkdir -p $(@D)
	$(CC) -c $< -o $@ $(INCLUDE_PATHS) $(COMPILER_FLAGS) $(DEFINES)

$(OUT_DIR)/$(BIN_NAME): $(OBJS)
	$(CPPC) $(COMPILER_FLAGS) -o $@ $^ $(LIBRARY_PATHS) $(LINKER_FLAGS) -std=c++20
ifeq ($(OS), wasm)
	cp -r $(WEB_ASSETS) $(OUT_DIR)/static
endif


clean:
	rm -rf $(OUT_DIR)

clean-all: clean
	rm -rf $(INSTALL_DIR)
	rm -rf $(DIST_DIR)
