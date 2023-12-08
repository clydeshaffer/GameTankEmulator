#pragma once
#include <cstdint>
#include "../timekeeper.h"

#define PROFILER_ENTRIES 64
#define PROFILER_HISTORY 64
class Profiler {
private:
    Timekeeper& timekeeper;
public:
    Profiler(Timekeeper& tk) : timekeeper(tk) {};
    int zeroConsec = 0;
    uint8_t bufferFlipCount = 0;
    void LogTime(uint8_t index);
    void ResetTimers();
    uint64_t profilingTimeStamps[PROFILER_ENTRIES];
    uint64_t profilingTimes[PROFILER_ENTRIES];
    uint64_t profilingCounts[PROFILER_ENTRIES];
    float profilingHistory[PROFILER_ENTRIES][PROFILER_HISTORY];
    int fps = 60;
    int history_num = 0;
};