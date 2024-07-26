#include <cstring>
#include <cstdio>
#include "emulator_config.h"

bool EmulatorConfig::noSound = false;
bool EmulatorConfig::noJoystick = false;
bool EmulatorConfig::noSave = false;
Uint32 EmulatorConfig::defaultRendererFlags = SDL_RENDERER_ACCELERATED;
char *EmulatorConfig::xorFile = NULL;

void EmulatorConfig::parseArg(const char* arg) {
    if(strcmp(arg, "--nosound") == 0) {
        noSound = true;
        return;
    }

    if(strcmp(arg, "--softrender") == 0) {
        defaultRendererFlags = SDL_RENDERER_SOFTWARE;
        return;
    }

    if(strcmp(arg, "--nojoystick") == 0) {
        noJoystick = true;
        return;
    }

    const char *xorFilePrefix = "--xorFile=";
    if(strncmp(arg, xorFilePrefix, strlen(xorFilePrefix)) == 0) {
      // TODO memory allocated here, need to clean up
      xorFile = strdup(arg + strlen(xorFilePrefix));
      return;
    }


    printf("Unrecognized option %s\n", arg);
}
