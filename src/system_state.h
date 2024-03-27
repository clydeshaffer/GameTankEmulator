#pragma once
#include <cstdint>

const int RAMSIZE = 32768;
const int CARTRAMSIZE = 32768;
const int FRAME_BUFFER_SIZE = 16384;

#define VRAM_BUFFER_SIZE (FRAME_BUFFER_SIZE*2)
#define GRAM_BUFFER_SIZE (FRAME_BUFFER_SIZE*32)

#define BANK_GRAM_MASK  0b00000111
#define BANK_VRAM_MASK  0b00001000
#define BANK_WRAPX_MASK 0b00010000
#define BANK_WRAPY_MASK 0b00100000
#define BANK_RAM_MASK   0b11000000

enum RomType {
	UNKNOWN,
	EEPROM8K,
	EEPROM32K,
	FLASH2M,
	FLASH2M_RAM32K,
};

struct SystemState {
    uint8_t dma_control;
    uint8_t banking;

    uint8_t ram[RAMSIZE];
    bool ram_initialized[RAMSIZE];

    uint8_t vram[VRAM_BUFFER_SIZE];
    uint8_t gram[GRAM_BUFFER_SIZE];
    
    uint8_t VIA_regs[16];
};

struct CartridgeState
{
    int size = 8192;
    uint8_t *rom;
    uint8_t bank_shifter;
    uint32_t bank_mask;

    uint8_t save_ram[CARTRAMSIZE];
    bool write_mode;
};