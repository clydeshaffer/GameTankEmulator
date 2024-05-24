#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include "memory_map.h"

using std::vector;
using std::string;

class Disassembler {
private:
    enum AddressMode {
        XX,  //Invalid
        AB, JX, AX, AY,
        AI, AC, NO, IM,
        PR, ST, ZP, IX,
        ZX, ZY, ZI, IY
    };
    static void FormatArgBytes(std::stringstream& ss, MemoryMap* mem_map, AddressMode mode, uint8_t argCount, uint16_t argBytes);
    static vector<string> opcodeNames;
    static AddressMode opcodeModes[256];
    static vector<string> lastDecode;
public:

    static vector<string> Decode(const std::function<uint8_t(uint16_t, bool)> mem_read, MemoryMap* mem_map, uint16_t address, size_t instruction_count);
    static vector<string> GetLastDecode();
};
