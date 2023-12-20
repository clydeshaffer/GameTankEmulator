
#include "debug_window.h"
#include "memory_map.h"
#include <functional>

class MemBrowserWindow : public DebugWindow {
private:
    MemoryMap*& memorymap;
    const std::function<uint8_t(uint16_t, bool)> mem_read;
    const std::function<uint8_t*(uint16_t)> ram_read;
    bool decimal = false;
protected:
    ImVec2 Render();
public:
    MemBrowserWindow(MemoryMap*& map, std::function<uint8_t(uint16_t, bool)> reader,
    std::function<uint8_t*(uint16_t)> ram_read):
        memorymap(map), 
        mem_read(reader),
        ram_read(ram_read) {};
};