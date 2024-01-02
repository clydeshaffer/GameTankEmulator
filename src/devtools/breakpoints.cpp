#include "breakpoints.h"
int Breakpoints::breakCooldown = 0;
bool Breakpoints::enabled = false;
set<uint16_t> Breakpoints::addresses;
set<uint16_t> Breakpoints::manual_breakpoints;