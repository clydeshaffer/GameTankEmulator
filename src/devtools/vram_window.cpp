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

    // Render VRAM
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

    // Render GRAM
    for(int i = 0; i < 8; i++) {
	dest.x = (i+1) * 128;
	src.y = i * 512;
	SDL_BlitSurface(gRAM_Surface, &src, surface, &dest);
    }

    // Render text
    // Clear the text area before re-rendering as not all text is always rendered
    dest.x = 0;
    dest.y = 256;
    dest.w = 128;
    dest.h = 256;
    SDL_FillRect(surface, &dest, 0);

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
	int vram_start = 0x4000;
	int hovered_pixel = (128*y) + x;
	int addr = vram_start + (128*(y % 128)) + x;

	int framebuffer = y >> 7;

	snprintf(
	  buf,
	  bufSize - 1,
	  "VRAM\nFRAMEBUFFER: %d\nADDR: 0x%X\nVAL: 0x%02X",
	  framebuffer,
	  addr,
	  system_state->vram[hovered_pixel]
	);
    } else {
	// The user is hovering over GRAM
        // Correct for the fact that GRAM starts at position 128 in the viewer
	x -= 128;

	int hovered_pixel = ((x >> 7) * 128 * 512) + (128*y) + (x % 128);

	int gram_start = 0x4000;
	int quadrant = y >> 7;
	int line_in_quadrant = y % 128;
	int addr = gram_start + (128*line_in_quadrant) + (x % 128);

	const char *quadrant_names[4] = {
	  "NW",
	  "NE",
	  "SW",
	  "SE"
	};

	snprintf(
	  buf,
	  bufSize - 1,
	  "GRAM\nBANK: %d\nQUADRANT: %s\nADDR: 0x%X\nVAL: 0x%02X",
	  x >> 7,
	  quadrant_names[quadrant],
	  addr,
	  system_state->gram[hovered_pixel]
        );
    }
}
