#pragma once
#include <cstdint>

const int RAMSIZE = 32768;
const int CARTRAMSIZE = 32768;
const int FRAME_BUFFER_SIZE = 16384;

#define VRAM_BUFFER_SIZE (FRAME_BUFFER_SIZE*2)
#define GRAM_BUFFER_SIZE (FRAME_BUFFER_SIZE*32)

struct SystemState {
    uint8_t dma_control;
    uint8_t banking;

    uint8_t ram[RAMSIZE];
    bool ram_initialized[RAMSIZE];

    uint8_t vram[VRAM_BUFFER_SIZE];
    uint8_t gram[GRAM_BUFFER_SIZE];
    
    uint8_t VIA_regs[16];
};

#define DMA_PARAMS_COUNT 8
const uint8_t DMA_PARAM_VX      = 0;
const uint8_t DMA_PARAM_VY      = 1;
const uint8_t DMA_PARAM_GX      = 2;
const uint8_t DMA_PARAM_GY      = 3;
const uint8_t DMA_PARAM_WIDTH   = 4;
const uint8_t DMA_PARAM_HEIGHT  = 5;
const uint8_t DMA_PARAM_TRIGGER = 6;
const uint8_t DMA_PARAM_COLOR   = 7;

struct BlitterState
{
    uint8_t params[DMA_PARAMS_COUNT];
    uint8_t gram_mid_bits;
};

struct CartridgeState
{
    int size = 8192;
    uint8_t *rom;
    uint8_t bank_shifter;
    uint32_t bank_mask;

    uint8_t save_ram[CARTRAMSIZE];
};