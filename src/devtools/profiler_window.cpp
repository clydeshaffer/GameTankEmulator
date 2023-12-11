#include "../imgui/imgui.h"
#include "profiler_window.h"
#include "implot.h"

static float prof_R[8] = {1, 1, 1, 0, 0, 0.5f, 0.5f, 1};
static float prof_G[8] = {0, 0.5f, 1, 1, 0, 0, 0.5f, 1};
static float prof_B[8] = {0, 0, 0, 0, 1, 0.5f, 0.5f, 1};

ImVec2 ProfilerWindow::Render() {

    ImVec2 sizeOut = {0, 0};

    ImGui::Begin("Profiler", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
    ImGui::Text("FPS: %d", _profiler.fps);
    
    if(ImPlot::BeginPlot("Timer history")) {
        ImPlot::SetupAxes("", "");
        ImPlot::SetupAxesLimits(0,256, 0, 2, ImPlotCond_Always);
        for(int i = 0; i < PROFILER_ENTRIES; ++i) {
            ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(prof_R[i % 8], prof_G[i % 8], prof_B[i % 8], 1.0f));
            ImPlot::PlotLine<float>("", _profiler.profilingHistory[i], PROFILER_HISTORY, 1, 0, 0, _profiler.history_num );
        }
        ImPlot::EndPlot();
    }
    
    ImGui::BeginChild("Scrolling");
    for(int i = 0; i < PROFILER_ENTRIES; ++i) {
        if(_profiler.profilingTimes[i] != 0) {
            ImGui::TextColored(ImVec4(prof_R[i % 8], prof_G[i % 8], prof_B[i % 8], 1.0f), 
            "Timer %02d: %ld / %ld", i, _profiler.profilingTimes[i], _profiler.profilingCounts[i]);
        }
    }

    ImGui::EndChild();
    ImGui::SetWindowPos({0, 0});
    ImGui::SetWindowSize({640,480});
    sizeOut = ImVec2(640, 480);
    ImGui::End();
    return sizeOut;
}
