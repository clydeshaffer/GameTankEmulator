#include "debug_window.h"
#include "memory_map.h"
#include "../game_config.h"
#include <functional>

class PatchingWindow : public DebugWindow {
private:
    MemoryMap*& memorymap;
    GameConfig*& gameconfig;
    std::string new_binding_path;
protected:
    ImVec2 Render();
public:
    PatchingWindow(MemoryMap*& memorymap, GameConfig*& gameconfig) : memorymap(memorymap), gameconfig(gameconfig) {};
};