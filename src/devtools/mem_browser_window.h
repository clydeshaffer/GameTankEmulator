
#include "debug_window.h"
#include "memory_map.h"
#include <functional>

class MemBrowserWindow : public DebugWindow {
private:
    MemoryMap* memorymap;
    const std::function<uint8_t(uint16_t, bool)> mem_read;
protected:
    void Render();
public:
    MemBrowserWindow(MemoryMap* map, std::function<uint8_t(uint16_t, bool)> reader):memorymap(map), mem_read(reader) {};
};