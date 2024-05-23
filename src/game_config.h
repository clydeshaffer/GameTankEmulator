#include <string>
#include <vector>

typedef struct BinFileBinding {
    uint16_t address;
    uint8_t bank;
    std::string path;
} BinFileBinding;

class GameConfig {
public:
    GameConfig(const char* path);
    void Save();
    void UpdateAllPatches(uint8_t* romdata);
    std::vector<BinFileBinding> bin_bindings;
private:
    std::string cfg_path;
};