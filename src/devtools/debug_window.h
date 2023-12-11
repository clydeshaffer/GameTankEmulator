#pragma once
#include "../SDL_inc.h"
#include "imgui.h"
#include "implot.h"

class DebugWindow {
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    ImGuiContext* ctx;
    ImPlotContext* plot_ctx;
    bool size_dirty = true;
protected:
    bool open = true;
    virtual ImVec2 Render() = 0;
public:
    DebugWindow();
    ~DebugWindow();
    bool IsOpen();
    void Draw();
    void HandleEvent(SDL_Event& e);
};