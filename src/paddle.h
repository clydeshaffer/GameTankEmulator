#ifndef PADDLE_H
#define PADDLE_H

#include "SDL_inc.h"
#include <stdbool.h>
#include <stdint.h>

extern bool joystick_paddle_enabled;
extern bool paddleDetected;
extern SDL_JoystickID paddle_instanceID;
extern int32_t currentPaddleRawValue;

void PaddleInit(void);

#endif // PADDLE_H