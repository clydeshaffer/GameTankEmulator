#pragma once
#include <cstdint>
#include "../timekeeper.h"

#define PROFILER_ENTRIES 64
#define PROFILER_HISTORY 256
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
    uint32_t profilingLastSample[PROFILER_ENTRIES];
    uint32_t profilingLastSampleCount[PROFILER_ENTRIES];
    float profilingHistory[PROFILER_ENTRIES][PROFILER_HISTORY];
    float blitter_history[PROFILER_HISTORY];
    uint64_t last_blitter_activity = 0;
    int fps = 60;
    int history_num = 0;
    bool measure_by_frameflip = false;
};