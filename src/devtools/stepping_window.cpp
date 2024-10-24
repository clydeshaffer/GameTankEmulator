#include "../imgui/imgui.h"
#include "stepping_window.h"
#include "breakpoints.h"
#include "disassembler.h"
#include "imgui-combo-filter.h"
#include <string>

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

    ImGui::Checkbox("Enable breakpoints", &Breakpoints::enabled);

    int selected_item = -1;
    if(memorymap != NULL) {
        if(ImGui::ComboFilter("##label names", selected_item, *memorymap, memory_map_getter, ImGuiComboFlags_HeightRegular )) {
            Symbol sym = memorymap->GetAt(selected_item);
            Breakpoint bp;
            bp.address = memorymap->GetAt(selected_item).address;
            bp.name = memorymap->GetAt(selected_item).name;
            bp.by_address = false;
            bp.bank_set = false;
            bp.enabled = true;
            bp.linked = true;
            bp.linkFailed = false;
            Breakpoints::breakpoints.push_back(bp);
            gameconfig.Save();
        }
    }

    if(ImGui::Button("Add by address")) {
        ImGui::OpenPopup("Add Manual Breakpoint");
    }

    if(ImGui::BeginPopup("Add Manual Breakpoint")) {
        static uint16_t manual_addr = 0;
        static uint8_t manual_bank = 0;
        static bool set_bank = false;
        ImGui::InputScalar("Address", ImGuiDataType_U16, &manual_addr, NULL, NULL, "%x", ImGuiInputTextFlags_CharsHexadecimal);
        ImGui::InputScalar("Bank", ImGuiDataType_U8, &manual_bank, NULL, NULL, "%x", ImGuiInputTextFlags_CharsHexadecimal);
        ImGui::Checkbox("Set bank", &set_bank);
        if(ImGui::Button("Add")) {
            Symbol sym = memorymap->GetAt(selected_item);
            Breakpoint bp;
            bp.name = std::string("N/A");
            bp.address = manual_addr;
            bp.by_address = true;
            bp.bank_set = set_bank;
            bp.bank = manual_bank;
            bp.enabled = true;
            bp.linked = true;
            bp.linkFailed = false;
            Breakpoints::breakpoints.push_back(bp);
            gameconfig.Save();
        }
        ImGui::EndPopup();
    }

    int index = 0;
    int delete_index = -1;
    bool should_save = false;
    for(auto& man : Breakpoints::breakpoints) {
        if(man.by_address) {
            if(man.bank_set) {
                ImGui::Text("%04x:%02x", man.address, man.bank);
            } else {
                ImGui::Text("%04x", man.address);
            }
        } else {
            if(man.bank_set) {
                ImGui::Text("%s@%04x:%02x", man.name.c_str(), man.address, man.bank);
            } else {
                ImGui::Text("%s@%04x", man.name.c_str(), man.address);
            }
        }
        ImGui::SameLine();
        if(ImGui::Checkbox("##enable breakpoint", &man.enabled)) {
            should_save = true;
        }
        ImGui::SameLine();
        if(ImGui::Button("x##delete breakpoint")) {
            delete_index = index;
            should_save = true;
        }
        ++index;
    }
    if(delete_index != -1) {
        Breakpoints::breakpoints.erase(std::next(Breakpoints::breakpoints.begin(), delete_index));
    }

    if(should_save) {
        gameconfig.Save();
    }


    ImGui::Separator();

    ImGui::Text("A:      %02x X:     %02x  Y:   %02x", cpu->A, cpu->X, cpu->Y);
    ImGui::Text("Status: %02x Stack: %02x PC: %04x", cpu->status, cpu->sp, cpu->pc);
    ImGui::Text("Cycles Since Boot: %lu", timekeeper.totalCyclesCount);
    ImGui::NewLine();
    if(timekeeper.clock_mode == CLOCKMODE_STOPPED) {
        for(auto& line : Disassembler::GetLastDecode()) {
            if(line.isLabel) {
                ImGui::Text("%s", line.disassembledLine.c_str());
            } else {
                ImGui::Text("%04x %s", line.address, line.disassembledLine.c_str());
            }
        }
    }

    ImGui::SetWindowPos({0, 0});
    ImGui::SetWindowSize({480,640});
    sizeOut = ImVec2(480, 640);
    ImGui::End();
    return sizeOut;
}