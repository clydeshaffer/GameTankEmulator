#include "../imgui/imgui.h"
#include "patching_window.h"
#include "../tinyfd/tinyfiledialogs.h"

static const char* var_headers[4] = {
    "Address",
    "Bank",
    "Path",
    ""
};

ImVec2 PatchingWindow::Render() {
     ImVec2 sizeOut = {0, 0};

    ImGui::Begin("ROM Patcher", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

    if(ImGui::Button("+")) {
        new_binding_path = std::string("none");
        ImGui::OpenPopup("Add File Binding");
    }

    if(ImGui::BeginPopup("Add File Binding")) {
        static uint16_t manual_addr = 0;
        static uint8_t manual_bank = 0;
        ImGui::Text("Addr");
        ImGui::SameLine();
        ImGui::InputScalar("##", ImGuiDataType_U16, &manual_addr, NULL, NULL, "%x", ImGuiInputTextFlags_CharsHexadecimal);
        ImGui::Text("Bank");
        ImGui::SameLine();
        ImGui::InputScalar("###", ImGuiDataType_U8, &manual_bank, NULL, NULL, "%x", ImGuiInputTextFlags_CharsHexadecimal);
        if(ImGui::Button("Pick file")) {
            char* filename = tinyfd_openFileDialog(
                "Pick a file to patch to this address",
                "",
                0,
                NULL,
                NULL,
                0
            );
            if(filename) {
                new_binding_path = std::string(filename);
            }
        }
        ImGui::SameLine();
        ImGui::Text("%s", new_binding_path.c_str());
        if(ImGui::Button("Add")) {
            BinFileBinding newbind;
            newbind.address = manual_addr;
            newbind.bank = manual_bank;
            newbind.path = new_binding_path;
            gameconfig->bin_bindings.emplace_back(newbind);
            gameconfig->Save();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::Separator();

    if(ImGui::BeginTable("bindtable",4, ImGuiTableFlags_SizingFixedFit, ImVec2(480, 200))) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn(var_headers[0], ImGuiTableColumnFlags_None);
        ImGui::TableSetupColumn(var_headers[1], ImGuiTableColumnFlags_None);
        ImGui::TableSetupColumn(var_headers[2], ImGuiTableColumnFlags_None, 100);
        ImGui::TableSetupColumn(var_headers[3], ImGuiTableColumnFlags_None);
        ImGui::TableHeadersRow();

        ImGuiListClipper clipper;
        clipper.Begin(gameconfig->bin_bindings.size());
        int del_index = -1;
        while(clipper.Step()) {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
                {
                    ImGui::TableNextRow();
                    BinFileBinding& binding = gameconfig->bin_bindings.at(row);
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%04x", binding.address);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%02x", binding.bank);
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%s",binding.path.c_str());
                    ImGui::TableSetColumnIndex(3);
                     ImGui::Text("%s",binding.path.c_str());
                    if(ImGui::Button("X")) {
                        del_index = row;
                    };
                }
        }
        if(del_index != -1) {
            gameconfig->bin_bindings.erase(gameconfig->bin_bindings.begin() + del_index);
            gameconfig->Save();
        }
        ImGui::EndTable();
    }

    ImGui::SetWindowPos({0, 0});
    ImGui::SetWindowSize({480,640});
    sizeOut = ImVec2(480, 640);
    ImGui::End();
    return sizeOut;
}