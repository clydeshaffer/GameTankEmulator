#include "disassembler.h"
#include <sstream>
#include <iostream>
#include <iomanip>
#include <assert.h>

#define MAX_INSTRUCTION_SIZE 3
#define MAX_INSTRUCTION_NAME_LEN 4
#define ARG_PAD_LEN 20

vector<AsmLine> Disassembler::lastDecode;

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

Disassembler::ArgIsLabel Disassembler::opcodeTakesLabels[256] = {
    N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,DL,
    DL,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,DL,
    AL,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,DL,
    DL,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,DL,
    N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,AL,N_,N_,DL,
    DL,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,DL,
    N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,AL,N_,N_,DL,
    DL,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,AL,N_,N_,DL,
    DL,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,DL,
    DL,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,DL,
    N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,DL,
    DL,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,DL,
    N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,DL,
    DL,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,DL,
    N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,DL,
    DL,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,N_,DL,
};

Disassembler::AddressMode Disassembler::opcodeModes[256] = {
    //BRK is listed as "stack" address mode but is a two byte instruction
    NO,IX,XX,XX,ZP,ZP,ZP,ZP,ST,NO,AC,XX,AB,AB,AB,BB,
    PR,IY,ZI,XX,ZP,ZX,ZX,ZP,IM,AY,AC,XX,AB,AX,AX,BB,
    AB,IX,XX,XX,ZP,ZP,ZP,ZP,ST,NO,AC,XX,AB,AB,AB,BB,
    PR,IY,ZI,XX,ZX,ZX,ZX,ZP,IM,AY,AC,XX,AX,AX,AX,BB,
    ST,IX,XX,XX,XX,ZP,ZP,ZP,ST,NO,AC,XX,AB,AB,AB,BB,
    PR,IY,ZI,XX,XX,ZX,ZX,ZP,IM,AY,AC,XX,XX,AX,AX,BB,
    ST,IX,XX,XX,ZP,ZP,ZP,ZP,ST,NO,AC,XX,AI,AB,AB,BB,
    PR,IY,ZI,XX,ZX,ZX,ZX,ZP,IM,AY,ST,XX,JX,AX,AX,BB,
    PR,IX,XX,XX,ZP,ZP,ZP,ZP,IM,NO,IM,XX,AB,AB,AB,BB,
    PR,IY,ZI,XX,ZX,ZX,ZY,ZP,IM,AY,IM,XX,AB,AX,AX,BB,
    NO,IX,NO,XX,ZP,ZP,ZP,ZP,IM,NO,IM,XX,AB,AB,AB,BB,
    PR,IY,ZI,XX,ZX,ZX,ZY,ZP,IM,AY,IM,XX,AX,AX,AY,BB,
    NO,IX,XX,XX,ZP,ZP,ZP,ZP,IM,NO,IM,IM,AB,AB,AB,BB,
    PR,IY,ZI,XX,XX,ZX,ZX,ZP,IM,AY,ST,IM,XX,AX,AX,BB,
    NO,IX,XX,XX,ZP,ZP,ZP,ZP,IM,NO,IM,XX,AB,AB,AB,BB,
    PR,IY,ZI,XX,XX,ZX,ZX,ZP,IM,AY,ST,XX,XX,AX,AX,BB,
};

static size_t opBytes[18] = {
    1,
    3, 3, 3, 3,
    3, 1, 2, 1,
    2, 1, 2, 2,
    2, 2, 2, 2,
    3,
};


