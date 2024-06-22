#include <vector>
#include "joystick_adapter.h"

void load_joystick_defaults(std::vector<InputBinding> &bindings) {
    InputBinding b;
	b.type = BindingTypes::KEYBOARD;
	b.host_input.key = SDLK_UP;
	b.button = GameTankButtons::P1_UP;
	bindings.push_back(b);

	b.host_input.key = SDLK_DOWN;
	b.button = GameTankButtons::P1_DOWN;
	bindings.push_back(b);

	b.host_input.key = SDLK_LEFT;
	b.button = GameTankButtons::P1_LEFT;
	bindings.push_back(b);

	b.host_input.key = SDLK_RIGHT;
	b.button = GameTankButtons::P1_RIGHT;
	bindings.push_back(b);

	b.host_input.key = SDLK_z;
	b.button = GameTankButtons::P1_A;
	bindings.push_back(b);

	b.host_input.key = SDLK_b;
	b.button = GameTankButtons::P1_A;
	bindings.push_back(b);

	b.host_input.key = SDLK_j;
	b.button = GameTankButtons::P1_A;
	bindings.push_back(b);

	b.host_input.key = SDLK_x;
	b.button = GameTankButtons::P1_B;
	bindings.push_back(b);

	b.host_input.key = SDLK_n;
	b.button = GameTankButtons::P1_B;
	bindings.push_back(b);

	b.host_input.key = SDLK_k;
	b.button = GameTankButtons::P1_B;
	bindings.push_back(b);

	b.host_input.key = SDLK_c;
	b.button = GameTankButtons::P1_C;
	bindings.push_back(b);

	b.host_input.key = SDLK_m;
	b.button = GameTankButtons::P1_C;
	bindings.push_back(b);

	b.host_input.key = SDLK_l;
	b.button = GameTankButtons::P1_C;
	bindings.push_back(b);

	b.host_input.key = SDLK_RETURN;
	b.button = GameTankButtons::P1_START;
	bindings.push_back(b);

	b.type = BindingTypes::KEYBOARD;
	b.host_input.key = SDLK_t;
	b.button = GameTankButtons::P2_UP;
	bindings.push_back(b);

	b.host_input.key = SDLK_g;
	b.button = GameTankButtons::P2_DOWN;
	bindings.push_back(b);

	b.host_input.key = SDLK_f;
	b.button = GameTankButtons::P2_LEFT;
	bindings.push_back(b);

	b.host_input.key = SDLK_h;
	b.button = GameTankButtons::P2_RIGHT;
	bindings.push_back(b);

	b.host_input.key = SDLK_LSHIFT;
	b.button = GameTankButtons::P2_A;
	bindings.push_back(b);

	b.host_input.key = SDLK_TAB;
	b.button = GameTankButtons::P2_A;
	bindings.push_back(b);

	b.host_input.key = SDLK_a;
	b.button = GameTankButtons::P2_B;
	bindings.push_back(b);

	b.host_input.key = SDLK_q;
	b.button = GameTankButtons::P2_B;
	bindings.push_back(b);

	b.host_input.key = SDLK_s;
	b.button = GameTankButtons::P2_C;
	bindings.push_back(b);

	b.host_input.key = SDLK_w;
	b.button = GameTankButtons::P2_C;
	bindings.push_back(b);

	b.host_input.key = SDLK_1;
	b.button = GameTankButtons::P2_START;
	bindings.push_back(b);
};