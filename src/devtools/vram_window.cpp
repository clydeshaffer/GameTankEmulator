#include "vram_window.h"
#include "../ui/ui_utils.h"

#define BUFFERS_PREVIEW_WIDTH 1152
#define BUFFERS_PREVIEW_HEIGHT 512

VRAMWindow::VRAMWindow(
    SDL_Surface* vram, SDL_Surface* gram, 
    SystemState* system, mos6502* cpu, 
    CartridgeState* cartridge) : BaseWindow(BUFFERS_PREVIEW_WIDTH, BUFFERS_PREVIEW_HEIGHT) {
    
    surface = SDL_GetWindowSurface(window);
    SDL_SetColorKey(surface, SDL_FALSE, 0);

    vRAM_Surface = vram;
    gRAM_Surface = gram;
    system_state = system;
    cpu_core = cpu;
    cartridge_state = cartridge;

}

void VRAMWindow::Draw() {
    SDL_Rect src, dest;
	dest.x = 0;
	dest.y = 0;
	dest.w = 128;
	dest.h = 256;
	src.x = 0;
	src.y = 0;
	src.w = 128;
	src.h = 256;
	SDL_BlitSurface(vRAM_Surface, &src, surface, &dest);
	src.h = 512;
	dest.h = 512;
	for(int i = 0; i < 8; i++) {
		dest.x = (i+1) * 128;
		src.y = i * 512;
		SDL_BlitSurface(gRAM_Surface, &src, surface, &dest);
	}

	char buf[128];
	sprintf(buf, "DMA:\n%x\nBANK:\n%x\nPC:\n%x\nSTATUS:\n%x\nWAIT:%d\nHIBITS:%d",
        system_state->dma_control, system_state->banking, cpu_core->pc, cpu_core->status,
        cpu_core->waiting, cartridge_state->bank_mask);
	dest.x = 0;
	dest.y = 256;
	dest.w = 128;
	dest.h = 512-128;
	drawText(surface, &dest, buf);

    SDL_UpdateWindowSurface(window);
}

void VRAMWindow::HandleEvent(SDL_Event& e) {
    if(e.type == SDL_WINDOWEVENT) {
        if(e.window.event == SDL_WINDOWEVENT_CLOSE) {
            SDL_Window* closedWindow = SDL_GetWindowFromID(e.window.windowID);
            if(closedWindow == window) {
                open = false;
            }
        }
    }
}