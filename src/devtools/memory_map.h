#pragma once
#include <string>
#include <vector>
#include <functional>
#include <stdint.h>

typedef struct {
    unsigned int address;
    std::string name;
    std::string flags;
} Symbol;

class MemoryMap {
    private:
        std::vector<Symbol> symbols;
        std::string filename;
        void parse();
        void printSymbols() const;
    public:
        MemoryMap();
        MemoryMap(const std::string& mapfile);
        void forEach(const std::function<void(const Symbol&)>& func) const;
        int GetCount() const;
        int size() const;
        const Symbol& GetAt(int i) const;
        bool FindAddress(uint16_t address, Symbol* result) const;
        bool FindName(uint16_t &address, std::string name) const;
};

const char* memory_map_getter(const MemoryMap& items, int index);