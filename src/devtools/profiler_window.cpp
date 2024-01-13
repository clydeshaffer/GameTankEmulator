#include "../imgui/imgui.h"
#include "profiler_window.h"
#include "implot.h"
#include "../audio_coprocessor.h"

static float prof_R[8] = {1,    1, 1, 0, 0, 0.5f, 0.5f, 1};
static float prof_G[8] = {0, 0.5f, 1, 1, 0,    0, 0.5f, 1};
static float prof_B[8] = {0,    0, 0, 0, 1, 0.5f, 0.5f, 1};

ImVec2 ProfilerWindow::Render() {

    ImVec2 sizeOut = {0, 0};

    ImGui::Begin("Profiler", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
    ImGui::Text("FPS: %d", _profiler.fps);
    ImGui::Text("ACP: %d", AudioCoprocessor::singleton_acp_state->last_irq_cycles);
    ImGui::Text("Graph max:");
    ImGui::SameLine();
    ImGui::InputFloat("##", &max_scale, 0, 0, "%.2f");

    ImGui::Checkbox("Per frame", &_profiler.measure_by_frameflip);
    if (ImGui::IsItemActive() || ImGui::IsItemHovered())
        ImGui::SetTooltip("Aggregate per buffer swap instead of per vsync");

    if(ImPlot::BeginPlot("Timer history")) {
        ImPlot::SetupAxes("", "");
        ImPlot::SetupAxesLimits(0,256, 0, max_scale, ImPlotCond_Always);
        for(int i = 0; i < PROFILER_ENTRIES; ++i) {
            if(profilerSeen[i] && profilerVis[i]) {
                ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(prof_R[i % 7], prof_G[i % 7], prof_B[i % 7], 1.0f));
                ImPlot::PlotLine<float>("", _profiler.profilingHistory[i], PROFILER_HISTORY, 1, 0, 0, _profiler.history_num );
                ImPlot::PopStyleColor();
            }
        }
        ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(prof_R[7], prof_G[7], prof_B[7], 1.0f));
        ImPlot::PlotLine<float>("Blitter", _profiler.blitter_history, PROFILER_HISTORY, 1, 0, 0, _profiler.history_num);
        ImPlot::PopStyleColor();
        ImPlot::EndPlot();
    }
    


    ImGui::BeginChild("Scrolling");
    ImGui::Text("Blit Pixels/Frame: %lu px", _profiler.last_blitter_activity);
    for(int i = 0; i < PROFILER_ENTRIES; ++i) {
        if(_profiler.profilingLastSample[i] != 0) {
            if(!profilerSeen[i]) {
                profilerVis[i] = true;
            }
            profilerSeen[i] = true;
        }
        if(profilerSeen[i]) {
            ImGui::Checkbox("##", &profilerVis[i]);
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(prof_R[i % 8], prof_G[i % 8], prof_B[i % 8], 1.0f), 
            "Timer %02d: %ld / %ld", i, _profiler.profilingLastSample[i], _profiler.profilingLastSampleCount[i]);
        }
    }

    ImGui::EndChild();
    ImGui::SetWindowPos({0, 0});
    ImGui::SetWindowSize({640,480});
    sizeOut = ImVec2(640, 480);
    ImGui::End();
    return sizeOut;
}
