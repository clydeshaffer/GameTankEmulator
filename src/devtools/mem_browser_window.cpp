#include "imgui.h"
#include "mem_browser_window.h"

static const char* headers[17] = {
    " ",
    " 0", " 1", " 2", " 3", " 4", " 5", " 6", " 7",
    " 8", " 9", " A", " B", " C", " D", " E", " F"
};

ImVec2 MemBrowserWindow::Render() {
    ImVec2 sizeOut = {0, 0};
    ImGui::Begin("Mem Browser", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
    if(ImGui::BeginTable("memtable",17, ImGuiTableFlags_SizingFixedFit, ImVec2(480, 200))) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn(headers[0], ImGuiTableColumnFlags_None, 48);
        for(int i = 1; i < 17; ++i) {
            ImGui::TableSetupColumn(headers[i], ImGuiTableColumnFlags_None);
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
    ImGui::SetWindowPos({0, 0});
    ImGui::SetWindowSize({0, 800});
    sizeOut = ImGui::GetItemRectSize();
    sizeOut.y = 800;
    ImGui::End();
    return sizeOut;
}