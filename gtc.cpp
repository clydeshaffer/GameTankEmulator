#include <SDL.h>
#include <stdio.h>

#include "mos6502/mos6502.h"

using namespace std;

typedef struct RGB_Color {
	uint8_t r, g, b;
} RGB_Color;

typedef struct RGBA_Color {
	uint8_t r, g, b, a;
} RGBA_Color;

const int ROMSIZE = 8192;
const int RAMSIZE = 8192;
const int FRAME_BUFFER_SIZE = 16384;

const int GT_WIDTH = 128;
const int GT_HEIGHT = 128;

const int SCREEN_WIDTH = 512;
const int SCREEN_HEIGHT = 512;

RGB_Color palette[256];
uint8_t rom_buffer[ROMSIZE];
uint8_t ram_buffer[RAMSIZE];
uint8_t vram_buffer[FRAME_BUFFER_SIZE*2];
uint8_t gram_buffer[FRAME_BUFFER_SIZE*2];

/**
 *   1 - Enable copy controller
 *   2 - Video output page select
 *   4 - Enable VSync NMI signal
 *   8 - G.RAM page select (for CPU or Copy Ops)
 *  16 - V.RAM page select (for CPU or Copy Ops)
 *  32 - CPU-facing bank select (1 for V.RAM, 0 for G.RAM)
 *  64 - Enable IRQ signal when copy op completes
 * 128 - Enable transparency keying on 0x00 pixels
 */
const uint8_t DMA_COPY_ENABLE_BIT = 1;
const uint8_t DMA_VID_OUT_PAGE_BIT = 2;
const uint8_t DMA_VSYNC_NMI_BIT = 4;
const uint8_t DMA_G_PAGE_SELECT_BIT = 8;
const uint8_t DMA_V_PAGE_SELECT_BIT = 16;
const uint8_t DMA_BANK_SELECT_BIT = 32;
const uint8_t DMA_COPY_IRQ_BIT = 64;
const uint8_t DMA_TRANSPARENCY_BIT = 128;
uint8_t dma_control_reg = 0;

const uint8_t DMA_PARAM_VX      = 0;
const uint8_t DMA_PARAM_VY      = 1;
const uint8_t DMA_PARAM_GX      = 2;
const uint8_t DMA_PARAM_GY      = 3;
const uint8_t DMA_PARAM_WIDTH   = 4;
const uint8_t DMA_PARAM_HEIGHT  = 5;
const uint8_t DMA_PARAM_TRIGGER = 6;
const uint8_t DMA_PARAM_COLOR   = 7;
uint8_t dma_params[8];

SDL_Surface* screenSurface = NULL;
SDL_Surface* gRAM_Surface = NULL;
SDL_Surface* vRAM_Surface = NULL;

mos6502 *cpu_core;

uint8_t open_bus() {
	return 0;
}

Uint32 convert_color(SDL_Surface* target, uint8_t cIndex) {
	if(cIndex == 0) return SDL_MapRGB(target->format, 0, 0, 0);
	RGB_Color c = palette[cIndex];
	Uint32 res = SDL_MapRGB(target->format, c.r, c.g, c.b);
	if(res == SDL_MapRGB(target->format, 0, 0, 0))
		return SDL_MapRGB(target->format, 1, 1, 1);
	return res;
}

Uint32 get_pixel32( SDL_Surface *surface, int x, int y )
{
    //Convert the pixels to 32 bit
    Uint32 *pixels = (Uint32 *)surface->pixels;
    
    //Get the requested pixel
    return pixels[ ( y * surface->w ) + x ];
}

void put_pixel32( SDL_Surface *surface, int x, int y, Uint32 pixel )
{
    //Convert the pixels to 32 bit
    Uint32 *pixels = (Uint32 *)surface->pixels;
    
    //Set the pixel
    pixels[ ( y * surface->w ) + x ] = pixel;
}

void refreshScreen() {
	SDL_Rect src, dest;
	src.x = 0;
	src.y = (dma_control_reg & DMA_VID_OUT_PAGE_BIT) ? GT_HEIGHT : 0;
	src.w = GT_WIDTH;
	src.h = GT_HEIGHT;
	dest.x = 0;
	dest.y = 0;
	dest.w = SCREEN_WIDTH;
	dest.h = SCREEN_HEIGHT;
	SDL_BlitScaled(vRAM_Surface, &src, screenSurface, &dest);
}

