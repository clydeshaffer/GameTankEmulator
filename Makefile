# specifes the directory to place the build files.
O = build
INSTALL_DIR = bin
DIST_DIR = dist

#CC specifies which C compiler we're using
CC = gcc

#CPPC specifies which C++ compiler we're using
CPPC = g++

XCOMP = no
TAG = ""
NIGHTLY = no

#OBJS specifies which files to compile as part of the project
SRCS = mos6502/mos6502.cpp joystick_adapter.cpp audio_coprocessor.cpp gte.cpp font.cpp
OBJS = $(SRCS:%=$O/%.o)
NATIVE_SRCS = tinyfd/tinyfiledialogs.c
NATIVE_OBJS = $(NATIVE_SRCS:%=$O/%.o)

#BIN_NAME specifies the name of our exectuable
BIN_NAME = GameTankEmulator
ZIP_NAME = GTE_$(OS).zip

WEB_SHELL = shell.html

ifeq ($(NIGHTLY), yes)
	TAG = _$(shell date '+%Y%m%d')
endif

ifndef OS
	OS=$(shell uname)
endif

ifeq ($(OS), Windows_NT)
	ifeq ($(XCOMP), yes)
		CC = i686-w64-mingw32-gcc
		CPPC = i686-w64-mingw32-g++
	endif
	BIN_NAME := $(BIN_NAME).exe

	ZIP_NAME = bin/GTE_Win32$(TAG).zip
	SDL_ROOT = ../SDL2-2.26.2/x86_64-w64-mingw32

	#INCLUDE_PATHS specifies the additional include paths we'll need
	INCLUDE_PATHS = -I$(SDL_ROOT)/include/SDL2

	#LIBRARY_PATHS specifies the additional library paths we'll need
	LIBRARY_PATHS = -L$(SDL_ROOT)/lib

	#COMPILER_FLAGS specifies the additional compilation options we're using
	# -w suppresses all warnings
	# -Wl,-subsystem,windows gets rid of the console window
	# change subsystem,windows to subsystem,console to get printfs on command line
	COMPILER_FLAGS = -w -Wl,-subsystem,windows -std=c++17
	DEFINES = -D _WIN32

	#LINKER_FLAGS specifies the libraries we're linking against
	LINKER_FLAGS = -lmingw32 -lSDL2main -lSDL2 -Wl,-Bstatic -mwindows -lm -ldinput8 -ldxguid -ldxerr8 -luser32 -lgdi32 -lwinmm -limm32 -lcomdlg32 -lole32 -loleaut32 -lshell32 -lversion -luuid -static-libgcc -lsetupapi
else
	COMPILER_FLAGS = -w -std=c++17
	LINKER_FLAGS = -lSDL2
endif
ifeq ($(OS), wasm)
	CC = emcc
	CPPC = emcc
	COMPILER_FLAGS += -s USE_SDL=2 -D WASM_BUILD -D EMBED_ROM_FILE='"$(ROMFILE)"'
	BIN_NAME = index.html
	LINKER_FLAGS += --embed-file $(ROMFILE) --shell-file web/$(WEB_SHELL) -s EXPORTED_FUNCTIONS='["_LoadRomFile", "_main", "_SetButtons"]' -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]'
else
	OBJS += $(NATIVE_OBJS)
endif


DEFINES += -D CPU_6502_STATIC -D CPU_6502_USE_LOCAL_HEADER -D CMOS_INDIRECT_JMP_FIX

#This is the target that compiles our executable
.PHONY: all bin dist install
all : bin dist

bin: $O/$(BIN_NAME)
dist : $O/$(ZIP_NAME)
	@mkdir -p $(DIST_DIR)
	cp $^ $(DIST_DIR)

install : bin
	@mkdir -p $(INSTALL_DIR)/bin
	install -t $(INSTALL_DIR)/bin $O/$(BIN_NAME)
ifeq ($(OS), Windows_NT)
	install -t $(INSTALL_DIR)/bin $(SDL_ROOT)/bin/SDL2.dll
endif
ifeq ($(OS), wasm)
	install -t $(INSTALL_DIR)/bin web/gamepad.png
	install -t $(INSTALL_DIR)/bin $O/index.js
	install -t $(INSTALL_DIR)/bin $O/index.wasm
endif

$O/$(ZIP_NAME) : bin commit_hash.txt
	@mkdir -p $(@D)/img
ifeq ($(OS), Windows_NT)
	cp $(SDL_ROOT)/bin/SDL2.dll $O
endif
ifeq ($(OS), wasm)
	cd $O; zip -9 -y -r -q $(ZIP_NAME) $(BIN_NAME) gamepad.png index.js index.wasm commit_hash.txt
else
	cd $O; zip -9 -y -r -q $(ZIP_NAME) $(BIN_NAME) SDL2.dll img commit_hash.txt
endif

commit_hash.txt :
	git rev-parse HEAD > $O/commit_hash.txt

$O/%.cpp.o : %.cpp
	@mkdir -p $(@D)
	$(CPPC) -c $< -o $@ $(INCLUDE_PATHS) $(COMPILER_FLAGS) $(DEFINES)

$O/%.c.o : %.c
	@mkdir -p $(@D)
	$(CC) -c $< -o $@ $(INCLUDE_PATHS) $(COMPILER_FLAGS) $(DEFINES)

$O/$(BIN_NAME) : $(OBJS)
	$(CPPC) $(COMPILER_FLAGS) -o $@ $^ $(LIBRARY_PATHS) $(LINKER_FLAGS)

clean:
	rm -rf $O

clean-all: clean
	rm -rf $(INSTALL_DIR)
	rm -rf $(DIST_DIR)
