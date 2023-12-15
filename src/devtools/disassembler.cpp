#include "disassembler.h"
#include <sstream>
#include <iostream>
#include <iomanip>

vector<string> Disassembler::lastDecode;

vector<string> Disassembler::opcodeNames = {
    "BRK", "ORA", "???", "???", "TSB", "ORA", "ASL", "RMB0", "PHP", "ORA", "ASL", "???", "TSB", "ORA", "ASL", "BBR0",
    "BPL", "ORA", "ORA", "???", "TRB", "ORA", "ASL", "RMB1", "CLC", "ORA", "INC", "???", "TRB", "ORA", "ASL", "BBR1",
    "JSR", "AND", "???", "???", "BIT", "AND", "ROL", "RMB2", "PLP", "AND", "ROL", "???", "BIT", "AND", "ROL", "BBR2",
    "BMI", "AND", "AND", "???", "BIT", "AND", "ROL", "RMB3", "SEC", "AND", "DEC", "???", "BIT", "AND", "ROL", "BBR3",
    "RTI", "EOR", "???", "???", "???", "EOR", "LSR", "RMB4", "PHA", "EOR", "LSR", "???", "JMP", "EOR", "LSR", "BBR4",
    "BVC", "EOR", "EOR", "???", "???", "EOR", "LSR", "RMB5", "CLI", "EOR", "PHY", "???", "???", "EOR", "LSR", "BBR5",
    "RTS", "ADC", "???", "???", "STZ", "ADC", "ROR", "RMB6", "PLA", "ADC", "ROR", "???", "JMP", "ADC", "ROR", "BBR6",
    "BVS", "ADC", "ADC", "???", "STZ", "ADC", "ROR", "RMB7", "SEI", "ADC", "PLY", "???", "JMP", "ADC", "ROR", "BBR7",
    "BRA", "STA", "???", "???", "STY", "STA", "STX", "SMB0", "DEY", "BIT", "TXA", "???", "STY", "STA", "STX", "BBS0",
    "BCC", "STA", "STA", "???", "STY", "STA", "STX", "SMB1", "TYA", "STA", "TXS", "???", "STZ", "STA", "STZ", "BBS1",
    "LDY", "LDA", "LDX", "???", "LDY", "LDA", "LDX", "SMB2", "TAY", "LDA", "TAX", "???", "LDY", "LDA", "LDX", "BBS2",
    "BCS", "LDA", "LDA", "???", "LDY", "LDA", "LDX", "SMB3", "CLV", "LDA", "TSX", "???", "LDY", "LDA", "LDX", "BBS3",
    "CPY", "CMP", "???", "???", "CPY", "CMP", "DEC", "SMB4", "INY", "CMP", "DEX", "WAI", "CPY", "CMP", "DEC", "BBS4",
    "BNE", "CMP", "CMP", "???", "???", "CMP", "DEC", "SMB5", "CLD", "CMP", "PHX", "STP", "???", "CMP", "DEC", "BBS5",
    "CPX", "SBC", "???", "???", "CPX", "SBC", "INC", "SMB6", "INX", "SBC", "NOP", "???", "CPX", "SBC", "INC", "BBS6",
    "BEQ", "SBC", "SBC", "???", "???", "SBC", "INC", "SMB7", "SED", "SBC", "PLX", "???", "???", "SBC", "INC", "BBS7"
    };

