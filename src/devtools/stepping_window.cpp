#include "../imgui/imgui.h"
#include "stepping_window.h"
#include "breakpoints.h"
#include "disassembler.h"
#include "imgui-combo-filter.h"
#include "source_map.h"
#include <string>

const char* source_file_getter(const std::vector<std::string> &items, int index) {
    if (index >= 0 && index < (int)items.size()) {
        return items[index].c_str();
    }
    return "N/A";
}

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
        if(ImGui::Button("Add##manualbreak")) {
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

    if(SourceMap::singleton) {
        static int selected_file_idx;
        static int32_t bp_line_num;
        if(ImGui::Button("Add C Breakpoint")) {
            ImGui::OpenPopup("Add C Breakpoint");
        }

        if(ImGui::BeginPopup("Add C Breakpoint")) {
            if(ImGui::ComboFilter("##cfile names", selected_file_idx, SourceMap::singleton->GetFileNames(), source_file_getter, ImGuiComboFlags_HeightRegular )) {
            }
            ImGui::InputScalar("Line", ImGuiDataType_S32, &bp_line_num, NULL, NULL, "%x", 0);
            if(selected_file_idx != -1) {
                if(ImGui::Button("Add##cfilebp")) {
                    auto searchResult = SourceMap::singleton->ReverseSearch(SourceMap::singleton->GetFileNames()[selected_file_idx], bp_line_num);
                    if(searchResult.found) {
                        Breakpoint bp;
                        bp.name = SourceMap::singleton->GetFileNames()[selected_file_idx] + ":" + std::to_string(bp_line_num);
                        bp.address = searchResult.address;
                        bp.by_address = true;
                        bp.bank_set = true;
                        bp.bank = searchResult.bank;
                        bp.enabled = true;
                        bp.linked = true;
                        bp.linkFailed = false;
                        Breakpoints::breakpoints.push_back(bp);
                        gameconfig.Save();
                    }
                }
            }
            ImGui::EndPopup();   
        }
    }

    if(ImGui::Button("Clear breakpoints")) {
        ImGui::OpenPopup("clearbreakpoints");
    }

    if(ImGui::BeginPopup("clearbreakpoints")) {
        ImGui::Text("Are you sure?");
        if(ImGui::Button("Yes##clear")) {
            Breakpoints::breakpoints.clear();
            gameconfig.Save();
            ImGui::CloseCurrentPopup();
        }
        if(ImGui::Button("No##clear")) {
             ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    int index = 0;
    int delete_index = -1;
    bool should_save = false;
    for(auto& man : Breakpoints::breakpoints) {
        ImGui::PushID(index);
        auto color = man.linkFailed ? ImVec4(1, 0, 0, 1) : ImVec4(1, 1, 1, 1);

        if(man.by_address) {
            if(man.bank_set) {
                ImGui::TextColored(color, "%04x:%02x", man.address, man.bank);
            } else {
                ImGui::TextColored(color, "%04x", man.address);
            }
        } else {
            if(man.bank_set) {
                ImGui::TextColored(color, "%s@%04x:%02x", man.name.c_str(), man.address, man.bank);
            } else {
                ImGui::TextColored(color, "%s@%04x", man.name.c_str(), man.address);
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
        ImGui::PopID();
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

    ImGui::BeginTabBar("codetabs", 0);
    if(ImGui::BeginTabItem("Disassembly")) {

        if(timekeeper.clock_mode == CLOCKMODE_STOPPED) {
            for(auto& line : Disassembler::GetLastDecode()) {
                if(line.isLabel) {
                    ImGui::Text("%s", line.disassembledLine.c_str());
                } else {
                    ImGui::Text("%04x %s", line.address, line.disassembledLine.c_str());
                }
            }
        }
        ImGui::EndTabItem();
    }
    if(ImGui::BeginTabItem("Source")) {
        if(timekeeper.clock_mode == CLOCKMODE_STOPPED) {

            if(SourceMap::singleton) {
                SourceMapSearchResult res = SourceMap::singleton->Search(cpu->pc, cartridgestate.bank_mask);
                if(res.found) {
                    SourceMap::singleton->GetFileContent(*res.file);
                    ImGui::Text("%s:%d", res.file->name.c_str(), res.line->line);
                    if(res.file->contents_cached) {
                        ImGui::BeginChild("Source file");
                        int i = 1;
                        
                        for(auto& line : res.file->contents) {
                            auto color = (i == res.line->line) ? ImVec4(1, 1, 1, 1) : ImVec4(0.75, 0.75, 0.75, 1);
                            if(i == res.line->line) {
                                ImGui::Separator();
                            }
                            ImGui::TextColored(color, "%s", line.c_str());
                            if(i == res.line->line) {
                                ImGui::Separator();
                                ImGui::ScrollToItem();
                            }
                            i++;
                        }
                        ImGui::EndChild();
                    } else {
                        ImGui::Text("Couldn't find file. %s/%s", SourceMap::singleton->project_root.c_str(), res.file->name.c_str());
                    }
                } else {
                    ImGui::Text("Couldn't find source for %04x/%02x, debug num is %d", cpu->pc, cartridgestate.bank_mask, res.debug);
                }
            } else {
                ImGui::Text("No source map loaded");
            }
        }
        ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
    ImGui::SetWindowPos({0, 0});
    ImGui::SetWindowSize({480,640});
    sizeOut = ImVec2(480, 640);
    ImGui::End();
    return sizeOut;
}