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

const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 128;

RGB_Color palette[256];
uint8_t rom_buffer[ROMSIZE];
uint8_t ram_buffer[RAMSIZE];
uint8_t frame_buffer[FRAME_BUFFER_SIZE];

uint8_t dma_control_reg = 0;

SDL_Surface* screenSurface = NULL;

Uint32 convert_color(RGB_Color c) {
	return SDL_MapRGB(screenSurface->format, c.r, c.g, c.b);
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

uint8_t MemoryRead(uint16_t address) {
	if(address & 0x8000) {
		return rom_buffer[address & 0x1FFF];
	} else if(address & 0x4000) {
		return frame_buffer[address & 0x3FFF];
	} else if(address < 0x2000) {
		return ram_buffer[address & 0x1FFF];
	}
	return 0;
}

void MemoryWrite(uint16_t address, uint8_t value) {
	if(address & 0x4000) {
		uint8_t x, y;
		x = address & 127;
		y = (address >> 7) & 127;
		put_pixel32(screenSurface, x, y, convert_color(palette[value]));
		frame_buffer[address & 0x3FFF] = value;
	}
	else if(address < 0x2000) {
		ram_buffer[address & 0x1FFF] = value;
	}
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

	mos6502 *cpu_core = new mos6502(MemoryRead, MemoryWrite);
	cpu_core->Reset();

	SDL_Window* window = NULL;

	SDL_Init(SDL_INIT_VIDEO);
	atexit(SDL_Quit);
	window = SDL_CreateWindow( "GameTank Emulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN );
	screenSurface = SDL_GetWindowSurface(window);

	SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 0x00, 0x00, 0x00));

	uint64_t actual_cycles;
	cpu_core->Run(14000000, actual_cycles);

	SDL_UpdateWindowSurface(window);
	

	SDL_Delay(2000);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}