#include "joystick_adapter.h"
#include <stdio.h>
#include <stdlib.h>

JoystickAdapter::JoystickAdapter() {
	SDL_InitSubSystem(SDL_INIT_JOYSTICK);
	if(SDL_NumJoysticks() > 0) {
		printf("Joystick found\n");
		gGameController = SDL_JoystickOpen(0);
	} else {
		printf("Joystick NOT found\n");
	}
}

JoystickAdapter::~JoystickAdapter() {
	SDL_JoystickClose(gGameController);
	gGameController = NULL;
}

uint8_t JoystickAdapter::read(uint8_t portNum, bool stateful) {
	uint8_t outbyte = 0xFF;
	if(portNum % 2) {
		
		if(pad2State) {
			outbyte = (uint8_t) (pad2Mask >> 8);
		} else {
			outbyte = (uint8_t) pad2Mask;
		}
		if(stateful) {
			pad1State = false;
			pad2State = !pad2State;
		}
	} else {
		
		if(pad1State) {
			outbyte = (uint8_t) ((pad1Mask | held1Mask) >> 8);
		} else {
			outbyte = (uint8_t) (pad1Mask | held1Mask);
		}
		if(stateful) {
			pad2State = false;
			pad1State = !pad1State;
		}
	}
	return ~outbyte;
}

#define BUTTON_COUNT 8
uint8_t button_press_counts[BUTTON_COUNT*2] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
};
uint16_t button_masks[BUTTON_COUNT] = {
	GAMEPAD_MASK_UP,
	GAMEPAD_MASK_DOWN,
	GAMEPAD_MASK_LEFT,
	GAMEPAD_MASK_RIGHT,
	GAMEPAD_MASK_A,
	GAMEPAD_MASK_B,
	GAMEPAD_MASK_C,
	GAMEPAD_MASK_START
};

enum ButtonId {
	P1_UP = 0, P1_DOWN = 1, P1_LEFT = 2, P1_RIGHT = 3,
	P1_A = 4, P1_B = 5, P1_C = 6, P1_START = 7,
	P2_UP = 8, P2_DOWN = 9, P2_LEFT = 10, P2_RIGHT = 11,
	P2_A = 12, P2_B = 13, P2_C = 14, P2_START = 15, NO_BUTTON = 16
};

void JoystickAdapter::update(SDL_Event *e) {
	/*
		Up - DB9 pin 1 - bit 3
		Down - DB9 pin 2 - bit 2
		Left - DB9 pin 3 (select HIGH) - bit 1
		Right - DB9 pin 4 (select HIGH) - bit 0
		A - DB9 pin 6 (select LOW) - bit 4
		B - DB9 pin 6 (select HIGH) - bit 4
		C - DB9 pin 9 (select HIGH) - bit 5
		Start - DB9 pin 9 (select LOW) - bit 5
		select status - bit 7
	*/
	uint16_t buttonMask = 0;
	ButtonId buttonId = NO_BUTTON;
	if(e->type == SDL_KEYDOWN || e->type == SDL_KEYUP) {
		switch(e->key.keysym.sym) {
			case SDLK_UP:
				buttonId = P1_UP;
				break;
			case SDLK_DOWN:
				buttonId = P1_DOWN;
				break;
			case SDLK_LEFT:
				buttonId = P1_LEFT;
				break;
			case SDLK_RIGHT:
				buttonId = P1_RIGHT;
				break;
			case SDLK_z:
			case SDLK_b:
			case SDLK_j:
				buttonId = P1_A;
				break;
			case SDLK_x:
			case SDLK_n:
			case SDLK_k:
				buttonId = P1_B;
				break;
			case SDLK_c:
			case SDLK_m:
			case SDLK_l:
				buttonId = P1_C;
				break;
			case SDLK_RETURN:
				buttonId = P1_START;
				break;
			case SDLK_t:
				buttonId = P2_UP;
				break;
			case SDLK_g:
				buttonId = P2_DOWN;
				break;
			case SDLK_f:
				buttonId = P2_LEFT;
				break;
			case SDLK_h:
				buttonId = P2_RIGHT;
				break;
			case SDLK_LSHIFT:
			case SDLK_TAB:
				buttonId = P2_A;
				break;
			case SDLK_a:
			case SDLK_q:
				buttonId = P2_B;
				break;
			case SDLK_s:
			case SDLK_w:
				buttonId = P2_C;
				break;
			case SDLK_1:
				buttonId = P2_START;
				break;
			default:
				buttonId = NO_BUTTON;
				break;
		}
		if(buttonId != NO_BUTTON) {
			if(e->type == SDL_KEYDOWN) {
				++button_press_counts[buttonId];
			} else if(e->type == SDL_KEYUP) {
				--button_press_counts[buttonId];
			}

			if(button_press_counts[buttonId] > 0) {
				if(buttonId < BUTTON_COUNT) {
					pad1Mask |= button_masks[buttonId];
				} else {
					pad2Mask |= button_masks[buttonId - BUTTON_COUNT];
				}
			} else {
				if(buttonId < BUTTON_COUNT) {
					pad1Mask &= ~button_masks[buttonId];
				} else {
					pad2Mask &= ~button_masks[buttonId - BUTTON_COUNT];
				}
			}
		}
	} else {
		if(e->type == SDL_JOYHATMOTION) {
			//clockwise from the top, cardinal directions go 1, 2, 4, 8
			pad2Mask &= ~GAMEPAD_MASK_ALLDIRS;
			if(e->jhat.value & 1) {
				pad2Mask |= GAMEPAD_MASK_UP;
			}
			if(e->jhat.value & 2) {
				pad2Mask |= GAMEPAD_MASK_RIGHT;
			}
			if(e->jhat.value & 4) {
				pad2Mask |= GAMEPAD_MASK_DOWN;
			}
			if(e->jhat.value & 8) {
				pad2Mask |= GAMEPAD_MASK_LEFT;
			}
		} else if(e->type == SDL_JOYAXISMOTION) {
			printf("Joystick axis %x %x\n", e->jaxis.axis, e->jaxis.value);
		} else if(e->type == SDL_JOYBUTTONDOWN || e->type == SDL_JOYBUTTONUP) {
			switch(e->jbutton.button) {
				case 0:
					buttonMask = GAMEPAD_MASK_A;
					break;
				case 1:
					buttonMask = GAMEPAD_MASK_B;
					break;
				case 2:
					buttonMask = GAMEPAD_MASK_C;
					break;
				case 6:
					buttonMask = GAMEPAD_MASK_START;
					break;
			}
			if(e->jbutton.state) {
				pad2Mask |= buttonMask;
			} else {
				pad2Mask &= ~buttonMask;
			}
		}
	}
}

void JoystickAdapter::SetHeldButtons(uint16_t heldMask) {
	held1Mask = heldMask;
}