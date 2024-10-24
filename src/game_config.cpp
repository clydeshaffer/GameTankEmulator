#include "game_config.h"
#include "emulator_config.h"
#include "toml/toml.hpp"
#include <filesystem>
#include <iostream>
#include <fstream>
#include "devtools/breakpoints.h"

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

        if(config.contains("breakpoints")) {
            auto breakpArray = *config.get_as<toml::array>("breakpoints");
            for(auto&& breakpEntry : breakpArray) {
                Breakpoint breakp;
                toml::table *tbl = breakpEntry.as_table();
                breakp.address = (int64_t) (*tbl->get_as<int64_t>("addr"));
                if(tbl->contains("bank")) {
                    breakp.bank = (int64_t) (*tbl->get_as<int64_t>("bank"));
                    breakp.bank_set = true;
                } else {
                    breakp.bank_set = false;
                }
                breakp.name = (*tbl)["name"].value_or(""sv);
                breakp.by_address = (bool) (*tbl->get_as<bool>("by_addr"));
                breakp.enabled = (bool) (*tbl->get_as<bool>("enabled"));
                breakp.linked = false;
                breakp.linkFailed = false;
                Breakpoints::breakpoints.emplace_back(breakp);
            }
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

    toml::array watchArray = toml::array();
    for(auto& watchEntry : watch_locations) {
        toml::table watchTomlEntry = toml::table();
        watchTomlEntry.emplace("addr", watchEntry.address);
        watchTomlEntry.emplace("name", watchEntry.name);
        watchTomlEntry.emplace("word", watchEntry.word);
        watchTomlEntry.emplace("by_addr", watchEntry.by_address);
        watchArray.push_back(watchTomlEntry);
    }
    config.emplace("memory_watches", watchArray);

    toml::array breakArray = toml::array();
    for(auto& breakEntry : Breakpoints::breakpoints) {
        toml::table breakTomlEntry = toml::table();
        breakTomlEntry.emplace("addr", breakEntry.address);
        if(breakEntry.bank_set) {
            breakTomlEntry.emplace("bank", breakEntry.bank);
        }
        breakTomlEntry.emplace("name", breakEntry.name);
        breakTomlEntry.emplace("enabled", breakEntry.enabled);
        breakTomlEntry.emplace("by_addr", breakEntry.by_address);
        breakArray.push_back(breakTomlEntry);
    }
    config.emplace("breakpoints", breakArray);

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