uint8_t VDMA_Read(uint16_t address) {
	if(dma_control_reg & DMA_COPY_ENABLE_BIT) {
		return open_bus();
	} else {
		uint8_t* bufPtr;
		uint16_t offset = 0;
		if(dma_control_reg & DMA_BANK_SELECT_BIT) {
			bufPtr = vram_buffer;
			if(dma_control_reg & DMA_V_PAGE_SELECT_BIT) {
				offset = 0x4000;
			}
		} else {
			bufPtr = gram_buffer;
			if(dma_control_reg & DMA_G_PAGE_SELECT_BIT) {
				offset = 0x4000;
			}
		}
		return bufPtr[(address & 0x3FFF) | offset];
	}
}

void VDMA_Write(uint16_t address, uint8_t value) {
	if(dma_control_reg & DMA_COPY_ENABLE_BIT) {
		if(((address & 0xF) == 6) && (value == 1)) {
			SDL_Rect gRect, vRect;
			vRect.x = dma_params[DMA_PARAM_VX] & 0x7F;
			vRect.y = dma_params[DMA_PARAM_VY] & 0x7F;
			vRect.w = dma_params[DMA_PARAM_WIDTH] + 1;
			vRect.h = dma_params[DMA_PARAM_HEIGHT];
			gRect.x = dma_params[DMA_PARAM_GX] & 0x7F;
			gRect.y = dma_params[DMA_PARAM_GY] & 0x7F;
			gRect.w = dma_params[DMA_PARAM_WIDTH] + 1;
			gRect.h = dma_params[DMA_PARAM_HEIGHT];
			uint8_t outColor[2];
			uint8_t colorSel = 0;
			if(dma_params[DMA_PARAM_GX] & 0x80) {
				colorSel = 1;
			}
			outColor[1] = ~(dma_params[DMA_PARAM_COLOR]);
			printf("Copying from (%d, %d) to (%d, %d) at (%d x %d)\n",
				gRect.x, gRect.y,
				vRect.x, vRect.y,
				gRect.w, gRect.h);
			uint16_t vOffset = 0, gOffset = 0;
			if(dma_control_reg & DMA_V_PAGE_SELECT_BIT) {
				vOffset = 0x4000;
				vRect.y += GT_HEIGHT;
			}
			if(dma_control_reg & DMA_G_PAGE_SELECT_BIT) {
				gOffset = 0x4000;
				gRect.y += GT_HEIGHT;
			}
			int vy = dma_params[DMA_PARAM_VY],
				gy = dma_params[DMA_PARAM_GY];
			for(uint16_t y = 0; y < dma_params[DMA_PARAM_HEIGHT]; y++) {
				int vx = dma_params[DMA_PARAM_VX],
					gx = dma_params[DMA_PARAM_GX];
				for(uint16_t x = 0; x < dma_params[DMA_PARAM_WIDTH]; x++) {
					outColor[0] = gram_buffer[(gy << 7) | gx | gOffset];
					vram_buffer[(vy << 7) | vx | vOffset] = outColor[colorSel];
					vx++;
					gx++;
				}
				vy++;
				gy++;
			}
			if(colorSel == 1) {
				SDL_FillRect(vRAM_Surface, &vRect,  convert_color(vRAM_Surface, outColor[1]));
			} else {
				SDL_BlitSurface(gRAM_Surface, &gRect, vRAM_Surface, &vRect);
			}

			if(dma_control_reg & DMA_COPY_IRQ_BIT) {
				cpu_core->IRQ();
			}
		} else {
			printf("Setting DMA param %d to %d\n", address & 0xF, value);
			dma_params[address & 0xF] = value;
		}
	} else {
		uint8_t* bufPtr;
		uint16_t offset = 0;
		SDL_Surface* targetSurface = screenSurface;
		int yShift = 0;
		if(dma_control_reg & DMA_BANK_SELECT_BIT) {
			bufPtr = vram_buffer;
			targetSurface = vRAM_Surface;
			if(dma_control_reg & DMA_V_PAGE_SELECT_BIT) {
				offset = 0x4000;
				yShift = GT_HEIGHT;
			}
		} else {
			bufPtr = gram_buffer;
			targetSurface = gRAM_Surface;
			if(dma_control_reg & DMA_G_PAGE_SELECT_BIT) {
				offset = 0x4000;
				yShift = GT_HEIGHT;
			}
		}
		bufPtr[(address & 0x3FFF) | offset] = value;

		uint8_t x, y;
		x = address & 127;
		y = (address >> 7) & 127;
		put_pixel32(targetSurface, x, y + yShift, convert_color(targetSurface, value));
	}
}

