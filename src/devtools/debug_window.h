#pragma once
#include "imgui.h"
class DebugWindow {
protected:
    bool open = true;
    ImGuiContext* ctx;
    virtual void Render() = 0;
public:
    DebugWindow();
    ~DebugWindow();
    bool IsOpen();
    void Draw();
};