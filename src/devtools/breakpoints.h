#pragma once
#include <set>
#include <cstdint>

using std::set;

class Breakpoints {
public:
    static int breakCooldown;
    static bool enabled;
    static set<uint16_t> addresses;
};