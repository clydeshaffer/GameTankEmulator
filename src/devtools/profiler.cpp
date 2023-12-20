#include "profiler.h"
#include <cmath>

void Profiler::LogTime(uint8_t index) {
	uint64_t delta = timekeeper.totalCyclesCount - profilingTimeStamps[index];
	if(delta > timekeeper.system_clock) {
		//printf("Timer %d took longer than one second: %llu cycles.\n", index, delta);
	}
	profilingTimes[index] += delta;
	profilingCounts[index]++;
    profilingHistory[index][history_num] = (((float)profilingTimes[index]) / ((float)timekeeper.cycles_per_vsync));
}

void Profiler::ResetTimers() {
    blitter_history[history_num] = (((float)last_blitter_activity) / ((float)timekeeper.cycles_per_vsync));
    ++history_num;
    history_num %= PROFILER_HISTORY;
    for(int i = 0; i < PROFILER_ENTRIES; ++i) {
        profilingLastSample[i] = profilingTimes[i];
        profilingLastSampleCount[i] = profilingCounts[i];
        profilingTimes[i] = 0;
		profilingCounts[i] = 0;
        profilingHistory[i][history_num] = 0;
    }
}