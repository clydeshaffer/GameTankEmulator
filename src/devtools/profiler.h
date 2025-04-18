#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <fstream>
#include "../timekeeper.h"
#include "memory_map.h"
#include "source_map.h"

#define PROFILER_ENTRIES 64
#define PROFILER_HISTORY 256

class Profiler {
    enum ProfileEventType {
        CALL,
        RETURN
    };
    struct ProfileEvent {
        ProfileEventType type;
        uint16_t origin;
        uint16_t destination;
        uint8_t bank;
        uint64_t cycle_number;

        ProfileEvent(ProfileEventType type, uint16_t origin, uint16_t destination, uint8_t bank, uint64_t cycle_number)
            : type(type), origin(origin), destination(destination), bank(bank), cycle_number(cycle_number)
        {}
    };

public: 
    class DeepProfileCallNode {
        public:
        std::string name;
        SourceMapLine line;
        uint32_t duration;
        uint32_t offset;
        std::vector<DeepProfileCallNode*> children;
    };

private:
    Timekeeper& timekeeper;
    bool deepProfileIsRecording;
    uint64_t deepProfileStartCycleCount;

    std::string profileEventTypeToString(ProfileEventType type);
    void recursiveNodePrintToFile(std::ofstream& file, DeepProfileCallNode* node, uint16_t depth);
    void exportTimelineToJSON(const std::vector<ProfileEvent>& timeline, const std::string& filename, MemoryMap* memory_map, SourceMap* source_map);
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
    std::vector<ProfileEvent> timeline;
    void LogJSR(uint16_t address, uint8_t bank, uint16_t dest);
    void LogRTS(uint16_t address, uint8_t bank);
    void DeepProfileStart();
    void DeepProfileStop(MemoryMap* memory_map, SourceMap* source_map);
    DeepProfileCallNode* lastDeepProfileRoot;
    DeepProfileCallNode* deepProfileZoomFocus;
    std::vector<DeepProfileCallNode*> cleanupList;
};