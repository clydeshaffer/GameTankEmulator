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
        int GetCount();
        Symbol& GetAt(int i);
        bool FindAddress(uint16_t address, Symbol* result);
};
