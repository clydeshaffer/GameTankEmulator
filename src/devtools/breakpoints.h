#pragma once
#include <cstdint>
#include <vector>
#include <string>

using std::vector;

typedef struct Breakpoint {
    uint16_t address;
    uint8_t bank;
    bool bank_set;
    std::string name;
    bool by_address;
    bool word;
    bool enabled;
    //not persisted below:
    bool linked; //flag for whether the var name has been searched and refreshed
    bool linkFailed;
} Breakpoint;

class Breakpoints {
public:
    static int breakCooldown;
    static bool enabled;
    static vector<Breakpoint> breakpoints;
    static bool checkBreakpoint(uint16_t address, uint8_t bank);
};