Disassembler::AddressMode Disassembler::opcodeModes[256] = {
    //BRK is listed as "stack" address mode but is a two byte instruction
    NO,IX,XX,XX,ZP,ZP,ZP,ZP,ST,NO,AC,XX,AB,AB,AB,PR,
    PR,IY,ZI,XX,ZP,ZX,ZX,ZP,IM,AY,AC,XX,AB,AX,AX,PR,
    AB,IX,XX,XX,ZP,ZP,ZP,ZP,ST,NO,AC,XX,AB,AB,AB,PR,
    PR,IY,ZI,XX,ZX,ZX,ZX,ZP,IM,AY,AC,XX,AX,AX,AX,PR,
    ST,IX,XX,XX,XX,ZP,ZP,ZP,ST,NO,AC,XX,AB,AB,AB,PR,
    PR,IY,ZI,XX,XX,ZX,ZX,ZP,IM,AY,AC,XX,XX,AX,AX,PR,
    ST,IX,XX,XX,ZP,ZP,ZP,ZP,ST,NO,AC,XX,AI,AB,AB,PR,
    PR,IY,ZI,XX,ZX,ZX,ZX,ZP,IM,AY,ST,XX,JX,AX,AX,PR,
    PR,IX,XX,XX,ZP,ZP,ZP,ZP,IM,NO,IM,XX,AB,AB,AB,PR,
    PR,IY,ZI,XX,ZX,ZX,ZY,ZP,IM,AY,IM,XX,AB,AX,AX,PR,
    NO,IX,NO,XX,ZP,ZP,ZP,ZP,IM,NO,IM,XX,AB,AB,AB,PR,
    PR,IY,ZI,XX,ZX,ZX,ZY,ZP,IM,AY,IM,XX,AX,AX,AY,PR,
    NO,IX,XX,XX,ZP,ZP,ZP,ZP,IM,NO,IM,IM,AB,AB,AB,PR,
    PR,IY,ZI,XX,XX,ZX,ZX,ZP,IM,AY,ST,IM,XX,AX,AX,PR,
    NO,IX,XX,XX,ZP,ZP,ZP,ZP,IM,NO,IM,XX,AB,AB,AB,PR,
    PR,IY,ZI,XX,XX,ZX,ZX,ZP,IM,AY,ST,XX,XX,AX,AX,PR
};

static size_t opBytes[17] = {
    1,
    3, 3, 3, 3,
    3, 1, 2, 1,
    2, 1, 2, 2,
    2, 2, 2, 2,
};

void Disassembler::FormatArgBytes(std::stringstream& ss, MemoryMap* mem_map, AddressMode mode, uint16_t argBytes) {
    switch(mode) {
        case AB:
            //Absolute
            ss << "$" << std::hex << argBytes;
            break;
        case JX:
            //Absolute Indexed Indirect (Indexed JMP)
            ss << "($" << std::hex << argBytes << ", x)";
            break;
        case AX:
            //Absolute Indexed X
            ss << "$" << std::hex << argBytes << ", x";
            break;
        case AY:
            //Absolute Indexed Y
            ss << "$" << std::hex << argBytes << ", y";
            break;
        case AI:
            //Absolute Indirect
            ss << "($" << std::hex << argBytes << ")";
            break;
        case AC:
            //Accumulator
            break;
        case NO:
            //Immediate (NO for Number)
            ss << "#$" << std::hex << argBytes;
            break;
        case IM:
            //Implied
            break;
        case PR:
            //Program Counter Relative
            ss << "$" << std::hex << argBytes;
            break;
        case ST:
            //Stack
            break;
        case ZP:
            //Zero page
            ss << "$" << std::hex << argBytes;
            break;
        case IX:
            //Zero Page Indexed Indirect X
            ss << "($" << std::hex << argBytes << ", x)";
            break;
        case ZX:
            //Zero Page Indexed X
            ss << "($" << std::hex << argBytes << ", x)";
            break;
        case ZY:
            //Zero Page Indexed Y
            ss << "$" << std::hex << argBytes << ", y";
            break;
        case ZI:
            //Zero Page Indirect
            ss << "($" << std::hex << argBytes << ")";
            break;
        case IY:
            //Zerp Page Indirect Indexed Y
            ss << "($" << std::hex << argBytes << "), y";
            break;
        case XX:
            //Treat invalid opcode as no bytes
            break;
    }
}

vector<string> Disassembler::Decode(const std::function<uint8_t(uint16_t, bool)> mem_read, MemoryMap* mem_map, uint16_t address, size_t instruction_count) {
    vector<string>& output = lastDecode;
    output.clear();

    while(instruction_count--) {
        std::stringstream ss;
        if(mem_map) {
            Symbol sym;
            if(mem_map->FindAddress(address, &sym)) {
                ss << sym.name << ": ";
            }
        }
        string& opcodeName = opcodeNames[mem_read(address, false)];
        AddressMode mode = opcodeModes[mem_read(address, false)];
        size_t argByteCount = opBytes[mode] - 1;
        ++address;
        uint16_t args = 0;
        if(argByteCount == 0) {
            output.push_back(opcodeName);
        } else {
            args = mem_read(address++, false);
            if(argByteCount == 2) {
                args += (mem_read(address++, false)) << 8;
            }
            ss << opcodeName << " ";
            FormatArgBytes(ss, mem_map, mode, args);
            output.push_back(ss.str());
        }
    }

    return output;
}

vector<string> Disassembler::GetLastDecode() {
    return lastDecode;
}