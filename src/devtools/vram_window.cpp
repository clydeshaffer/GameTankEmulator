#include "vram_window.h"
#include "../ui/ui_utils.h"

#define BUFFERS_PREVIEW_WIDTH 1152
#define BUFFERS_PREVIEW_HEIGHT 512

VRAMWindow::VRAMWindow(
    SDL_Surface* vram, SDL_Surface* gram, 
    SystemState* system, mos6502* cpu, 
    CartridgeState* cartridge) : BaseWindow(BUFFERS_PREVIEW_WIDTH, BUFFERS_PREVIEW_HEIGHT) {

    SDL_SetWindowTitle(window, "VRAM Viewer");
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

    // Clear the surface before re-rendering
    dest.x = 0;
    dest.y = 0;
    dest.w = 128*9;
    dest.h = 512;
    SDL_FillRect(surface, &dest, 0);

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

    int charsWritten = sprintf(
	buf,
	"DMA:\n%x\nBANK:\n%x\nPC:\n%x\nSTATUS:\n%x\nWAIT:%d\nHIBITS:%d\n\n",
	system_state->dma_control,
	system_state->banking,
	cpu_core->pc,
	cpu_core->status,
	cpu_core->waiting,
	cartridge_state->bank_mask
    );

    WriteDataUnderMouse(buf + charsWritten, 128 - charsWritten);
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

void VRAMWindow::WriteDataUnderMouse(char *buf, int bufSize) {
    int x, y;
    SDL_Window *focusedWindow;

    focusedWindow = SDL_GetMouseFocus();

    if (focusedWindow != window) {
	// The user is not currently hovering over the VRAM Viewer
	// There is no data under their mouse to be written
	return;
    }

    SDL_GetMouseState(&x, &y);

    // Which region of the VRAM viewer does the user's mouse lie in?
    if (x < 128 && y >= 256) {
	// The user is hovering over the area where debug information is printed
	// There are no relevant pixels to describe here
	return;
    }

    if (x < 128) {
	// The user is hovering over VRAM
	const int vram_start = 0;
	int addr = vram_start + (128*y) + x;

	snprintf(buf, bufSize - 1, "VRAM\nADDR: 0x%X\nVAL: 0x%02X", addr, system_state->vram[addr]);
    } else {
	// The user is hovering over GRAM
	x -= 128;
	const int gram_start = 0;
	int addr = gram_start + ((x >> 7) * 128 * 512) + (128*y) + (x % 128);

	snprintf(buf, bufSize - 1, "GRAM\nADDR: 0x%X\nVAL: 0x%02X", addr, system_state->gram[addr]);
    }
}
