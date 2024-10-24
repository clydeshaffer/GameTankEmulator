#include <string>
#include <vector>
#pragma once
#include <stdint.h>

typedef struct BinFileBinding {
    uint16_t address;
    uint8_t bank;
    std::string path;
} BinFileBinding;

typedef struct MemoryWatch {
    uint16_t address;
    std::string name;
    bool by_address;
    bool word;
    //not persisted below:
    bool linked; //flag for whether the var name has been searched and refreshed
    bool linkFailed;
} MemoryWatch;

class GameConfig {
public:
    GameConfig(const char* path);
    void Save();
    void UpdateAllPatches(uint8_t* romdata);
    std::vector<BinFileBinding> bin_bindings;
    std::vector<MemoryWatch> watch_locations;
private:
    std::string cfg_path;
};
