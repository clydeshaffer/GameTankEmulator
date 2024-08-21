#include "joystick_adapter.h"
#include "emulator_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include "joystick_config.h"
#include "input_recorder.h"

JoystickAdapter::JoystickAdapter() {
	replay_ptr = NULL;
	load_joystick_config(this->bindings);

	if(!EmulatorConfig::noJoystick) {
		SDL_InitSubSystem(SDL_INIT_JOYSTICK);
		if(SDL_NumJoysticks() > 0) {
			printf("Joystick found\n");
			gGameController = SDL_JoystickOpen(0);
		} else {
			printf("Joystick NOT found\n");
		}
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
			outbyte |= 0x80;
		}
		if(stateful) {
			pad1State = false;
			pad2State = !pad2State;
		}
	} else {
		
		if(pad1State) {
			outbyte = (uint8_t) ((pad1Mask | held1Mask) >> 8);
			if(replay_ptr) {
				pad1Mask = convert_tas_frame_to_gamepad_mask(replay_ptr[replay_index++]);
				pad2Mask = convert_tas_frame_to_gamepad_mask(replay_ptr[replay_index++]);
				replay_index += 2;
				if(replay_index == replay_total_bytes) {
					replay_ptr = NULL;
				}
			}
		} else {
			outbyte = (uint8_t) (pad1Mask | held1Mask);
			outbyte |= 0x80;
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
	GameTankButtons::UP,
	GameTankButtons::DOWN,
	GameTankButtons::LEFT,
	GameTankButtons::RIGHT,
	GameTankButtons::A,
	GameTankButtons::B,
	GameTankButtons::C,
	GameTankButtons::START
};

void JoystickAdapter::SaveBindings() {
	save_joystick_config(this->bindings);
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
	if(replay_ptr) {
		return;
	}
	uint16_t buttonMask = 0;
	GameTankButtons::ButtonId buttonId = GameTankButtons::NO_BUTTON;
	for (InputBinding binding : this->bindings) {
		if((binding.type == BindingTypes::KEYBOARD) && (e->type == SDL_KEYDOWN || e->type == SDL_KEYUP)) {
			if(binding.host_input.key == e->key.keysym.sym) {
				buttonId = binding.button;
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
		} else if (binding.type == BindingTypes::JOYSTICK_HAT) {
			if(e->type == SDL_JOYHATMOTION) {
				//clockwise from the top, cardinal directions go 1, 2, 4, 8
				buttonMask = 0;
				if(e->jhat.value & 1) {
					buttonMask |= GameTankButtons::UP;
				}
				if(e->jhat.value & 2) {
					buttonMask |= GameTankButtons::RIGHT;
				}
				if(e->jhat.value & 4) {
					buttonMask |= GameTankButtons::DOWN;
				}
				if(e->jhat.value & 8) {
					buttonMask |= GameTankButtons::LEFT;
				}

				if(binding.button < BUTTON_COUNT) {
					pad1Mask &= ~GameTankButtons::ALLDIRS;
					pad1Mask |= buttonMask;
				} else {
					pad2Mask &= ~GameTankButtons::ALLDIRS;
					pad2Mask |= buttonMask;
				}
			}
		} else if (binding.type == BindingTypes::JOYSTICK_BUTTON) {
			if(e->type == SDL_JOYBUTTONDOWN || e->type == SDL_JOYBUTTONUP) {
				if(binding.host_input.joy_button == e->jbutton.button) {
					buttonId = binding.button;
					if(e->type == SDL_JOYBUTTONDOWN) {
						++button_press_counts[buttonId];
					} else if(e->type == SDL_JOYBUTTONUP) {
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
			}
		} else if (binding.type == BindingTypes::JOYSTICK_AXIS) {
			if(e->type == SDL_JOYAXISMOTION) {
				if(e->jaxis.axis == binding.host_input.axis.axis) {
					uint16_t clearMask = 0;
					buttonMask = 0;
					if(binding.host_input.axis.negative) {
						if(e->jaxis.value < -16384) {
							buttonMask = button_masks[binding.button % BUTTON_COUNT];
						} else {
							clearMask = button_masks[binding.button % BUTTON_COUNT];
						}
					} else {
						if(e->jaxis.value > 16384) {
							buttonMask = button_masks[binding.button % BUTTON_COUNT];
						} else {
							clearMask = button_masks[binding.button % BUTTON_COUNT];
						}
					}
					if(binding.button < BUTTON_COUNT) {
						pad1Mask |= buttonMask;
						pad1Mask &= ~clearMask;
					} else {
						pad2Mask |= buttonMask;
						pad2Mask &= ~clearMask;
					}
				}
				printf("Joystick axis %x %x\n", e->jaxis.axis, e->jaxis.value);
			}	
		}
	}
}

void JoystickAdapter::SetHeldButtons(uint16_t heldMask) {
	held1Mask = heldMask;
}

uint16_t JoystickAdapter::GetHeldButtons(uint8_t port) {
	if(port & 1) return pad2Mask;
	return pad1Mask;
}

void JoystickAdapter::StartReplay(char* replay_data, uint32_t replay_length) {
	if(replay_length < 4) return;
	replay_ptr = replay_data;
	replay_total_bytes = replay_length;
	replay_index = 0;
	pad1Mask = convert_tas_frame_to_gamepad_mask(replay_ptr[replay_index++]);
	pad2Mask = convert_tas_frame_to_gamepad_mask(replay_ptr[replay_index++]);
	replay_index += 2;
	if(replay_index == replay_length) {
		replay_ptr = NULL;
	}
}