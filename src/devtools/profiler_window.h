
#include "debug_window.h"
#include "profiler.h"

class ProfilerWindow : public DebugWindow {
private:
    Profiler& _profiler;
    void Render();
public:
    ProfilerWindow(Profiler& profiler): _profiler(profiler) {};
};