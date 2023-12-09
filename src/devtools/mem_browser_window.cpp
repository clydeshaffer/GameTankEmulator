#include "imgui.h"
#include "mem_browser_window.h"

static const char* headers[16] = {
    " 0", " 1", " 2", " 3", " 4", " 5", " 6", " 7",
    " 8", " 9", " A", " B", " C", " D", " E", " F"
};

void MemBrowserWindow::Draw() {
    ImGui::Begin("Mem Browser", NULL, ImGuiWindowFlags_None);
    ImGui::SetWindowFontScale(2);
    if(ImGui::BeginTable("memtable",16, 0, ImVec2(800, 400))) {
        ImGui::TableSetupScrollFreeze(0, 1);
        for(int i = 0; i < 16; ++i) {
            ImGui::TableSetupColumn(headers[i], ImGuiTableColumnFlags_None);
        }
        ImGui::TableHeadersRow();

        ImGuiListClipper clipper;
        clipper.Begin(4096);
        while(clipper.Step()) {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
                {
                    ImGui::TableNextRow();
                    for (int column = 0; column < 16; column++)
                    {
                        ImGui::TableSetColumnIndex(column);
                        ImGui::Text("%02x", mem_read((row * 16) + column, false));
                    }
                }
        }
        ImGui::EndTable();
    }

    ImGui::End();
}