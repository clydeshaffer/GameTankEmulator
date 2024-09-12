#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include "memory_map.h"

using std::vector;
using std::string;

typedef struct AsmLine {
    string disassembledLine;
    uint16_t address;
    bool isLabel;
} AsmLine;

class Disassembler {
private:
    enum AddressMode {
        XX,  //Invalid
        AB, JX, AX, AY,
        AI, AC, NO, IM,
        PR, ST, ZP, IX,
        ZX, ZY, ZI, IY
    };
    enum ArgIsLabel {
        // Not Label
        N_,
        // Displacement Label
        DL,
        // Absolute Label
        AL
    };
        static void FormatArgBytes(std::stringstream& ss, MemoryMap* mem_map, uint16_t address, uint8_t opcode, uint16_t argBytes);
    static vector<string> opcodeNames;
    static AddressMode opcodeModes[256];
    static ArgIsLabel opcodeTakesLabels[256];
    static vector<AsmLine> lastDecode;
public:

    static vector<AsmLine> Decode(const std::function<uint8_t(uint16_t, bool)> mem_read, MemoryMap* mem_map, uint16_t address, size_t instruction_count);
    static vector<AsmLine> GetLastDecode();
};
