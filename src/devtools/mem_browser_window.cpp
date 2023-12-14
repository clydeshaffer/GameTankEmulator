#include "imgui.h"
#include "mem_browser_window.h"
#include "../tinyfd/tinyfiledialogs.h"

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
            if(ImGui::BeginTable("vartable",3, ImGuiTableFlags_SizingFixedFit, ImVec2(480, 200))) {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn(var_headers[0], ImGuiTableColumnFlags_None);
            ImGui::TableSetupColumn(var_headers[1], ImGuiTableColumnFlags_None);
            ImGui::TableSetupColumn(var_headers[2], ImGuiTableColumnFlags_None, 100);
            ImGui::TableHeadersRow();

            ImGuiListClipper clipper;
            clipper.Begin(memorymap->GetCount());
            while(clipper.Step()) {
                for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
                    {
                        ImGui::TableNextRow();
                        Symbol& sym = memorymap->GetAt(row);
                        size_t size = (row < (memorymap->GetCount()-2)) ? memorymap->GetAt(row+1).address - sym.address : 1;
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("%04x", sym.address);
                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("%s",sym.name.c_str());
                        ImGui::TableSetColumnIndex(2);
                        
                        if(sym.address < 0x2000) {
                            ImGui::PushID(row);
                            ImGui::InputScalar("", (size == 1) ? ImGuiDataType_U8 : ImGuiDataType_U16, ram_read(sym.address), NULL, NULL, "%x", ImGuiInputTextFlags_CharsHexadecimal);
                            ImGui::PopID();
                        } else {
                            ImGui::Text("%02x", mem_read(sym.address, false));
                        }
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