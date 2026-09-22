#include "paddle.h"
#include <stdio.h>

bool joystick_paddle_enabled = false;
bool paddleDetected = false;

SDL_JoystickID paddle_instanceID = -1;
int32_t currentPaddleRawValue = 0;
static SDL_Joystick* active_paddle_handle = NULL;

void PaddleInit(void) {
    int num_joysticks = SDL_NumJoysticks();
    SDL_JoystickID bakInstanceID = -1;

    if (active_paddle_handle != NULL) {
        bakInstanceID = paddle_instanceID;
    }

    paddleDetected = false; 
    paddle_instanceID = -1;
    
    SDL_Joystick* chosen_joystick = NULL;
    const char* chosen_name = NULL;

    for (int i = 0; i < num_joysticks; i++) {
        const char* name = SDL_JoystickNameForIndex(i);
        SDL_Joystick* j = SDL_JoystickOpen(i); 
        if (!j) continue;

        if (chosen_joystick == NULL) {
            chosen_joystick = j;
            chosen_name = name;
            break; 
        }

        if (j != chosen_joystick) {
            SDL_JoystickClose(j);
        }
    }

    if (chosen_joystick != NULL) {
        if (active_paddle_handle != NULL && active_paddle_handle != chosen_joystick) {
            SDL_JoystickClose(active_paddle_handle);
        }

        active_paddle_handle = chosen_joystick;
        paddle_instanceID = SDL_JoystickInstanceID(active_paddle_handle);
        paddleDetected = true;

        if (paddle_instanceID != bakInstanceID) {
            printf("Hardware Verified: %s (Instance ID: %d)\n", 
                   chosen_name ? chosen_name : "Unknown", paddle_instanceID);
        }
    } else {
        if (active_paddle_handle != NULL) {
            SDL_JoystickClose(active_paddle_handle);
            active_paddle_handle = NULL;
        }
    }
}
