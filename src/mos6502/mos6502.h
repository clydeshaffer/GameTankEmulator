//============================================================================
// Name        : mos6502
// Author      : Gianluca Ghettini
// Version     : 1.0
// Copyright   :
// Description : A MOS 6502 CPU emulator written in C++
//============================================================================
#pragma once
#include <iostream>
#include <stdint.h>
using namespace std;

#define NEGATIVE  0x80
#define OVERFLOW  0x40
#define CONSTANT  0x20
#define BREAK     0x10
#define DECIMAL   0x08
#define INTERRUPT 0x04
#define ZERO      0x02
#define CARRY     0x01

#define SET_NEGATIVE(x) (x ? (status |= NEGATIVE) : (status &= (~NEGATIVE)) )
#define SET_OVERFLOW(x) (x ? (status |= OVERFLOW) : (status &= (~OVERFLOW)) )
#define SET_CONSTANT(x) (x ? (status |= CONSTANT) : (status &= (~CONSTANT)) )
#define SET_BREAK(x) (x ? (status |= BREAK) : (status &= (~BREAK)) )
#define SET_DECIMAL(x) (x ? (status |= DECIMAL) : (status &= (~DECIMAL)) )
#define SET_INTERRUPT(x) (x ? (status |= INTERRUPT) : (status &= (~INTERRUPT)) )
#define SET_ZERO(x) (x ? (status |= ZERO) : (status &= (~ZERO)) )
#define SET_CARRY(x) (x ? (status |= CARRY) : (status &= (~CARRY)) )

#define IF_NEGATIVE() ((status & NEGATIVE) ? true : false)
#define IF_OVERFLOW() ((status & OVERFLOW) ? true : false)
#define IF_CONSTANT() ((status & CONSTANT) ? true : false)
#define IF_BREAK() ((status & BREAK) ? true : false)
#define IF_DECIMAL() ((status & DECIMAL) ? true : false)
#define IF_INTERRUPT() ((status & INTERRUPT) ? true : false)
#define IF_ZERO() ((status & ZERO) ? true : false)
#define IF_CARRY() ((status & CARRY) ? true : false)



class mos6502
{
private:
	struct AddrRes
	{
		uint8_t cycles;
		uint16_t addr;
	};

	typedef uint8_t (mos6502::*CodeExec)(uint16_t);
	typedef AddrRes (mos6502::*AddrExec)();

	struct Instr
	{
		AddrExec addr;
		CodeExec code;
	};

	Instr InstrTable[256];

	uint8_t Exec(Instr i);

	// addressing modes
	AddrRes Addr_ACC(); // ACCUMULATOR
	AddrRes Addr_IMM(); // IMMEDIATE
	AddrRes Addr_ABS(); // ABSOLUTE
	AddrRes Addr_ZER(); // ZERO PAGE
	AddrRes Addr_ZEX(); // INDEXED-X ZERO PAGE
	AddrRes Addr_ZEY(); // INDEXED-Y ZERO PAGE
	AddrRes Addr_ABX(); // INDEXED-X ABSOLUTE
	AddrRes Addr_ABY(); // INDEXED-Y ABSOLUTE
	AddrRes Addr_IMP(); // IMPLIED
	AddrRes Addr_REL(); // RELATIVE
	AddrRes Addr_INX(); // INDEXED-X INDIRECT
	AddrRes Addr_INY(); // INDEXED-Y INDIRECT
	AddrRes Addr_ABI(); // ABSOLUTE INDIRECT
	AddrRes Addr_ZPI(); // ZERO PAGE INDIRECT
	AddrRes Addr_AXI(); // INDEXED-X ABSOLUTE INDIRECT

	// opcodes (grouped as per datasheet)
	uint8_t Op_ADC(uint16_t src);
	uint8_t Op_AND(uint16_t src);
	uint8_t Op_ASL(uint16_t src); 	uint8_t Op_ASL_ACC(uint16_t src);
	uint8_t Op_BCC(uint16_t src);
	uint8_t Op_BCS(uint16_t src);

	uint8_t Op_BEQ(uint16_t src);
	uint8_t Op_BIT(uint16_t src);
	uint8_t Op_BMI(uint16_t src);
	uint8_t Op_BNE(uint16_t src);
	uint8_t Op_BPL(uint16_t src);

	uint8_t Op_BRK(uint16_t src);
	uint8_t Op_BVC(uint16_t src);
	uint8_t Op_BVS(uint16_t src);
	uint8_t Op_CLC(uint16_t src);
	uint8_t Op_CLD(uint16_t src);

	uint8_t Op_CLI(uint16_t src);
	uint8_t Op_CLV(uint16_t src);
	uint8_t Op_CMP(uint16_t src);
	uint8_t Op_CPX(uint16_t src);
	uint8_t Op_CPY(uint16_t src);

