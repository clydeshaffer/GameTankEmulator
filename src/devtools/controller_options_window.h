#include "debug_window.h"
#include "../joystick_adapter.h"

class ControllerOptionsWindow : public DebugWindow {
private:
    JoystickAdapter* inputAdapter;
protected:
    ImVec2 Render();
public:
    ControllerOptionsWindow(JoystickAdapter* inputAdapter);
};