#include "../imgui/imgui.h"
#include "profiler_window.h"
#include "implot.h"
#include "../audio_coprocessor.h"

static float prof_R[8] = {1,    1, 1, 0, 0, 0.5f, 0.5f, 1};
static float prof_G[8] = {0, 0.5f, 1, 1, 0,    0, 0.5f, 1};
static float prof_B[8] = {0,    0, 0, 0, 1, 0.5f, 0.5f, 1};

static void recurse_tree_nodes(Profiler::DeepProfileCallNode* node, uint64_t totalCycles) {
    bool opened = ImGui::TreeNode(node, "%s : %d", node->name.c_str(), node->duration);
    ImGui::SameLine(ImGui::GetWindowWidth()-256);
    ImGui::Text("%.1f%%", (100.0f * (float) node->duration) / ((float) totalCycles));
    ImGui::SameLine(ImGui::GetWindowWidth()-216);
    ImVec2 p = ImGui::GetCursorScreenPos();
    float width = (200.0f * (float) node->duration) / ((float) totalCycles);
    float offset = (200.0f * (float) node->offset) / ((float) totalCycles);
    if(width < 1) width = 1;
    ImGui::GetForegroundDrawList()->AddRectFilled(ImVec2(p.x,p.y), ImVec2(p.x+200, p.y+16),
        IM_COL32( 64, 64,128,255));
    ImGui::GetForegroundDrawList()->AddRectFilled(ImVec2(p.x+offset,p.y), ImVec2(p.x+width+offset, p.y+16),
        IM_COL32(255,255,0,255));
    //ImGui::GetForegroundDrawList()->AddText(ImVec2(p.x-32,p.y),IM_COL32_WHITE, "%%");
    ImGui::NewLine();
    if(opened) {
        for(auto& child : node->children) {
            recurse_tree_nodes(child, totalCycles);
        }
        ImGui::TreePop();
    }
}

ImVec2 ProfilerWindow::Render() {

    ImVec2 sizeOut = {0, 0};

    ImGui::Begin("Profiler", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
    ImGui::BeginTabBar("profilertabs", 0);
    if(ImGui::BeginTabItem("Stopwatches")) {
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
        ImGui::EndTabItem();
    }

    if(ImGui::BeginTabItem("Deep Profile")) {
        if(_profiler.lastDeepProfileRoot == nullptr) {
            ImGui::Text("Run a deep profile to see results here");
        } else {
            ImGui::Text("Total: %d cycles", _profiler.lastDeepProfileRoot->duration);
            for(auto& node : _profiler.lastDeepProfileRoot->children) {
                recurse_tree_nodes(node, _profiler.lastDeepProfileRoot->duration);
            }
        }
        ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
    ImGui::SetWindowPos({0, 0});
    ImGui::SetWindowSize({800,800});
    sizeOut = ImVec2(800, 800);
    ImGui::End();
    return sizeOut;
}
