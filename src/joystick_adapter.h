#pragma once
#include "SDL_inc.h"
#include <vector>
namespace GameTankButtons {
	enum GamepadButtonMask {
		UP = 0b0000100000001000,
		DOWN = 0b0000010000000100,
		LEFT = 0b0000001000000000,
		RIGHT = 0b0000000100000000,
		A = 0b0000000000010000,
		B = 0b0001000000000000,
		C = 0b0010000000000000,
		START = 0b0000000000100000,
		ALLDIRS = 0b0000111100001100
	};

	enum ButtonId {
		P1_UP = 0, P1_DOWN = 1, P1_LEFT = 2, P1_RIGHT = 3,
		P1_A = 4, P1_B = 5, P1_C = 6, P1_START = 7,
		P2_UP = 8, P2_DOWN = 9, P2_LEFT = 10, P2_RIGHT = 11,
		P2_A = 12, P2_B = 13, P2_C = 14, P2_START = 15, NO_BUTTON = 16
	};
};

using namespace std;

namespace BindingTypes {
	enum BindingType {
		KEYBOARD,
		JOYSTICK_AXIS,
		JOYSTICK_BUTTON,
		JOYSTICK_HAT,
	};
};

typedef struct JoyAxisInput {
	bool negative;
	uint8_t axis;
} JoyInput;

typedef struct InputBinding {
	BindingTypes::BindingType type;
	union host_input {
		SDL_Keycode key;
		JoyAxisInput axis;
		uint8_t joy_button;
	} host_input;
    GameTankButtons::ButtonId button;
} InputBinding;

class JoystickAdapter {
private:
	bool pad1State = false;
	bool pad2State = false;
	uint16_t pad1Mask = 0;
	uint16_t pad2Mask = 0;
	uint16_t held1Mask = 0;
	SDL_Joystick* gGameController = NULL;
public:
	JoystickAdapter();
	~JoystickAdapter();
	uint8_t read(uint8_t portNum, bool stateful);
	void update(SDL_Event *e);
	void SetHeldButtons(uint16_t heldMask);
	std::vector<InputBinding> bindings;
	void SaveBindings();
	void Reset();
};
