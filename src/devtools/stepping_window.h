#include "debug_window.h"
#include "memory_map.h"
#include "../timekeeper.h"
#include <functional>
#include "../mos6502/mos6502.h"
#include "../game_config.h"

class SteppingWindow : public DebugWindow {
private:
    Timekeeper& timekeeper;
    MemoryMap*& memorymap;
    mos6502* cpu;
    GameConfig& gameconfig;
protected:
    ImVec2 Render();
public:
    SteppingWindow(Timekeeper& timekeeper, MemoryMap*& memorymap, mos6502* cpu, GameConfig& gameconfig) : timekeeper(timekeeper), memorymap(memorymap), cpu(cpu), gameconfig(gameconfig){};
};