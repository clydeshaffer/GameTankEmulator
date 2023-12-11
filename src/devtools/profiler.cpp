#include "profiler.h"
#include <cmath>

void Profiler::LogTime(uint8_t index) {
	uint64_t delta = timekeeper.totalCyclesCount - profilingTimeStamps[index];
	if(delta > timekeeper.system_clock) {
		//printf("Timer %d took longer than one second: %llu cycles.\n", index, delta);
	}
	profilingTimes[index] += delta;
	profilingCounts[index]++;
    profilingHistory[index][history_num] = std::log(profilingTimes[index]) / std::log(timekeeper.cycles_per_vsync);
}

void Profiler::ResetTimers() {
    ++history_num;
    history_num %= PROFILER_HISTORY;
    for(int i = 0; i < PROFILER_ENTRIES; ++i) {
        //profilingHistory[i][history_num] = 0;
        profilingTimes[i] = 0;
		profilingCounts[i] = 0;
    }
}