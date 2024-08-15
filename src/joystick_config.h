#include "joystick_adapter.h"
#include <string>

void load_joystick_defaults(std::vector<InputBinding> &bindings);
void save_joystick_config(std::vector<InputBinding> &bindings);
void load_joystick_config(std::vector<InputBinding> &bindings);
std::string get_joystick_config_path();