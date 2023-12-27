#include "SDL_inc.h"
class EmulatorConfig {
public:
    static bool noSound;
    static void parseArg(const char* arg);
    static Uint32 defaultRendererFlags;
};