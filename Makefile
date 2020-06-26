#OBJS specifies which files to compile as part of the project
OBJS = mos6502/mos6502.cpp joystick_adapter.cpp dynawave.cpp gtc.cpp

#CC specifies which compiler we're using
CC = g++

SDL_ROOT = ..\SDL2-2.0.12\x86_64-w64-mingw32

#INCLUDE_PATHS specifies the additional include paths we'll need
INCLUDE_PATHS = -I$(SDL_ROOT)\include\SDL2

#LIBRARY_PATHS specifies the additional library paths we'll need
LIBRARY_PATHS = -L $(SDL_ROOT)\lib

DEFINES = -D CPU_6502_STATIC -D CPU_6502_USE_LOCAL_HEADER

#COMPILER_FLAGS specifies the additional compilation options we're using
# -w suppresses all warnings
# -Wl,-subsystem,windows gets rid of the console window
COMPILER_FLAGS = -w -Wl,-subsystem,windows

#LINKER_FLAGS specifies the libraries we're linking against
LINKER_FLAGS = -lmingw32 -lSDL2main -lSDL2

#OBJ_NAME specifies the name of our exectuable
OBJ_NAME = GameTankEmulator

#This is the target that compiles our executable
all : $(OBJS)
	$(CC) $(OBJS) $(INCLUDE_PATHS) $(LIBRARY_PATHS) $(COMPILER_FLAGS) $(DEFINES) $(LINKER_FLAGS) -o $(OBJ_NAME)