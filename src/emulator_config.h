#pragma once
#include "SDL_inc.h"

class EmulatorConfig {
public:
    static bool noSound;
    static bool noJoystick;
    static void parseArg(const char* arg);
    static Uint32 defaultRendererFlags;
    static bool noSave;
    static char *xorFile;
};
