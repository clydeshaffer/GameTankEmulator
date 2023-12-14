#include <cstdint>
#include "timekeeper.h"
#include "system_state.h"
#include "SDL_inc.h"
#include "mos6502/mos6502.h"
#include "palette.h"

#define DMA_PARAMS_COUNT 8

#define DMA_COPY_ENABLE_BIT 1
#define DMA_VID_OUT_PAGE_BIT 2
#define DMA_VSYNC_NMI_BIT 4
#define DMA_COLORFILL_ENABLE_BIT 8
#define DMA_GCARRY_BIT 16
#define DMA_CPU_TO_VRAM 32
#define DMA_COPY_IRQ_BIT 64
#define DMA_TRANSPARENCY_BIT 128

Uint32 get_pixel32( SDL_Surface *surface, int x, int y );
void put_pixel32( SDL_Surface *surface, int x, int y, Uint32 pixel );

class Blitter {
private:
    mos6502*& cpu_core;
    Timekeeper* timekeeper;
    SystemState* system_state;
    SDL_Surface*& vram_surface;

    uint8_t counterVX;
    uint8_t counterVY;
    uint8_t counterGX;
    uint8_t counterGY;
    uint8_t counterW;
    uint8_t counterH;

    uint8_t params[DMA_PARAMS_COUNT];

    bool trigger;
    bool init;
    bool irq;
    bool running;

    uint64_t last_updated_cycle = 0;

public:
    static const uint8_t PARAM_VX      = 0;
    static const uint8_t PARAM_VY      = 1;
    static const uint8_t PARAM_GX      = 2;
    static const uint8_t PARAM_GY      = 3;
    static const uint8_t PARAM_WIDTH   = 4;
    static const uint8_t PARAM_HEIGHT  = 5;
    static const uint8_t PARAM_TRIGGER = 6;
    static const uint8_t PARAM_COLOR   = 7;

    
    uint8_t gram_mid_bits;

    Blitter(mos6502*& cpu_core, Timekeeper* timekeeper, SystemState* system_state, SDL_Surface*& vram_surface) : cpu_core(cpu_core), timekeeper(timekeeper), system_state(system_state), vram_surface(vram_surface) {};

    void SetParam(uint8_t address, uint8_t value);
    void CatchUp();
};