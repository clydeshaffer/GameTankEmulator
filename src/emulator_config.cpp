#include <cstring>
#include "emulator_config.h"

bool EmulatorConfig::noSound = false;

void EmulatorConfig::parseArg(const char* arg) {
    if(strcmp(arg, "--nosound") == 0) {
        noSound = true;
        return;
    }
}
