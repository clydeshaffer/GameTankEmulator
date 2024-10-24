#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "memory_map.h"
#include <cstdint>

const char* memory_map_getter(const MemoryMap& items, int index) {
    if (index >= 0 && index < (int)items.GetCount()) {
        return items.GetAt(index).name.c_str();
    }
    return "N/A";
}

void MemoryMap::parse() {
    std::ifstream file(filename);
    std::string line;

    // Skip lines until the "Exports list by value:" line is found
    while (std::getline(file, line)) {
        if (line.find("Exports list by value:") != std::string::npos) {
            break;
        }
    }

    // Skip the line containing only dash characters
    std::getline(file, line);

    // Read and parse the lines containing two name and address entries each
    while (std::getline(file, line)) {
        if (line.empty() || line.find('-') != std::string::npos) {
            // Break loop if an empty line or a line with dashes is encountered
            break;
        }

        std::istringstream iss(line);
        // Read the first entry
        Symbol symbol1;
        iss >> symbol1.name >> std::hex >> symbol1.address >> symbol1.flags;
        symbols.push_back(symbol1);

        // Try to read the second entry, if available
        Symbol symbol2;
        if (iss >> symbol2.name >> std::hex >> symbol2.address >> symbol2.flags) {
            symbols.push_back(symbol2);
        }
    }
}

void MemoryMap::printSymbols() const {
    for (const auto& symbol : symbols) {
        std::cout << "Name: " << symbol.name << ", Address: 0x" << std::hex << std::uppercase << symbol.address
                    << ", Flags: " << symbol.flags << std::dec << std::endl;
    }
}

MemoryMap::MemoryMap() {}

MemoryMap::MemoryMap(const std::string& mapfile) : filename(mapfile) {
    parse();
}

void MemoryMap::forEach(const std::function<void(const Symbol&)>& func) const {
    for (const auto& symbol : symbols) {
            func(symbol);
        }
}

int MemoryMap::GetCount() const {
    return symbols.size();
}

int MemoryMap::size() const {
    return symbols.size();
}

const Symbol& MemoryMap::GetAt(int i) const {
    return symbols.at(i);
}

bool MemoryMap::FindAddress(uint16_t address, Symbol* result) const {
    for(auto sym : symbols) {
        if(sym.address == address) {
            if(result != NULL) {
                *result = sym;
            }
            return true;
        }
    }
    return false;
}

bool MemoryMap::FindName(uint16_t &address, std::string name) const {
    for(auto sym : symbols) {
        if(sym.name == name) {
            address = sym.address;
            return true;
        }
    }
    return false;   
}