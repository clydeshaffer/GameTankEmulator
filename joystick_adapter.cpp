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

uint8_t JoystickAdapter::read(uint8_t portNum) {
	uint8_t outbyte = 0xFF;
	if(portNum % 2) {
		pad1State = false;
		if(pad2State) {
			outbyte = (uint8_t) (pad2Mask >> 8);
		} else {
			outbyte = (uint8_t) pad2Mask;
		}
		pad2State = !pad2State;
	} else {
		pad2State = false;
		if(pad1State) {
			outbyte = (uint8_t) ((pad1Mask | held1Mask) >> 8);
		} else {
			outbyte = (uint8_t) (pad1Mask | held1Mask);
		}
		pad1State = !pad1State;
	}
	return ~outbyte;
}

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
	if(e->type == SDL_KEYDOWN || e->type == SDL_KEYUP) {
		switch(e->key.keysym.sym) {
			case SDLK_UP:
				buttonMask = GAMEPAD_MASK_UP;
				break;
			case SDLK_DOWN:
				buttonMask = GAMEPAD_MASK_DOWN;
				break;
			case SDLK_LEFT:
				buttonMask = GAMEPAD_MASK_LEFT;
				break;
			case SDLK_RIGHT:
				buttonMask = GAMEPAD_MASK_RIGHT;
				break;
			case SDLK_z:
				buttonMask = GAMEPAD_MASK_A;
				break;
			case SDLK_x:
				buttonMask = GAMEPAD_MASK_B;
				break;
			case SDLK_c:
				buttonMask = GAMEPAD_MASK_C;
				break;
			case SDLK_RETURN:
				buttonMask = GAMEPAD_MASK_START;
				break;
			default:
				break;
		}
		if(e->type == SDL_KEYDOWN) {
			pad1Mask |= buttonMask;
		} else {
			pad1Mask &= ~buttonMask;
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