	uint8_t Op_DEC(uint16_t src);	uint8_t Op_DEC_ACC(uint16_t src);
	uint8_t Op_DEX(uint16_t src);
	uint8_t Op_DEY(uint16_t src);
	uint8_t Op_EOR(uint16_t src);
	uint8_t Op_INC(uint16_t src);	uint8_t Op_INC_ACC(uint16_t src);

	uint8_t Op_INX(uint16_t src);
	uint8_t Op_INY(uint16_t src);
	uint8_t Op_JMP(uint16_t src);
	uint8_t Op_JSR(uint16_t src);
	uint8_t Op_LDA(uint16_t src);

	uint8_t Op_LDX(uint16_t src);
	uint8_t Op_LDY(uint16_t src);
	uint8_t Op_LSR(uint16_t src); 	uint8_t Op_LSR_ACC(uint16_t src);
	uint8_t Op_NOP(uint16_t src);
	uint8_t Op_ORA(uint16_t src);

	uint8_t Op_PHA(uint16_t src);
	uint8_t Op_PHP(uint16_t src);
	uint8_t Op_PHX(uint16_t src);
	uint8_t Op_PHY(uint16_t src);
	uint8_t Op_PLA(uint16_t src);
	uint8_t Op_PLP(uint16_t src);
	uint8_t Op_PLX(uint16_t src);
	uint8_t Op_PLY(uint16_t src);
	uint8_t Op_ROL(uint16_t src); 	uint8_t Op_ROL_ACC(uint16_t src);

	uint8_t Op_ROR(uint16_t src);	uint8_t Op_ROR_ACC(uint16_t src);
	uint8_t Op_RTI(uint16_t src);
	uint8_t Op_RTS(uint16_t src);
	uint8_t Op_SBC(uint16_t src);
	uint8_t Op_SEC(uint16_t src);
	uint8_t Op_SED(uint16_t src);

	uint8_t Op_SEI(uint16_t src);
	uint8_t Op_STA(uint16_t src);
	uint8_t Op_STZ(uint16_t src);
	uint8_t Op_STX(uint16_t src);
	uint8_t Op_STY(uint16_t src);
	uint8_t Op_TAX(uint16_t src);

	uint8_t Op_TAY(uint16_t src);
	uint8_t Op_TSX(uint16_t src);
	uint8_t Op_TXA(uint16_t src);
	uint8_t Op_TXS(uint16_t src);
	uint8_t Op_TYA(uint16_t src);

	uint8_t Op_WAI(uint16_t src);
	uint8_t Op_STP(uint16_t src);
	uint8_t Op_BRA(uint16_t src);
	uint8_t Op_TRB(uint16_t src);
	uint8_t Op_TSB(uint16_t src);

	uint8_t Op_ILLEGAL(uint16_t src);

	// IRQ, reset, NMI vectors
	static const uint16_t irqVectorH = 0xFFFF;
	static const uint16_t irqVectorL = 0xFFFE;
	static const uint16_t rstVectorH = 0xFFFD;
	static const uint16_t rstVectorL = 0xFFFC;
	static const uint16_t nmiVectorH = 0xFFFB;
	static const uint16_t nmiVectorL = 0xFFFA;

	// read/write callbacks
	typedef void (*CPUEvent)(void);
	typedef void (*BusWrite)(uint16_t, uint8_t);
	typedef uint8_t (*BusRead)(uint16_t);
	BusRead Read;
	BusWrite Write;
	CPUEvent Stopped;
	BusRead Sync;

	// stack operations
	inline void StackPush(uint8_t byte);
	inline uint8_t StackPop();

	uint32_t irq_timer;
	bool irq_line = false;

	//Specific hack for the GameTank's Blit IRQ enable
	//If not null, this is checked before actually sending IRQ
	bool *irq_gate;

public:
	bool freeze = false;
	bool illegalOpcode = false;
	bool waiting;
	uint16_t illegalOpcodeSrc;

	// registers
	uint8_t A; // accumulator
	uint8_t X; // X-index
	uint8_t Y; // Y-index

	// stack pointer
	uint8_t sp;

	// program counter
	uint16_t pc;

	// status register
	uint8_t status;
	
	enum CycleMethod {
		INST_COUNT,
		CYCLE_COUNT,
	};
	mos6502(BusRead r, BusWrite w, CPUEvent stp, BusRead sync = NULL);
	void NMI();
	void IRQ();
	void ScheduleIRQ(uint32_t cycles, bool *gate);
	void ClearIRQ();
	void Reset();
	void Run(
		int32_t cycles,
		uint64_t& cycleCount,
		CycleMethod cycleMethod = CYCLE_COUNT);
	void Freeze();
};