uint8_t MemoryRead(uint16_t address) {
	if(address & 0x8000) {
		if(rom_buffer[address & 0x1FFF] == 0x4C) {
		}
		return rom_buffer[address & 0x1FFF];
	} else if(address & 0x4000) {
		return VDMA_Read(address);
	} else if(address < 0x2000) {
		return ram_buffer[address & 0x1FFF];
	}
	return open_bus();
}

void MemoryWrite(uint16_t address, uint8_t value) {
	if(address & 0x8000) {

	}
	else if(address & 0x4000) {
		VDMA_Write(address, value);
	}
	else if(address & 0x2000) {
		if((address & 0x000F) == 0x0007) {
			dma_control_reg = value;
			if(dma_control_reg & DMA_TRANSPARENCY_BIT) {
				SDL_SetColorKey(gRAM_Surface, SDL_TRUE, SDL_MapRGB(gRAM_Surface->format, 0, 0, 0));
			} else {
				SDL_SetColorKey(gRAM_Surface, SDL_FALSE, 0);
			}
		}
	}
	else if(address < 0x2000) {
		ram_buffer[address & 0x1FFF] = value;
	}
}

SDL_Event e;
bool running = true;

void CPUStopped() {
	running = false;
}

int main(int argC, char* argV[]) {
	for(int i = 0; i < ROMSIZE; i++) {
		rom_buffer[i] = 0;
	}

	FILE* palFileP = fopen("GMETNK2.act", "rb");
	if(palFileP) {
		fread(palette, sizeof(RGB_Color), 256, palFileP);
		fclose(palFileP);
	}

	if(argC > 1) {
		FILE* romFileP = fopen(argV[1], "rb");
		if(romFileP) {
			fread(rom_buffer, sizeof(uint8_t), ROMSIZE, romFileP);
			fclose(romFileP);
		}
	}

	cpu_core = new mos6502(MemoryRead, MemoryWrite, CPUStopped);
	cpu_core->Reset();

	SDL_Window* window = NULL;

	SDL_Init(SDL_INIT_VIDEO);
	atexit(SDL_Quit);
	window = SDL_CreateWindow( "GameTank Emulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN );
	screenSurface = SDL_GetWindowSurface(window);
	SDL_SetColorKey(screenSurface, SDL_FALSE, 0);

	Uint32 rmask, gmask, bmask, amask;
	#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	    rmask = 0xff000000;
	    gmask = 0x00ff0000;
	    bmask = 0x0000ff00;
	    amask = 0x000000ff;
	#else
	    rmask = 0x000000ff;
	    gmask = 0x0000ff00;
	    bmask = 0x00ff0000;
	    amask = 0xff000000;
	#endif

	vRAM_Surface = SDL_CreateRGBSurface(0, GT_WIDTH, GT_HEIGHT * 2, 32, rmask, gmask, bmask, amask);
	gRAM_Surface = SDL_CreateRGBSurface(0, GT_WIDTH, GT_HEIGHT * 2, 32, rmask, gmask, bmask, amask);

	SDL_SetColorKey(vRAM_Surface, SDL_FALSE, 0);
	SDL_SetColorKey(gRAM_Surface, SDL_FALSE, 0);

	SDL_FillRect(gRAM_Surface, NULL, SDL_MapRGB(gRAM_Surface->format, 0, 0, 0));
	SDL_FillRect(vRAM_Surface, NULL, SDL_MapRGB(vRAM_Surface->format, 0, 0, 0));

	SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 0x00, 0x00, 0x00));

	uint64_t actual_cycles = 0;
	uint64_t cycles_since_vsync = 0;
	uint64_t cycles_per_vsync = 233333;
	uint64_t target_runtime = 60;
	uint64_t target_cycles = target_runtime * 14000000;
	int zeroConsec = 0;
	while(running) {
		actual_cycles = 0;
		cpu_core->Run(233333, actual_cycles);
		if(actual_cycles == 0) {
			zeroConsec++;
			if(zeroConsec == 10) {
				printf("(Got stuck!)\n");
				break;
			}
		} else {
			zeroConsec = 0;
		}
		SDL_Delay(actual_cycles/140000);
		cycles_since_vsync += actual_cycles;
		if(cycles_since_vsync >= cycles_per_vsync) {
			cycles_since_vsync -= cycles_per_vsync;

		}
		cpu_core->NMI();
		refreshScreen();
		SDL_UpdateWindowSurface(window);

		while( SDL_PollEvent( &e ) != 0 )
        {
            //User requests quit
            if( e.type == SDL_QUIT )
            {
               running = false;
            }
        }
		
	}
	printf("Finished running\n");
	


	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}