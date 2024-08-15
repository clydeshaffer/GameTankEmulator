#pragma once
#include "../SDL_inc.h"
#include "imgui.h"
#include "implot.h"
#include "base_window.h"

class DebugWindow : public BaseWindow {
private:
    ImGuiContext* ctx;
    ImPlotContext* plot_ctx;
    bool size_dirty = true;
protected:
    SDL_Renderer* renderer;
    virtual ImVec2 Render() = 0;
public:
    DebugWindow();
    virtual ~DebugWindow();
    void Draw();
    void HandleEvent(SDL_Event& e);
};