void Disassembler::FormatArgBytes(std::stringstream& ss, MemoryMap* mem_map, uint16_t address, uint8_t opcode, uint16_t argBytes) {
    #define FMT_ARG std::setw(argCount * 2) << std::setfill('0') << std::right << std::hex << argBytes
    std::stringstream argstream;

    AddressMode mode = opcodeModes[opcode];
    uint8_t argCount = opBytes[mode] - 1;
    bool isAbsolute = false;

    switch(mode) {
        case AB:
        case JX:
        case AX:
        case AY:
        case AI:
            isAbsolute = true;
            break;
        default:
            break;
    }

    Symbol sym;
    if(isAbsolute && mem_map->FindAddress(argBytes, &sym)) {
        string name = sym.name;
        switch(mode) {
            case AB:
                //Absolute
                argstream << "$" << name;
                break;
            case JX:
                //Absolute Indexed Indirect (Indexed JMP)
                argstream << "($" << name << ", x)";
                break;
            case AX:
                //Absolute Indexed X
                argstream << "$" << name << ", x";
                break;
            case AY:
                //Absolute Indexed Y
                argstream << "$" << name << ", y";
                break;
            case AI:
                //Absolute Indirect
                argstream << "($" << name << ")";
                break;
            default:
                break;
        }
    } else {
        switch(mode) {
            case AB:
                //Absolute
                argstream << "$" << FMT_ARG;
                break;
            case JX:
                //Absolute Indexed Indirect (Indexed JMP)
                argstream << "($" << FMT_ARG << ", x)";
                break;
            case AX:
                //Absolute Indexed X
                argstream << "$" << FMT_ARG << ", x";
                break;
            case AY:
                //Absolute Indexed Y
                argstream << "$" << FMT_ARG << ", y";
                break;
            case AI:
                //Absolute Indirect
                argstream << "($" << FMT_ARG << ")";
                break;
            case AC:
                //Accumulator
                break;
            case NO:
                //Immediate (NO for Number)
                argstream << "#$" << FMT_ARG;
                break;
            case IM:
                //Implied
                break;
            case PR:
                //Program Counter Relative
                //Check to see if the we're branching
                if (mem_map && opcodeTakesLabels[opcode] == DL) {
                    Symbol sym;
                    //Check to see if the branch target is a named label
                    if(mem_map->FindAddress(address + (char) argBytes, &sym)) {
                        string name = sym.name;
                        argstream << name;
                        break;
                    }
                }

                argstream << "$" << FMT_ARG;
                break;
            case ST:
                //Stack
                break;
            case ZP:
                //Zero page
                argstream << "$" << FMT_ARG;
                break;
            case IX:
                //Zero Page Indexed Indirect X
                argstream << "($" << FMT_ARG << ", x)";
                break;
            case ZX:
                //Zero Page Indexed X
                argstream << "($" << FMT_ARG << ", x)";
                break;
            case ZY:
                //Zero Page Indexed Y
                argstream << "$" << FMT_ARG << ", y";
                break;
            case ZI:
                //Zero Page Indirect
                argstream << "($" << FMT_ARG << ")";
                break;
            case IY:
                //Zero Page Indirect Indexed Y
                argstream << "($" << FMT_ARG << "), y";
                break;
	    case BB: {
		// BB Weird instruction style used by BBRx and BBSx instructions
		// Unlike all other instructions this takes two arguments
		// First, a location in the zero page to test the given bit of
		uint16_t zpArg = (argBytes & 0x00FF);
		// Second, an relative jump offset
		uint16_t relArg = (argBytes & 0xFF00) >> 8;
		// In this regard it is sort of like a zero page and relative instruction combined
		// Because this takes multiple distinct args, we won't be able to make use of the FMT_ARG marco here :(

		// Print the first argument and separator here
		// TODO might want to check for named zero page items in the first argument
		argstream << "$" << std::setw(2) << std::setfill('0') << std::right << std::hex << zpArg << ", ";

		// Print the second arg
		// First check if it is a named label if so print the label
		// Otherwise print it as a normal offset
                if (mem_map && opcodeTakesLabels[opcode] == DL) {
                    Symbol sym;
                    // Check to see if the branch target is a named label
		    // Note that we only want the least significant byte of the argument
                    if(mem_map->FindAddress(address + (char) relArg, &sym)) {
                        string name = sym.name;
                        argstream << name;
                        break;
                    }
                }

		// Not a named label, print as a normal offset
		argstream << "$" << std::setw(2) << std::setfill('0') << std::right << std::hex << relArg;
                break;
	    }
            case XX:
                //Treat invalid opcode as no bytes
                break;
        }
    }

    ss << std::setw(ARG_PAD_LEN) << std::left << argstream.str();
}

vector<AsmLine> Disassembler::Decode(const std::function<uint8_t(uint16_t, bool)> mem_read, MemoryMap* mem_map, uint16_t address, size_t instruction_count) {
    vector<AsmLine>& output = lastDecode;
    output.clear();

    while(instruction_count--) {
        std::stringstream ss;
        uint8_t instructionBytes[MAX_INSTRUCTION_SIZE];

        if(mem_map) {
            Symbol sym;
            if(mem_map->FindAddress(address, &sym)) {
                AsmLine labelLine;
                labelLine.disassembledLine = sym.name + ':';
                labelLine.address = address;
                labelLine.isLabel = true;
                output.push_back(labelLine);
            }
        }
        uint8_t opcode = mem_read(address, false);
        instructionBytes[0] = opcode;
        string& opcodeName = opcodeNames[opcode];
        AddressMode mode = opcodeModes[opcode];
        size_t instructionSize = opBytes[mode];

        assert(instructionSize <= MAX_INSTRUCTION_SIZE);

        

        ss << "\t" << std::setw(MAX_INSTRUCTION_NAME_LEN + 1) << std::left << opcodeName;

        AsmLine line;
        line.address = address;
        line.num_args = instructionSize - 1;

        ++address;

        if(instructionSize == 1) {
            // We don't have any arguments for this instruction
            ss << std::setw(ARG_PAD_LEN) << "";
        } else {
            uint16_t args = mem_read(address++, false);
            instructionBytes[1] = args;

            if(instructionSize == 3) {
                uint8_t secondArg = mem_read(address++, false);
                args += secondArg << 8;
                instructionBytes[2] = secondArg;
            }
            FormatArgBytes(ss, mem_map, address, opcode, args);
            line.args = args;
        }

        for (size_t i = 0; i < instructionSize; i++) {
            // NOTE the cast to `uint32_t` is so that the number isn't printed as a char
            ss << ' ' << std::hex << std::setw(2) << std::setfill('0') << std::right << (uint32_t) instructionBytes[i];
        }

        
        line.disassembledLine = ss.str();
        line.isLabel = false;
        line.opcode = opcode;

        output.push_back(line);
    }

    return output;
}

vector<AsmLine> Disassembler::GetLastDecode() {
    return lastDecode;
}
