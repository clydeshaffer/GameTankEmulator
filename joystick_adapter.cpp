#import "joystick_adapter.h"

JoystickAdapter::JoystickAdapter() {

}

uint8_t JoystickAdapter::read(uint8_t portNum) {
	if(portNum % 2) {
		pad1State = false;
		pad2State = !pad2State;
		return 0xFF;
	} else {
		pad2State = false;
		uint8_t outbyte = 0xFF;
		if(pad1State) {
			outbyte = (uint8_t) (pad1Mask >> 8);
		} else {
			outbyte = (uint8_t) pad1Mask;
		}
		pad1State = !pad1State;
		return ~outbyte;
	}
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
	switch(e->key.keysym.sym) {
		case SDLK_UP:
			buttonMask = 0b0000100000001000;
			break;
		case SDLK_DOWN:
			buttonMask = 0b0000010000000100;
			break;
		case SDLK_LEFT:
			buttonMask = 0b0000001000000000;
			break;
		case SDLK_RIGHT:
			buttonMask = 0b0000000100000000;
			break;
		case SDLK_z:
			buttonMask = 0b0000000000010000;
			break;
		case SDLK_x:
			buttonMask = 0b0001000000000000;
			break;
		case SDLK_c:
			buttonMask = 0b0010000000000000;
			break;
		case SDLK_RETURN:
			buttonMask = 0b0000000000100000;
			break;
		default:
			break;
	}
	if(e->type == SDL_KEYDOWN) {
		pad1Mask |= buttonMask;
	} else {
		pad1Mask &= ~buttonMask;
	}
}