#pragma once
#include "base_window.h"
#include "../system_state.h"
#include "../mos6502/mos6502.h"

class VRAMWindow : public BaseWindow {
private:
    SDL_Surface* surface;
    SDL_Surface* vRAM_Surface;
    SDL_Surface* gRAM_Surface;
    SystemState* system_state;
    mos6502* cpu_core;
    CartridgeState* cartridge_state;
public:
    VRAMWindow(SDL_Surface* vram, SDL_Surface* gram, SystemState* system, mos6502* cpu, CartridgeState* cartridge);
    void Draw();
    void HandleEvent(SDL_Event& e);
};