
#include "debug_window.h"
#include "profiler.h"

class ProfilerWindow : public DebugWindow {
private:
    Profiler& _profiler;
    ImVec2 Render();
public:
    ProfilerWindow(Profiler& profiler): _profiler(profiler) {};
};