#include "game_config.h"
#include "emulator_config.h"
#include "toml/toml.hpp"
#include <filesystem>
#include <iostream>
#include <fstream>

using namespace std::string_view_literals;

GameConfig::GameConfig(const char* path) {
    this->cfg_path = std::string(path);
    if(std::filesystem::exists(this->cfg_path)) {
        toml::table config = toml::parse_file(this->cfg_path);
        auto bindArray = *config.get_as<toml::array>("patch_binds");
        for(auto&& bindEntry : bindArray) {
                BinFileBinding bind;
                toml::table *tbl = bindEntry.as_table();
                bind.address = (int64_t) (*tbl->get_as<int64_t>("addr"));
                bind.bank = (int64_t) (*tbl->get_as<int64_t>("bank"));
                bind.path = (*tbl)["path"].value_or(""sv);
                bin_bindings.emplace_back(bind);
        }
    }
}

void GameConfig::Save() {
    toml::table config = toml::table();
    toml::array bindArray = toml::array();

    for(auto& bindEntry : bin_bindings) {
        toml::table bindTomlEntry = toml::table();
        bindTomlEntry.emplace("addr", bindEntry.address);
        bindTomlEntry.emplace("bank", bindEntry.bank);
        bindTomlEntry.emplace("path", bindEntry.path);
        bindArray.push_back(bindTomlEntry);
    }

    config.emplace("patch_binds", bindArray);

    std::fstream outFile;
    outFile.open(cfg_path, std::ios_base::out | std::ios_base::trunc);
    outFile << config << "\n\n";
    outFile.close();
}

void GameConfig::UpdateAllPatches(uint8_t* romdata) {
    if(!bin_bindings.empty()) {
        EmulatorConfig::noSave = true;
    }
    for(auto& bindEntry : bin_bindings) {
        std::fstream patchFile;
        patchFile.open(bindEntry.path, std::ios_base::in | std::ios_base::binary);
        uint64_t offset = bindEntry.address + ((0x7F & bindEntry.bank) * 0x4000) - 0x8000;
        patchFile.read((char*) &(romdata[offset]), 0x200000 - offset);
        patchFile.close();
    }
}