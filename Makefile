#OBJS specifies which files to compile as part of the project
OBJS = mos6502/mos6502.cpp joystick_adapter.cpp dynawave.cpp gte.cpp
C_OBJS = tinyfd/tinyfiledialogs.c

#CC specifies which C compiler we're using
CC = gcc

#CPPC specifies which C++ compiler we're using
CPPC = g++

ifeq ($(OS), Windows_NT)
	SDL_ROOT = ../SDL2-2.0.12/i686-w64-mingw32

	#INCLUDE_PATHS specifies the additional include paths we'll need
	INCLUDE_PATHS = -I$(SDL_ROOT)/include/SDL2

	#LIBRARY_PATHS specifies the additional library paths we'll need
	LIBRARY_PATHS = -L$(SDL_ROOT)/lib

	#COMPILER_FLAGS specifies the additional compilation options we're using
	# -w suppresses all warnings
	# -Wl,-subsystem,windows gets rid of the console window
	# change subsystem,windows to subsystem,console to get printfs on command line
	COMPILER_FLAGS = -w -Wl,-subsystem,windows
	
	#LINKER_FLAGS specifies the libraries we're linking against
	LINKER_FLAGS = -lmingw32 -lSDL2main -lSDL2 -Wl,-Bstatic -mwindows -lm -ldinput8 -ldxguid -ldxerr8 -luser32 -lgdi32 -lwinmm -limm32 -lcomdlg32 -lole32 -loleaut32 -lshell32 -lversion -luuid -static-libgcc -lsetupapi
else
	COMPILER_FLAGS = -w
	LINKER_FLAGS = -lSDL2
endif

DEFINES = -D CPU_6502_STATIC -D CPU_6502_USE_LOCAL_HEADER -D CMOS_INDIRECT_JMP_FIX

#OBJ_NAME specifies the name of our exectuable
OBJ_NAME = GameTankEmulator

#This is the target that compiles our executable
all : $(C_OBJS) $(OBJS)
	$(CC) -c $(C_OBJS) $(INCLUDE_PATHS) $(COMPILER_FLAGS) $(DEFINES)
	$(CPPC) -c $(OBJS) $(INCLUDE_PATHS) $(COMPILER_FLAGS) $(DEFINES)
	$(CPPC) $(INCLUDE_PATHS) $(COMPILER_FLAGS) -o $(OBJ_NAME) *.o $(LIBRARY_PATHS) $(LINKER_FLAGS)
