#include "imgui.h"
#include "mem_browser_window.h"
#include "../tinyfd/tinyfiledialogs.h"

#include "./imgui-combo-filter.h"

static const char* hex_headers[17] = {
    " ",
    " 0", " 1", " 2", " 3", " 4", " 5", " 6", " 7",
    " 8", " 9", " A", " B", " C", " D", " E", " F"
};

static const char* var_headers[3] = {
    "Address",
    "Name",
    "Value"
};

static char const * mapFilterPatterns[1] = {"*.map"};

ImVec2 MemBrowserWindow::Render() {
    ImVec2 sizeOut = {0, 0};
    ImGui::Begin("Mem Browser", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
    ImGui::BeginTabBar("memtabs", 0);
    if(ImGui::BeginTabItem("Explorer")) {
        if(ImGui::BeginTable("memtable",17, ImGuiTableFlags_SizingFixedFit, ImVec2(480, 200))) {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn(hex_headers[0], ImGuiTableColumnFlags_None, 48);
            for(int i = 1; i < 17; ++i) {
                ImGui::TableSetupColumn(hex_headers[i], ImGuiTableColumnFlags_None);
            }
            ImGui::TableHeadersRow();

            ImGuiListClipper clipper;
            clipper.Begin(4096);
            while(clipper.Step()) {
                for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
                    {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("%04x", row * 16);
                        for (int column = 0; column < 16; column++)
                        {
                            ImGui::TableSetColumnIndex(column+1);
                            ImGui::Text("%02x", mem_read((row * 16) + column, false));
                        }
                    }
            }
            ImGui::EndTable();
        }
        ImGui::EndTabItem();
    }
    
    if(ImGui::BeginTabItem("Variables")) {
        if(memorymap == NULL) {
            ImGui::Text("Variables will go here if a cc65 linker map file is loaded.");
            if(ImGui::Button("Load")) {
                char* mapFileName = tinyfd_openFileDialog(
                    "Select a map file from a cc65 project",
                    "",
                    1,
                    mapFilterPatterns,
                    "LD65 map file",
                    0
                );
                if(mapFileName) {
                    memorymap = new MemoryMap(std::string(mapFileName));
                }
            }
        } else {
            static int selected_search_item = -1;
            bool searched = false;
            if(ImGui::ComboFilter("##variable names", selected_search_item, *memorymap, memory_map_getter, ImGuiComboFlags_HeightRegular )) {
                searched = true;
            }
            ImGui::Checkbox("Decimal", &decimal);
            if(ImGui::BeginTable("vartable",3, ImGuiTableFlags_SizingFixedFit, ImVec2(480, 200))) {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn(var_headers[0], ImGuiTableColumnFlags_None);
            ImGui::TableSetupColumn(var_headers[1], ImGuiTableColumnFlags_None);
            ImGui::TableSetupColumn(var_headers[2], ImGuiTableColumnFlags_None, 100);
            ImGui::TableHeadersRow();

            ImGuiListClipper clipper;
            clipper.Begin(memorymap->GetCount());
            if(selected_search_item != -1) {
                clipper.IncludeItemByIndex(selected_search_item);
            }
            while(clipper.Step()) {
                for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
                    {
                        ImGui::TableNextRow();
                        const Symbol& sym = memorymap->GetAt(row);
                        size_t size = (row < (memorymap->GetCount()-2)) ? memorymap->GetAt(row+1).address - sym.address : 1;
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("%04x", sym.address);
                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("%s",sym.name.c_str());
                        ImGui::TableSetColumnIndex(2);                        
                        if(sym.address < 0x2000) {
                            ImGui::PushID(row);
                            ImGui::InputScalar("", (size == 1) ? ImGuiDataType_U8 : ImGuiDataType_U16, ram_read(sym.address), NULL, NULL, decimal ? "%d" : "%x", ImGuiInputTextFlags_CharsHexadecimal);
                            ImGui::PopID();
                        } else {
                            ImGui::Text(decimal ? "%d" : "%02x", mem_read(sym.address, false));
                        }
                        if(searched && (row == selected_search_item)) {
                            ImGui::ScrollToItem();
                        }
                    }
            }
            clipper.End();
            ImGui::EndTable();
        }
        }
        ImGui::EndTabItem();
    }

    if(ImGui::BeginTabItem("Watch")) {
        static int selected_item = -1;
        if(memorymap != NULL) {
            if(ImGui::ComboFilter("##variable names", selected_item, *memorymap, memory_map_getter, ImGuiComboFlags_HeightRegular )) {
                if(selected_item != -1) {
                    MemoryWatch watch;
                    watch.address = memorymap->GetAt(selected_item).address;
                    watch.name = memorymap->GetAt(selected_item).name;
                    watch.by_address = false;
                    watch.linked = true;
                    watch.linkFailed = false;
                    gameconfig.watch_locations.push_back(watch);
                    gameconfig.Save();
                }
            }

            if(ImGui::Button("Clear watches")) {
                ImGui::OpenPopup("clearwatches");
            }

            if(ImGui::BeginPopup("clearwatches")) {
                ImGui::Text("Are you sure?");
                if(ImGui::Button("Yes##clear")) {
                    gameconfig.watch_locations.clear();
                    gameconfig.Save();
                    ImGui::CloseCurrentPopup();
                }
                if(ImGui::Button("No##clear")) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
            
            ImGui::Checkbox("Decimal", &decimal);
            if(ImGui::BeginTable("watchtable",5, ImGuiTableFlags_SizingFixedFit, ImVec2(480, 200))) {
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableSetupColumn(var_headers[0], ImGuiTableColumnFlags_None);
                ImGui::TableSetupColumn(var_headers[1], ImGuiTableColumnFlags_None);
                ImGui::TableSetupColumn(var_headers[2], ImGuiTableColumnFlags_None, 100);
                ImGui::TableSetupColumn("word", ImGuiTableColumnFlags_None, 32);
                ImGui::TableSetupColumn("X", ImGuiTableColumnFlags_None, 32);
                ImGui::TableHeadersRow();

                if(gameconfig.watch_locations.size() != 0) {
                    int delete_item = -1;
                    int should_save = 0;
                    for (int row = 0; row < gameconfig.watch_locations.size(); row++)
                    {
                        ImGui::TableNextRow();
                        MemoryWatch& sym = gameconfig.watch_locations.at(row);

                        if((!sym.linked) && !(sym.linkFailed)) {
                            if(memorymap->FindName(sym.address, sym.name)) {
                                sym.linked = true;
                            } else {
                                sym.linkFailed = true;
                            }
                        }

                        if(sym.linkFailed) {
                            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImVec4(1, 0, 0, 1)));
                        }
                        ImGui::PushID(row);

                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("%04x", sym.address);
                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("%s",sym.name.c_str());
                        ImGui::TableSetColumnIndex(2);
                        if(sym.address < 0x2000) {
                            ImGui::InputScalar("##edit", (!sym.word) ? ImGuiDataType_U8 : ImGuiDataType_U16, ram_read(sym.address), NULL, NULL, decimal ? "%d" : "%x", ImGuiInputTextFlags_CharsHexadecimal);
                        } else {
                            ImGui::Text(decimal ? "%d" : "%02x", mem_read(sym.address, false));
                        }
                        ImGui::TableSetColumnIndex(3);
                        if(ImGui::Checkbox("##word", &(sym.word))) {
                            should_save = 1;
                        }
                        ImGui::TableSetColumnIndex(4);
                        if(ImGui::SmallButton("x##deleterow")) {
                            delete_item = row;
                            should_save = 1;
                        }
                        ImGui::PopID();
                    }
                    if(delete_item != -1) {
                        gameconfig.watch_locations.erase(std::next(gameconfig.watch_locations.begin(), delete_item));
                    }
                    if(should_save) {
                        gameconfig.Save();
                    }
                }
                ImGui::EndTable();
            }
        }
        ImGui::EndTabItem();
    }
    
    ImGui::EndTabBar();
    ImGui::SetWindowPos({0, 0});
    ImGui::SetWindowSize({432, 800});
    sizeOut.x = 432;
    sizeOut.y = 800;
    ImGui::End();
    return sizeOut;
}