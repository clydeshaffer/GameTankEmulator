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
        if(config.contains("patch_binds")) {
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

        if(config.contains("memory_watches")) {
            auto watchArray = *config.get_as<toml::array>("memory_watches");
            for(auto&& watchEntry : watchArray) {
                MemoryWatch watch;
                toml::table *tbl = watchEntry.as_table();
                watch.address = (int64_t) (*tbl->get_as<int64_t>("addr"));
                watch.name = (*tbl)["name"].value_or(""sv);
                watch.word = (bool) (*tbl->get_as<bool>("word"));
                watch.by_address = (bool) (*tbl->get_as<bool>("by_addr"));
                watch.linked = false;
                watch.linkFailed = false;
                watch_locations.emplace_back(watch);
            }
        }
    }
}

void GameConfig::Save() {
    toml::table config = toml::table();
    toml::array bindArray = toml::array();
    toml::array watchArray = toml::array();

    for(auto& bindEntry : bin_bindings) {
        toml::table bindTomlEntry = toml::table();
        bindTomlEntry.emplace("addr", bindEntry.address);
        bindTomlEntry.emplace("bank", bindEntry.bank);
        bindTomlEntry.emplace("path", bindEntry.path);
        bindArray.push_back(bindTomlEntry);
    }

    config.emplace("patch_binds", bindArray);

    for(auto& watchEntry : watch_locations) {
        toml::table watchTomlEntry = toml::table();
        watchTomlEntry.emplace("addr", watchEntry.address);
        watchTomlEntry.emplace("name", watchEntry.name);
        watchTomlEntry.emplace("word", watchEntry.word);
        watchTomlEntry.emplace("by_addr", watchEntry.by_address);
        watchArray.push_back(watchTomlEntry);
    }
    config.emplace("memory_watches", watchArray);

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