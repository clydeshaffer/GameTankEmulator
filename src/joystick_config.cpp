#include <vector>
#include "joystick_adapter.h"
#include "toml/toml.hpp"
#include <filesystem>
#include <iostream>
#include <fstream>
#include "joystick_config.h"
std::string get_joystick_config_path() {
	std::filesystem::path prefPath(SDL_GetPrefPath("GameTank", "Emulator"));
	return (prefPath / "input_cfg.toml").string();
}

void save_joystick_config(std::vector<InputBinding> &bindings) {
	std::string path = get_joystick_config_path();
	toml::table config = toml::table();
	toml::array bindArray = toml::array();

	for(auto& bindEntry : bindings) {
		toml::table bindTomlEntry = toml::table();
		bindTomlEntry.emplace("type", bindEntry.type);
		bindTomlEntry.emplace("gtButton", bindEntry.button);
		switch (bindEntry.type)
		{
			case BindingTypes::KEYBOARD:
			bindTomlEntry.emplace("key", bindEntry.host_input.key);
			break;
			case BindingTypes::JOYSTICK_AXIS:
			bindTomlEntry.emplace("joyAxis", bindEntry.host_input.axis.axis);
			bindTomlEntry.emplace("joyAxisInvert", bindEntry.host_input.axis.negative);
			break;
			case BindingTypes::JOYSTICK_BUTTON:
			bindTomlEntry.emplace("joyButton", bindEntry.host_input.joy_button);
			break;
			case BindingTypes::JOYSTICK_HAT:
			bindTomlEntry.emplace("joyHat", bindEntry.host_input.joy_button);
			break;
		}
		bindArray.emplace_back(bindTomlEntry);
	}
	config.emplace("input_bindings", bindArray);
	std::fstream outFile;
    outFile.open(path, std::ios_base::out | std::ios_base::trunc);
    outFile << config << "\n\n";
    outFile.close();
}

void load_joystick_config(std::vector<InputBinding> &bindings) {
	std::string path = get_joystick_config_path();
    if(std::filesystem::exists(path)) {
        toml::table config = toml::parse_file(path);
        auto bindArray = *config.get_as<toml::array>("input_bindings");
        for(auto&& bindEntry : bindArray) {
                InputBinding bind;
                toml::table *tbl = bindEntry.as_table();
				bind.type = (BindingTypes::BindingType)((int64_t) (*tbl->get_as<int64_t>("type")));
				bind.button = (GameTankButtons::ButtonId)((int64_t) (*tbl->get_as<int64_t>("gtButton")));
				switch (bind.type)
				{
					case BindingTypes::KEYBOARD:
					bind.host_input.key = (int64_t) (*tbl->get_as<int64_t>("key"));
					break;
					case BindingTypes::JOYSTICK_AXIS:
					bind.host_input.axis.axis = (int64_t) (*tbl->get_as<int64_t>("joyAxis"));
					bind.host_input.axis.negative = (bool) (*tbl->get_as<bool>("joyAxisInvert"));
					break;
					case BindingTypes::JOYSTICK_BUTTON:
					bind.host_input.joy_button = (int64_t) (*tbl->get_as<int64_t>("joyButton"));
					break;
					case BindingTypes::JOYSTICK_HAT:
					bind.host_input.joy_button = (int64_t) (*tbl->get_as<int64_t>("joyHat"));
					break;
				}
                bindings.emplace_back(bind);
        }
    } else {
		load_joystick_defaults(bindings);
	}
}

