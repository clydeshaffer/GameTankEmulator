#include "profiler.h"
#include <cmath>
#include <fstream>
#include <stack>

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

void Profiler::LogJSR(uint16_t address, uint8_t bank, uint16_t destination) {
    if(deepProfileIsRecording) {
        timeline.emplace_back(ProfileEventType::CALL, address, destination, bank, timekeeper.totalCyclesCount);
    }
}

void Profiler::LogRTS(uint16_t address, uint8_t bank) {
    if(deepProfileIsRecording) {
        timeline.emplace_back(ProfileEventType::RETURN, address, 0, bank, timekeeper.totalCyclesCount);
    }
}

void Profiler::DeepProfileStart() {
    deepProfileIsRecording = true;
    deepProfileStartCycleCount = timekeeper.totalCyclesCount;
    lastDeepProfileRoot = nullptr;
    deepProfileZoomFocus = nullptr;
    timeline.clear();
    for(auto& ptr : cleanupList) {
        free(ptr);
    }
    cleanupList.clear();
}

std::string Profiler::profileEventTypeToString(ProfileEventType type) {
    switch (type) {
        case ProfileEventType::CALL: return "CALL";
        case ProfileEventType::RETURN: return "RETURN";
        default: return "UNKNOWN";
    }
}

void Profiler::recursiveNodePrintToFile(std::ofstream& file, DeepProfileCallNode* node, uint16_t depth) {
    file << std::string(depth, '\t') << "{\n";
    file << std::string(depth+1, '\t') << "name: " << node->name << ",\n";
    file << std::string(depth+1, '\t') << "duration: " << node->duration << ",\n";
    file << std::string(depth+1, '\t') << "children: [";
    for(auto &childNode : node->children) {
        file << "\n";
        recursiveNodePrintToFile(file, childNode, depth+2);
        file << ",";
    }
    file << "]\n";
    file << std::string(depth, '\t') << "}";
}

void Profiler::exportTimelineToJSON(const std::vector<ProfileEvent>& timeline, const std::string& filename, MemoryMap* memory_map, SourceMap* source_map) {
    std::stack<DeepProfileCallNode*> callstack;
    
    DeepProfileCallNode* rootNode = new DeepProfileCallNode();
    rootNode->name = "root";
    rootNode->duration = timekeeper.totalCyclesCount - deepProfileStartCycleCount;
    rootNode->offset = 0;
    cleanupList.push_back(rootNode);
    for (const auto& event : timeline) {
        if(event.type == ProfileEventType::CALL) {
            callstack.push(new DeepProfileCallNode());
            cleanupList.push_back(callstack.top());
            callstack.top()->duration = event.cycle_number;
            callstack.top()->offset = event.cycle_number - deepProfileStartCycleCount;
            if(memory_map != nullptr) {
                Symbol sym;
                if(memory_map->FindAddress(event.destination, &sym)) {
                    callstack.top()->name = sym.name;
                }
            }
            if(source_map != nullptr) {
                SourceMapSearchResult result = source_map->Search(event.origin, event.bank);
                if(result.found) {
                    callstack.top()->line = *result.line;
                }
            }
        } else {
            DeepProfileCallNode* parent;
            if(callstack.size() > 0) {
                DeepProfileCallNode* returningNode = callstack.top();
                callstack.pop();
                if(callstack.size() > 0) {
                    parent = callstack.top();
                } else {
                    parent = rootNode;
                }
                returningNode->duration = event.cycle_number - returningNode->duration;
                parent->children.push_back(returningNode);
            }
        }
    }

    lastDeepProfileRoot = rootNode;

    /*std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }
    recursiveNodePrintToFile(file, rootNode, 0);

    file.close();*/
}

void Profiler::DeepProfileStop(MemoryMap* memory_map, SourceMap* source_map) {
    deepProfileIsRecording = false;
    exportTimelineToJSON(timeline, "./deep_profile.json", memory_map, source_map);
}