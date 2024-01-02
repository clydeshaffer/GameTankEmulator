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

        if(ImGui::Button("Add by address")) {
            ImGui::OpenPopup("Add Manual Breakpoint");
        }

        if(ImGui::BeginPopup("Add Manual Breakpoint")) {
            static uint16_t manual_addr = 0;
            ImGui::InputScalar("##", ImGuiDataType_U16, &manual_addr, NULL, NULL, "%x", ImGuiInputTextFlags_CharsHexadecimal);
            if(ImGui::Button("Add")) {
                Breakpoints::addresses.insert(manual_addr);
                Breakpoints::manual_breakpoints.insert(manual_addr);
            }
            ImGui::EndPopup();
        }

        for(auto& man : Breakpoints::manual_breakpoints) {
            ImGui::Text("%04x", man);
            ImGui::SameLine();
            if(ImGui::Button("x")) {
                Breakpoints::addresses.erase(man);
                Breakpoints::manual_breakpoints.erase(man);
            }
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