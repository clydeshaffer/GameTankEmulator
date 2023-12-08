#pragma once
#include <cstdint>

#define INITIAL_TIME_SCALING 1000
#define INITIAL_SCALING_INCREMENT 100

class Timekeeper {
public:
    uint64_t system_clock = 315000000/88;
    uint64_t actual_cycles = 0;
    uint64_t cycles_since_vsync = 0;
    uint64_t cycles_per_vsync = system_clock / 60;
    double time_scaling = INITIAL_TIME_SCALING;
    double scaling_increment = INITIAL_SCALING_INCREMENT;
    uint32_t lastTicks = 0, currentTicks;
    uint8_t frameCount = 0;
    bool prev_overlong = false;
    uint64_t totalCyclesCount = 0;
};