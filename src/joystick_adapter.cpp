#include "joystick_adapter.h"
#include "emulator_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include "joystick_config.h"
#ifndef WASM_BUILD
#include "imgui/backends/imgui_impl_sdl2.h"
#endif

JoystickAdapter::JoystickAdapter() {

	load_joystick_config(this->bindings);

	if(!EmulatorConfig::noJoystick) {
		SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
		if(SDL_NumJoysticks() > 0) {
			printf("Joystick found\n");
			gGameController = SDL_GameControllerOpen(0);
			gGameControllerId = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(gGameController));
#ifndef WASM_BUILD
			ImGui_ImplSDL2_SetGamepadMode(ImGui_ImplSDL2_GamepadMode_Manual, &gGameController, 1);
#endif
		} else {
			printf("Joystick NOT found\n");
		}
	}
}

JoystickAdapter::~JoystickAdapter() {
	if(gGameController != NULL) {
		SDL_GameControllerClose(gGameController);
		gGameController = NULL;
	}
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

void JoystickAdapter::update(SDL_Event *e, bool managementOnly) {
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

	if((e->type == SDL_CONTROLLERDEVICEADDED) && (gGameController == NULL)) {
		gGameController = SDL_GameControllerOpen(e->cdevice.which);
		gGameControllerId = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(gGameController));
#ifndef WASM_BUILD		
		ImGui_ImplSDL2_SetGamepadMode(ImGui_ImplSDL2_GamepadMode_Manual, &gGameController, 1);
#endif
	}

	if((e->type == SDL_CONTROLLERDEVICEREMOVED) && (gGameController != NULL)) {
		if(e->cdevice.which == gGameControllerId) {
			SDL_GameControllerClose(gGameController);
			gGameController = NULL;
#ifndef WASM_BUILD
			ImGui_ImplSDL2_SetGamepadMode(ImGui_ImplSDL2_GamepadMode_Manual);
#endif
		}
	}

	uint16_t buttonMask = 0;
	GameTankButtons::ButtonId buttonId = GameTankButtons::NO_BUTTON;
	for (InputBinding binding : this->bindings) {
		if(managementOnly && (binding.type != BindingTypes::JOYSTICK_BUTTON_SYSTEM)) {
			continue;
		}
		if((binding.type == BindingTypes::KEYBOARD) && ((e->type == SDL_KEYDOWN) || (e->type == SDL_KEYUP))) {
			if(binding.host_input.key == e->key.keysym.sym) {
				buttonId = binding.button;
				if(e->type == SDL_KEYDOWN) {
					++button_press_counts[buttonId];
				} else if(e->type == SDL_KEYUP) {
					if(button_press_counts[buttonId] > 0)
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
		} /*else if (binding.type == BindingTypes::JOYSTICK_HAT) {
			if(e->type == SDL_CONTROLLERHATMOTION) {
				//clockwise from the top, cardinal directions go 1, 2, 4, 8
				buttonMask = 0;
				if(e->chat.value & 1) {
					buttonMask |= GameTankButtons::UP;
				}
				if(e->chat.value & 2) {
					buttonMask |= GameTankButtons::RIGHT;
				}
				if(e->chat.value & 4) {
					buttonMask |= GameTankButtons::DOWN;
				}
				if(e->chat.value & 8) {
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
		}*/ else if (binding.type == BindingTypes::JOYSTICK_BUTTON) {
			if((e->type == SDL_CONTROLLERBUTTONDOWN) || (e->type == SDL_CONTROLLERBUTTONUP)) {
				if((binding.host_input.joy_button) == (e->cbutton.button)) {
					buttonId = binding.button;
					if(e->type == SDL_CONTROLLERBUTTONDOWN) {
						++button_press_counts[buttonId];
					} else if(e->type == SDL_CONTROLLERBUTTONUP) {
						if(button_press_counts[buttonId] > 0)
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
			if(e->type == SDL_CONTROLLERAXISMOTION) {
				if(e->caxis.axis == binding.host_input.axis.axis) {
					uint16_t clearMask = 0;
					buttonMask = 0;
					if(binding.host_input.axis.negative) {
						if(e->caxis.value < -16384) {
							buttonMask = button_masks[binding.button % BUTTON_COUNT];
						} else {
							clearMask = button_masks[binding.button % BUTTON_COUNT];
						}
					} else {
						if(e->caxis.value > 16384) {
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
				//printf("Joystick axis %x %x\n", e->caxis.axis, e->caxis.value);
			}	
		} else if(binding.type == BindingTypes::JOYSTICK_BUTTON_SYSTEM) {
			if(e->type == SDL_CONTROLLERBUTTONDOWN) {
				if((binding.host_input.joy_button) == (e->cbutton.button)) {
					systemMenuPressed = true;
				}
			}
		}
	}
}

void JoystickAdapter::SetHeldButtons(uint16_t heldMask) {
	held1Mask = heldMask;
}

void JoystickAdapter::Reset() {
	pad1State = false;
	pad2State = false;
	pad1Mask = 0;
	pad2Mask = 0;
	held1Mask = 0;
	for(int i = 0; i < (BUTTON_COUNT*2); ++i) {
		button_press_counts[i] = 0;
	}
}

bool JoystickAdapter::CheckSystemButtonPressed() {
	bool ret = systemMenuPressed;
	systemMenuPressed = false;
	return ret;
}

void JoystickAdapter::SetPaddleBitsDirect(int val) {
    uint8_t p = ~(uint8_t)val;//invert the bits for the ADC paddle

    uint16_t paddle_state = 0;
    if (p & 0x01) paddle_state |= GameTankButtons::GamepadButtonMask::PADDLE_UP;
    if (p & 0x02) paddle_state |= GameTankButtons::GamepadButtonMask::PADDLE_DOWN;
    if (p & 0x04) paddle_state |= GameTankButtons::GamepadButtonMask::LEFT;
    if (p & 0x08) paddle_state |= GameTankButtons::GamepadButtonMask::RIGHT;
    if (p & 0x10) paddle_state |= GameTankButtons::GamepadButtonMask::PADDLE_X;
    if (p & 0x20) paddle_state |= GameTankButtons::GamepadButtonMask::PADDLE_Y;
    if (p & 0x40) paddle_state |= GameTankButtons::GamepadButtonMask::PADDLE_Z;
    if (p & 0x80) paddle_state |= GameTankButtons::GamepadButtonMask::PADDLE_MODE;

	uint16_t current = held1Mask;
	uint16_t paddle_bits_mask = 0xFF0F; // Adjust this mask to cover ONLY paddle bits
	current &= ~paddle_bits_mask; 
	current |= (paddle_state & paddle_bits_mask);
	SetHeldButtons(current);
}
void JoystickAdapter::SetPaddleAButtonDirect(bool pressedState) {
    uint16_t current = held1Mask;
    
    if (pressedState) {
        current |= GameTankButtons::A;
    } else {
        current &= ~GameTankButtons::A;
    }
    
    SetHeldButtons(current);
}

void JoystickAdapter::UpdatePaddleFromCursorPos(int player, int mouseX, int windowWidth) {
    uint8_t p = ((uint8_t)((mouseX * 255) / windowWidth));
	SetPaddleBitsDirect(p);
}

void JoystickAdapter::UpdatePaddleFromMouse(int index, int dx) {
    int newValue = currentPaddleValue[index] + dx;

    if (newValue > 255) newValue = 255;
    if (newValue < 0) newValue = 0;

    currentPaddleValue[index] = (uint8_t)newValue;
	SetPaddleBitsDirect(newValue);

}