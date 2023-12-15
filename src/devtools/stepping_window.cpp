#include "../imgui/imgui.h"
#include "stepping_window.h"
#include "breakpoints.h"
#include "disassembler.h"

ImVec2 SteppingWindow::Render() {
     ImVec2 sizeOut = {0, 0};

    ImGui::Begin("Code Stepper", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
    
    if(ImGui::Button("Step")) {
        timekeeper.clock_mode = CLOCKMODE_SINGLE;
    }
    ImGui::SameLine();
    if(ImGui::Button("Continue")) {
        timekeeper.clock_mode = CLOCKMODE_NORMAL;
    }
    
    ImGui::Separator();

    if (ImGui::TreeNode("Set Breakpoints")) {
        ImGui::Checkbox("Enabled", &Breakpoints::enabled);
        if (ImGui::BeginListBox(""))
        {
            if(memorymap != NULL) {
                int symCount = memorymap->GetCount();
                for (int n = 0; n < symCount; ++n)
                {
                    Symbol sym = memorymap->GetAt(n);
                    if(sym.address >= 0x8000) {
                        if (ImGui::Selectable(sym.name.c_str(), Breakpoints::addresses.count(sym.address) != 0)) {
                            if(Breakpoints::addresses.count(sym.address) != 0) {
                                Breakpoints::addresses.erase(sym.address);
                            } else {
                                Breakpoints::addresses.insert(sym.address);
                            }
                        }
                    }
                }
            }
            ImGui::EndListBox();
        }
        ImGui::TreePop();
    }

    ImGui::Separator();

    ImGui::Text("A:      %02x X:     %02x  Y:   %02x", cpu->A, cpu->X, cpu->Y);
    ImGui::Text("Status: %02x Stack: %02x PC: %04x", cpu->status, cpu->sp, cpu->pc);
    ImGui::Text("Cycles Since Boot: %lu", timekeeper.totalCyclesCount);
    ImGui::NewLine();
    if(timekeeper.clock_mode == CLOCKMODE_STOPPED) {
        for(auto& line : Disassembler::GetLastDecode()) {
            ImGui::Text("%s", line.c_str());
        }
    }

    ImGui::SetWindowPos({0, 0});
    ImGui::SetWindowSize({480,640});
    sizeOut = ImVec2(480, 640);
    ImGui::End();
    return sizeOut;
}