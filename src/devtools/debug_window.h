#pragma once
#include "../SDL_inc.h"
#include "imgui.h"
#include "implot.h"
#include "base_window.h"

class DebugWindow : public BaseWindow {
private:
    SDL_Renderer* renderer;
    ImGuiContext* ctx;
    ImPlotContext* plot_ctx;
    bool size_dirty = true;
protected:
    virtual ImVec2 Render() = 0;
public:
    DebugWindow();
    virtual ~DebugWindow();
    void Draw();
    void HandleEvent(SDL_Event& e);
};