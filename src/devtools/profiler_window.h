
#include "debug_window.h"
#include "profiler.h"

class ProfilerWindow : public DebugWindow {
private:
    Profiler& _profiler;
    ImVec2 Render();
    float max_scale = 1.1f;
public:
    ProfilerWindow(Profiler& profiler): _profiler(profiler) {};
};