void load_joystick_defaults(std::vector<InputBinding> &bindings) {
    InputBinding b;
	b.type = BindingTypes::KEYBOARD;
	b.host_input.key = SDLK_UP;
	b.button = GameTankButtons::P1_UP;
	bindings.emplace_back(b);

	b.host_input.key = SDLK_DOWN;
	b.button = GameTankButtons::P1_DOWN;
	bindings.emplace_back(b);

	b.host_input.key = SDLK_LEFT;
	b.button = GameTankButtons::P1_LEFT;
	bindings.emplace_back(b);

	b.host_input.key = SDLK_RIGHT;
	b.button = GameTankButtons::P1_RIGHT;
	bindings.emplace_back(b);

	b.host_input.key = SDLK_z;
	b.button = GameTankButtons::P1_A;
	bindings.emplace_back(b);

	b.host_input.key = SDLK_j;
	b.button = GameTankButtons::P1_A;
	bindings.emplace_back(b);

	b.host_input.key = SDLK_x;
	b.button = GameTankButtons::P1_B;
	bindings.emplace_back(b);

	b.host_input.key = SDLK_k;
	b.button = GameTankButtons::P1_B;
	bindings.emplace_back(b);

	b.host_input.key = SDLK_c;
	b.button = GameTankButtons::P1_C;
	bindings.emplace_back(b);

	b.host_input.key = SDLK_l;
	b.button = GameTankButtons::P1_C;
	bindings.emplace_back(b);

	b.host_input.key = SDLK_RETURN;
	b.button = GameTankButtons::P1_START;
	bindings.emplace_back(b);

	b.type = BindingTypes::KEYBOARD;
	b.host_input.key = SDLK_t;
	b.button = GameTankButtons::P2_UP;
	bindings.emplace_back(b);

	b.host_input.key = SDLK_g;
	b.button = GameTankButtons::P2_DOWN;
	bindings.emplace_back(b);

	b.host_input.key = SDLK_f;
	b.button = GameTankButtons::P2_LEFT;
	bindings.emplace_back(b);

	b.host_input.key = SDLK_h;
	b.button = GameTankButtons::P2_RIGHT;
	bindings.emplace_back(b);

	b.host_input.key = SDLK_LSHIFT;
	b.button = GameTankButtons::P2_A;
	bindings.emplace_back(b);

	b.host_input.key = SDLK_TAB;
	b.button = GameTankButtons::P2_A;
	bindings.emplace_back(b);

	b.host_input.key = SDLK_a;
	b.button = GameTankButtons::P2_B;
	bindings.emplace_back(b);

	b.host_input.key = SDLK_q;
	b.button = GameTankButtons::P2_B;
	bindings.emplace_back(b);

	b.host_input.key = SDLK_s;
	b.button = GameTankButtons::P2_C;
	bindings.emplace_back(b);

	b.host_input.key = SDLK_w;
	b.button = GameTankButtons::P2_C;
	bindings.emplace_back(b);

	b.host_input.key = SDLK_1;
	b.button = GameTankButtons::P2_START;
	bindings.emplace_back(b);

	b.type = BindingTypes::JOYSTICK_AXIS;
	b.host_input.axis.axis = 0;
	b.host_input.axis.negative = false;
	b.button = GameTankButtons::P1_RIGHT;
	bindings.emplace_back(b);

	b.type = BindingTypes::JOYSTICK_AXIS;
	b.host_input.axis.axis = 0;
	b.host_input.axis.negative = true;
	b.button = GameTankButtons::P1_LEFT;
	bindings.emplace_back(b);

	b.type = BindingTypes::JOYSTICK_AXIS;
	b.host_input.axis.axis = 1;
	b.host_input.axis.negative = false;
	b.button = GameTankButtons::P1_UP;
	bindings.emplace_back(b);

	b.type = BindingTypes::JOYSTICK_AXIS;
	b.host_input.axis.axis = 1;
	b.host_input.axis.negative = true;
	b.button = GameTankButtons::P1_DOWN;
	bindings.emplace_back(b);

	b.type = BindingTypes::JOYSTICK_HAT;
	bindings.emplace_back(b);

	b.type = BindingTypes::JOYSTICK_BUTTON;
	b.host_input.joy_button = 0;
	b.button = GameTankButtons::P1_A;
	bindings.emplace_back(b);

	b.type = BindingTypes::JOYSTICK_BUTTON;
	b.host_input.joy_button = 1;
	b.button = GameTankButtons::P1_B;
	bindings.emplace_back(b);

	b.type = BindingTypes::JOYSTICK_BUTTON;
	b.host_input.joy_button = 2;
	b.button = GameTankButtons::P1_C;
	bindings.emplace_back(b);

	b.type = BindingTypes::JOYSTICK_BUTTON;
	b.host_input.joy_button = 3;
	b.button = GameTankButtons::P1_C;
	bindings.emplace_back(b);

	b.type = BindingTypes::JOYSTICK_BUTTON;
	b.host_input.joy_button = 7;
	b.button = GameTankButtons::P1_START;
	bindings.emplace_back(b);
};
