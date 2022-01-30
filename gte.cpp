#include "SDL_inc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cmath>
#include <time.h>

#ifdef WASM_BUILD
#include "emscripten.h"
#include <emscripten/html5.h>
#else
#include "tinyfd/tinyfiledialogs.h"
#endif

#include "joystick_adapter.h"
#include "audio_coprocessor.h"
#include "gametank_palette.h"

#include "mos6502/mos6502.h"

using namespace std;

typedef struct RGB_Color {
	uint8_t r, g, b;
} RGB_Color;

typedef struct RGBA_Color {
	uint8_t r, g, b, a;
} RGBA_Color;

int ROMSIZE = 8192;
const int RAMSIZE = 32768;
const int FRAME_BUFFER_SIZE = 16384;

const int GT_WIDTH = 128;
const int GT_HEIGHT = 128;

enum RomType {
	UNKNOWN,
	EEPROM8K,
	EEPROM32K,
	FLASH2M,	
};

uint8_t flash2M_highbits_shifter;
uint32_t flash2M_highbits;
RomType loadedRomType;

const int SCREEN_WIDTH = 512;
const int SCREEN_HEIGHT = 512;
const int PROFILER_WIDTH = 256;
const int PROFILER_HEIGHT = 512;
RGB_Color *palette;
uint8_t *rom_buffer;
uint8_t ram_buffer[RAMSIZE];
bool ram_inited[RAMSIZE];

#define VRAM_BUFFER_SIZE (FRAME_BUFFER_SIZE*2)
#define GRAM_BUFFER_SIZE (FRAME_BUFFER_SIZE*32)

uint8_t vram_buffer[VRAM_BUFFER_SIZE];
uint8_t gram_buffer[GRAM_BUFFER_SIZE];

uint8_t VIA_regs[16];
const uint8_t VIA_ORB    = 0x0;
const uint8_t VIA_ORA    = 0x1;
const uint8_t VIA_DDRB   = 0x2;
const uint8_t VIA_DDRA   = 0x3;
const uint8_t VIA_T1CL   = 0x4;
const uint8_t VIA_T1CH   = 0x5;
const uint8_t VIA_T1LL   = 0x6;
const uint8_t VIA_T1LH   = 0x7;
const uint8_t VIA_T2CL   = 0x8;
const uint8_t VIA_T2CH   = 0x9;
const uint8_t VIA_SR     = 0xA;
const uint8_t VIA_ACR    = 0xB;
const uint8_t VIA_PCR    = 0xC;
const uint8_t VIA_IFR    = 0xD;
const uint8_t VIA_IER    = 0xE;
const uint8_t VIA_ORA_NH = 0xF;

//Pins of VIA Port A used for Serial comms (or other misc cartridge use)
const uint8_t VIA_SPI_BIT_CLK  = 0b00000001;
const uint8_t VIA_SPI_BIT_MOSI = 0b00000010;
const uint8_t VIA_SPI_BIT_CS   = 0b00000100;
const uint8_t VIA_SPI_BIT_MISO = 0b10000000;

#define DMA_COPY_ENABLE_BIT 1
#define DMA_VID_OUT_PAGE_BIT 2
#define DMA_VSYNC_NMI_BIT 4
#define DMA_COLORFILL_ENABLE_BIT 8
#define DMA_GCARRY_BIT 16
#define DMA_CPU_TO_VRAM 32
#define DMA_COPY_IRQ_BIT 64
#define DMA_TRANSPARENCY_BIT 128
uint8_t dma_control_reg = 0;

#define BANK_GRAM_MASK  0b00000111
#define BANK_VRAM_MASK  0b00001000
#define BANK_WRAPX_MASK 0b00010000
#define BANK_WRAPY_MASK 0b00100000
#define BANK_RAM_MASK   0b11000000
#define RAM_HIGHBITS_SHIFT 7

uint8_t banking_reg = 0;
uint8_t gram_mid_bits = 0;

#define FULL_RAM_ADDRESS(x) (((banking_reg & BANK_RAM_MASK) << RAM_HIGHBITS_SHIFT) | (x))

#define DMA_PARAMS_COUNT 8
const uint8_t DMA_PARAM_VX      = 0;
const uint8_t DMA_PARAM_VY      = 1;
const uint8_t DMA_PARAM_GX      = 2;
const uint8_t DMA_PARAM_GY      = 3;
const uint8_t DMA_PARAM_WIDTH   = 4;
const uint8_t DMA_PARAM_HEIGHT  = 5;
const uint8_t DMA_PARAM_TRIGGER = 6;
const uint8_t DMA_PARAM_COLOR   = 7;
uint8_t dma_params[DMA_PARAMS_COUNT];

SDL_Surface* bmpFont = NULL;
SDL_Surface* screenSurface = NULL;
SDL_Surface* profilerSurface = NULL;
SDL_Surface* gRAM_Surface = NULL;
SDL_Surface* vRAM_Surface = NULL;

mos6502 *cpu_core;
AudioCoprocessor *soundcard;
JoystickAdapter *joysticks;

#define PROFILER_ENTRIES 64
uint8_t bufferFlipCount = 0;
uint64_t totalCyclesCount = 0;
uint64_t profilingTimeStamps[PROFILER_ENTRIES];
uint64_t profilingTimes[PROFILER_ENTRIES];
int fps = 60;

#define BMP_CHAR_SIZE 8

#define INITIAL_TIME_SCALING 1000
#define INITIAL_SCALING_INCREMENT 100

SDL_Window* window = NULL;
SDL_Window* profiler_window = NULL;
Uint32 rmask, gmask, bmask, amask;
uint64_t system_clock = 315000000/88;
uint64_t actual_cycles = 0;
uint64_t cycles_since_vsync = 0;
uint64_t cycles_per_vsync = system_clock / 60;
double time_scaling = INITIAL_TIME_SCALING;
double scaling_increment = INITIAL_SCALING_INCREMENT;
uint32_t lastTicks = 0, currentTicks;
uint8_t frameCount = 0;
bool prev_overlong = false;
int zeroConsec = 0;
bool isFullScreen = false;

void drawText(SDL_Surface* surface, SDL_Rect* area, char* text) {
	SDL_Rect cursor_rect, char_rect;
	cursor_rect.x = area->x;
	cursor_rect.y = area->y;
	cursor_rect.w = BMP_CHAR_SIZE;
	cursor_rect.h = BMP_CHAR_SIZE;

	char_rect.w = BMP_CHAR_SIZE;
	char_rect.h = BMP_CHAR_SIZE;

	while((*text != 0) && (cursor_rect.y < (area->y + area->h))) {
		if(*text == '\n' || ((cursor_rect.x + cursor_rect.w) > area->w)) {
			cursor_rect.x = area->x;
			cursor_rect.y += BMP_CHAR_SIZE;
		} else {
			char_rect.x = ((*text) & 0x0F) << 3;
			char_rect.y = ((*text) & 0xF0) >> 1;
			SDL_BlitSurface(bmpFont, &char_rect, profilerSurface, &cursor_rect);
			cursor_rect.x += BMP_CHAR_SIZE;
		}
		++text;
	}
}

bool profiler_open = false;
int profiler_x_axis = 0;

uint8_t prof_R[8] = {255, 255, 255, 0, 0, 128, 128, 255};
uint8_t prof_G[8] = {0, 128, 255, 255, 0, 0, 128, 255};
uint8_t prof_B[8] = {0, 0, 0, 0, 255, 128, 128, 255};


void drawProfilingWindow() {
	SDL_Rect profilerArea, graphRect;
	profilerArea.x = 0;
	profilerArea.y = 0;
	profilerArea.w = PROFILER_WIDTH;
	profilerArea.h = BMP_CHAR_SIZE;

	graphRect.x = profiler_x_axis;
	graphRect.y = 0;
	graphRect.w = 1;
	graphRect.h = PROFILER_HEIGHT;

	char buf[64];

	SDL_FillRect(profilerSurface, &graphRect, SDL_MapRGB(profilerSurface->format, 0, 0, 0));
	graphRect.h = 1;
	graphRect.y = PROFILER_HEIGHT - 128;
	SDL_FillRect(profilerSurface, &graphRect, SDL_MapRGB(profilerSurface->format, 32, 32, 32));
	
	sprintf(buf,"FPS: %d\n", fps);
	SDL_FillRect(profilerSurface, &profilerArea, SDL_MapRGB(profilerSurface->format, 0, 0, 0));
	drawText(profilerSurface, &profilerArea, buf);
	profilerArea.y += BMP_CHAR_SIZE;

	for(int i = 0; i < PROFILER_ENTRIES; ++i) {
		Uint32 id_col = SDL_MapRGB(profilerSurface->format, prof_R[i % 8], prof_G[i % 8], prof_B[i % 8]);
		if(profilingTimes[i] != 0) {
			sprintf(buf,"Timer %d: %llu\n", i, profilingTimes[i]);
			profilerArea.x = 0;
			profilerArea.w = 2;
			SDL_FillRect(profilerSurface, &profilerArea, id_col);
			profilerArea.x = 4;
			profilerArea.w = PROFILER_WIDTH-4;
			SDL_FillRect(profilerSurface, &profilerArea, SDL_MapRGB(profilerSurface->format, 0, 0, 0));
			drawText(profilerSurface, &profilerArea, buf);

			if(profilingTimes[i] < 100000) {
				graphRect.y = PROFILER_HEIGHT - (profilingTimes[i] >> 9);
			} else {
				graphRect.y = 128;
			}
			SDL_FillRect(profilerSurface, &graphRect, id_col);
		}
		profilerArea.y += BMP_CHAR_SIZE;
		profilingTimes[i] = 0;
	}
	
	profiler_x_axis = (profiler_x_axis + 1) % PROFILER_WIDTH;
}

void reportProfileTime(uint8_t index) {
	profilingTimes[index] +=  totalCyclesCount - profilingTimeStamps[index];
}

uint8_t open_bus() {
	return rand() % 256;
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
	int scr_w, scr_h;
	src.x = 0;
	src.y = (dma_control_reg & DMA_VID_OUT_PAGE_BIT) ? GT_HEIGHT : 0;
	src.w = GT_WIDTH;
	src.h = GT_HEIGHT;
	SDL_GetWindowSize(window, &scr_w, &scr_h);
	dest.w = min(scr_w, scr_h);
	dest.h = dest.w;
	dest.x = (scr_w - dest.w) / 2;
	dest.y = (scr_h - dest.h) / 2;
	SDL_BlitScaled(vRAM_Surface, &src, screenSurface, &dest);
	if(profiler_open) {
		drawProfilingWindow();
	}
}

uint8_t VDMA_Read(uint16_t address) {
	if(dma_control_reg & DMA_COPY_ENABLE_BIT) {
		return open_bus();
	} else {
		uint8_t* bufPtr;
		uint32_t offset = 0;
		if(dma_control_reg & DMA_CPU_TO_VRAM) {
			bufPtr = vram_buffer;
			if(banking_reg & BANK_VRAM_MASK) {
				offset = 0x4000;
			}
		} else {
			bufPtr = gram_buffer;
			offset = (((banking_reg & BANK_GRAM_MASK) << 2) | (gram_mid_bits)) << 14;
		}
		return bufPtr[(address & 0x3FFF) | offset];
	}
}

void VDMA_Write(uint16_t address, uint8_t value) {
	if(dma_control_reg & DMA_COPY_ENABLE_BIT) {
		if(((address & 0xF) == DMA_PARAM_TRIGGER) && (value & 1)) {
			SDL_Rect gRect, vRect;
			vRect.x = dma_params[DMA_PARAM_VX];
			vRect.y = dma_params[DMA_PARAM_VY];
			vRect.w = dma_params[DMA_PARAM_WIDTH];
			vRect.h = dma_params[DMA_PARAM_HEIGHT];
			gRect.x = dma_params[DMA_PARAM_GX];
			gRect.y = dma_params[DMA_PARAM_GY];
			gRect.w = dma_params[DMA_PARAM_WIDTH];
			gRect.h = dma_params[DMA_PARAM_HEIGHT];
			uint8_t outColor[2];
			uint8_t colorSel = 0;
			if(dma_control_reg & DMA_COLORFILL_ENABLE_BIT) {
				colorSel = 1;
			}
			outColor[1] = ~(dma_params[DMA_PARAM_COLOR]);
#ifdef VIDDEBUG
			printf("Copying from (%d, %d) to (%d, %d) at (%d x %d)\n",
				gRect.x & 0x7F, gRect.y & 0x7F,
				vRect.x, vRect.y,
				gRect.w, gRect.h);
#endif
			uint32_t vOffset = 0, gOffset = 0;
			if(banking_reg & BANK_VRAM_MASK) {
				vOffset = 0x4000;
			}
			int yShift = 0;
			int vy = dma_params[DMA_PARAM_VY] & 0x7F,
				gy = dma_params[DMA_PARAM_GY],
				gy2;
			if(banking_reg & BANK_VRAM_MASK) {
				yShift = GT_HEIGHT;
			}
			for(uint16_t y = 0; y < (dma_params[DMA_PARAM_HEIGHT] & 0x7F); y++) {
				int vx = dma_params[DMA_PARAM_VX] & 0x7F,
					gx = dma_params[DMA_PARAM_GX],
					gx2;
				for(uint16_t x = 0; x < (dma_params[DMA_PARAM_WIDTH] & 0x7F); x++) {
					gx2 = gx; gy2 = gy;
					if(dma_params[DMA_PARAM_WIDTH] & 0x80) {
						gx2 = ~gx2;
					}
					if(dma_params[DMA_PARAM_HEIGHT] & 0x80) {
						gy2 = ~gy2;
					}

					gOffset = ((banking_reg & BANK_GRAM_MASK) << 16) + 
						(!!(gy2 & 0x80) << 15) +
						(!!(gx2 & 0x80) << 14);
					gram_mid_bits = (!!(gy2 & 0x80) * 2) +
						(!!(gx2 & 0x80) * 1);
					outColor[0] = gram_buffer[((gy2 & 0x7F) << 7) | (gx2 & 0x7F) | gOffset];
					if(((dma_control_reg & DMA_TRANSPARENCY_BIT) || (outColor[colorSel] != 0))
						&& !((vx & 0x80) && (banking_reg & BANK_WRAPX_MASK))
						&& !((vy & 0x80) && banking_reg & BANK_WRAPY_MASK)) {
						vram_buffer[((vy & 0x7F) << 7) | (vx & 0x7F) | vOffset] = outColor[colorSel];
						put_pixel32(vRAM_Surface, vx & 0x7F, (vy & 0x7F) + yShift, convert_color(vRAM_Surface, outColor[colorSel]));
					}
					vx++;
					if(dma_control_reg & DMA_GCARRY_BIT) {
						gx++;
					} else {
						gx = (gx & 0xF0) | ((gx+1) & 0x0F);
					}
				}
				vy++;
				if(dma_control_reg & DMA_GCARRY_BIT) {
					gy++;
				} else {
					gy = (gy & 0xF0) | ((gy+1) & 0x0F);
				}
			}

			if(dma_control_reg & DMA_COPY_IRQ_BIT) {
				cpu_core->ClearIRQ();
				cpu_core->ScheduleIRQ(((dma_params[DMA_PARAM_HEIGHT] & 0x7F) * (dma_params[DMA_PARAM_WIDTH] & 0x7F)));
			}
		} else if(((address & 0xF) == DMA_PARAM_TRIGGER) && !(value & 1)) {
			cpu_core->ClearIRQ();
		} else {
#ifdef VIDDEBUG
			printf("Setting DMA param %d to %d\n", address & 0xF, value);
#endif
			dma_params[address & 0xF] = value;
		}
	} else {
		uint8_t* bufPtr;
		uint32_t offset = 0;
		SDL_Surface* targetSurface = screenSurface;
		uint32_t yShift = 0;
		if(dma_control_reg & DMA_CPU_TO_VRAM) {
			bufPtr = vram_buffer;
			targetSurface = vRAM_Surface;
			if(banking_reg & BANK_VRAM_MASK) {
				offset = 0x4000;
				yShift = GT_HEIGHT;
			}
		} else {
			bufPtr = gram_buffer;
			targetSurface = gRAM_Surface;
			yShift = (((banking_reg & BANK_GRAM_MASK) << 2) | (gram_mid_bits)) * GT_HEIGHT;
			offset = (((banking_reg & BANK_GRAM_MASK) << 2) | (gram_mid_bits)) << 14;
		}
		bufPtr[(address & 0x3FFF) | offset] = value;

		uint8_t x, y;
		x = address & 127;
		y = (address >> 7) & 127;
		put_pixel32(targetSurface, x, y + yShift, convert_color(targetSurface, value));
	}
}

void UpdateFlashShiftRegister(uint8_t nextVal) {
	//TODO: Care about DDR bits
	//For now assuming that if we're using Flash2M hardware we're behaving ourselves
	uint8_t oldVal = VIA_regs[VIA_ORA];
	uint8_t risingBits = nextVal & ~oldVal;
	if(risingBits & VIA_SPI_BIT_CLK) {
		flash2M_highbits_shifter = flash2M_highbits_shifter << 1;
		flash2M_highbits_shifter &= 0xFE;
		flash2M_highbits_shifter |= !!(oldVal & VIA_SPI_BIT_MOSI);
	} else if(risingBits & VIA_SPI_BIT_CS) {
		//flash cart CS is connected to latch clock
		flash2M_highbits = flash2M_highbits_shifter & 0xFF;
		printf("Flash highbits set to %x\n", flash2M_highbits & 0x7F);
	}
}

uint8_t MemoryRead_Flash2M(uint16_t address) {
	if(address & 0x4000) {
		return rom_buffer[0b111111100000000000000 | (address & 0x3FFF)];
	} else {
		return rom_buffer[((flash2M_highbits & 0x7F) << 14) | (address & 0x3FFF)];
	}
}

uint8_t MemoryRead_Unknown(uint16_t address) {
	//If ROMSIZE is smaller than unbanked ROM range, align end with 0xFFFF and wrap
	//If ROMSIZE is bigger than unbanked ROM range, access window at end of file.
	//TODO: Decide if unknown ROM type should just terminate emulator :P
	if(ROMSIZE <= 32768) {
		return rom_buffer[((address & 0x7FFF) + 32768 - ROMSIZE) % ROMSIZE];
	} else {
		return rom_buffer[((address & 0x7FFF) + ROMSIZE - 32768)];
	}
}

uint8_t MemoryRead(uint16_t address) {

	if(address & 0x8000) {
		switch(loadedRomType) {
			case RomType::EEPROM8K:
			return rom_buffer[address & 0x1FFF];
			case RomType::EEPROM32K:
			return rom_buffer[address & 0x7FFF];
			case RomType::FLASH2M:
			return MemoryRead_Flash2M(address);
			case RomType::UNKNOWN:
			return MemoryRead_Unknown(address);
		}
	} else if(address & 0x4000) {
		return VDMA_Read(address);
	} else if(address >= 0x3000 && address <= 0x3FFF) {
		return soundcard->ram_read(address);
	} else if(address & 0x2800 == address) {
		return VIA_regs[address & 0xF];
	} else if(address < 0x2000) {
		if(!ram_inited[address & 0x1FFF]) {
			printf("WARNING! Uninitialized RAM read at %x\n", address);
		}
		return ram_buffer[FULL_RAM_ADDRESS(address & 0x1FFF)];
	} else if(address == 0x2008 || address == 0x2009) {
		return joysticks->read((uint8_t) address);
	}
	printf("Attempted to read write-only device, may be unintended? %x\n", address);
	return open_bus();
}

void MemoryWrite(uint16_t address, uint8_t value) {
	if(address & 0x8000) {
		//ROM -> don't write
	}
	else if(address & 0x4000) {
		VDMA_Write(address, value);
	} else if(address >= 0x3000 && address <= 0x3FFF) {
		soundcard->ram_write(address, value);
	} else if((address & 0x2000)) {
		if(address & 0x800) {
			if(loadedRomType == RomType::FLASH2M) {
				if((address & 0xF) == VIA_ORA) {
					UpdateFlashShiftRegister(value);
				}
			}
			if((address & 0xF) == VIA_ORB) {
				if((VIA_regs[VIA_ORB] & 0x80) && !(value & 0x80)) {
					//falling edge of high bit of ORB
					if(value & 0x40) {
						//report duration
						reportProfileTime(value & 0x3F);
					} else {
						//store timestamp
						profilingTimeStamps[value & 0x3F] = totalCyclesCount;
					}
				}
			}
			VIA_regs[address & 0xF] = value;
		} else {
			if((address & 0x000F) == 0x0007) {
				if((value & DMA_VID_OUT_PAGE_BIT) != (dma_control_reg & DMA_VID_OUT_PAGE_BIT)) {
					bufferFlipCount++;
				}
				dma_control_reg = value;
				if(dma_control_reg & DMA_TRANSPARENCY_BIT) {
					SDL_SetColorKey(gRAM_Surface, SDL_TRUE, SDL_MapRGB(gRAM_Surface->format, 0, 0, 0));
				} else {
					SDL_SetColorKey(gRAM_Surface, SDL_FALSE, 0);
				}
			} else if((address & 0x000F) == 0x0005) {
				banking_reg = value;
			} else {
				soundcard->register_write(address, value);
			}
		}
	}
	else if(address < 0x2000) {
		ram_inited[FULL_RAM_ADDRESS(address & 0x1FFF)] = true;
		ram_buffer[FULL_RAM_ADDRESS(address & 0x1FFF)] = value;
	}
}

SDL_Event e;
bool running = true;
bool gofast = false;
bool paused = false;
bool lshift = false;
bool rshift = false;

void vram_to_surface() {
	for(int i = 0; i < FRAME_BUFFER_SIZE*2; i ++) {
		vram_buffer[i] = rand() % 256;
		put_pixel32(vRAM_Surface, i & 127, i >> 7, convert_color(vRAM_Surface, vram_buffer[i]));
	}
	for(int i = 0; i < FRAME_BUFFER_SIZE*2; i ++) {
		gram_buffer[i] = rand() % 256;
		put_pixel32(gRAM_Surface, i & 127, i >> 7, convert_color(gRAM_Surface, gram_buffer[i]));
	}
}

void randomize_memory() {
	for(int i = 0; i < RAMSIZE; i++) {
		ram_buffer[i] = rand() % 256;
		ram_inited[i] = false;
	}

	for(int i = 0; i < VRAM_BUFFER_SIZE; i++) {
		vram_buffer[i] = rand() % 256;	
	}

	for(int i = 0; i < GRAM_BUFFER_SIZE; i++) {
		gram_buffer[i] = rand() % 256;	
	}
	
	dma_control_reg = rand() % 256;
	banking_reg = rand() % 256;
	gram_mid_bits = rand() % 4;
	for(int i = 0; i < DMA_PARAMS_COUNT; i++) {
		dma_params[i] = rand() % 256;
	}
}

void CPUStopped() {
	paused = true;
	printf("CPU stopped");
#ifdef TINYFILEDIALOGS_H
	tinyfd_notifyPopup("Alert",
		"CPU has stopped either due to STP opcode",
		"info");
#endif
}

char * open_rom_dialog() {
	char const * lFilterPatterns[1] = {"*.gtr"};
#ifdef TINYFILEDIALOGS_H
	return tinyfd_openFileDialog(
		"Select a GameTank ROM file",
		"",
		1,
		lFilterPatterns,
		"GameTank Rom",
		0);
#else
	return EMBED_ROM_FILE;
#endif
}

char* lTheOpenFileName = NULL;
char filenameNoPath[256];

extern "C" {
	void LoadRomFile(const char* filename) {
		const char* cur = filename;
		const char* cur2 = NULL;
		char* cur3;
		while(*cur != 0) {
			if((*cur == '/') ||(*cur == '\\')) {
				cur2 = cur + 1;
			}
			cur++;
		}
		if(cur2 != NULL) {
			cur3 = filenameNoPath;
			while(*cur2 != 0) {
				*cur3++ = *cur2++;
			}
			*cur3 = 0;
		}
		printf("loading %s\n", filename);
		FILE* romFileP = fopen(filename, "rb");
		if(romFileP) {
			fseek(romFileP, 0L, SEEK_END);
			ROMSIZE = ftell(romFileP);
			rom_buffer = new uint8_t [ROMSIZE];
			rewind(romFileP);
			switch(ROMSIZE) {
				case 8192:
				loadedRomType = RomType::EEPROM8K;
				printf("Detected 8K (EEPROM)\n");
				break;
				case 32768:
				loadedRomType = RomType::EEPROM32K;
				printf("Detected 32K (EEPROM)\n");
				break;
				case 2097152:
				loadedRomType = RomType::FLASH2M;
				printf("Detected 2M (Flash)\n");
				break;
				default:
				loadedRomType = RomType::UNKNOWN;
				printf("Unknown ROM type: Size is %d bytes\n");
				break;
			}
			fread(rom_buffer, sizeof(uint8_t), ROMSIZE, romFileP);
			fclose(romFileP);
		}
		if(cpu_core) {
			paused = false;
			cpu_core->Reset();
		}
	}

	void SetButtons(int buttonMask) {
		if(joysticks != NULL) {
			joysticks->SetHeldButtons(buttonMask);
		}
	}
}

void openProfilerWindow() {
	if(!profiler_open) {
		profiler_window = SDL_CreateWindow( "GameTank Profiler", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, PROFILER_WIDTH, PROFILER_HEIGHT, SDL_WINDOW_SHOWN );
		profilerSurface = SDL_GetWindowSurface(profiler_window);
		SDL_SetColorKey(profilerSurface, SDL_FALSE, 0);
		profiler_open = true;
	}
}

void closeProfilerWindow() {
	if(profiler_open) {

	}
}

#ifndef EM_BOOL
#define EM_BOOL int
#endif

char titlebuf[256];

EM_BOOL mainloop(double time, void* userdata) {
	if(!paused) {
			actual_cycles = totalCyclesCount;
			cpu_core->Run(cycles_per_vsync, totalCyclesCount);
			actual_cycles = totalCyclesCount - actual_cycles;
			if(cpu_core->illegalOpcode) {
				printf("Hit illegal opcode %x\npc = %x\n", cpu_core->illegalOpcodeSrc, cpu_core->pc);
				paused = true;
			} else if(actual_cycles == 0) {
				zeroConsec++;
				if(zeroConsec == 10) {
					printf("(Got stuck at 0x%x)\n", cpu_core->pc);
					paused = true;
				}
			} else {
				zeroConsec = 0;
			}

#ifndef WASM_BUILD
			if(!gofast) {
				SDL_Delay(time_scaling * actual_cycles/system_clock);
			} else {
				lastTicks = 0;
			}
			currentTicks = SDL_GetTicks();
			if(lastTicks != 0) {
				int time_error = (currentTicks - lastTicks) - (1000 * actual_cycles/system_clock);
				if(frameCount == 100) {
					sprintf(titlebuf, "GameTank Emulator | %s | s: %.1f inc: %.1f err: %d\n", filenameNoPath, time_scaling, scaling_increment, time_error);
					SDL_SetWindowTitle(window, titlebuf);
					fps = bufferFlipCount * 60 / 100;
					frameCount = 0;
					bufferFlipCount = 0;
				}
				bool overlong = time_error > 0;
				if(overlong == prev_overlong) {
					//scaling_increment = 1;
				} else if(scaling_increment > 1) {
					scaling_increment -= 1;
				}
				if((scaling_increment > 1) || (abs(time_error) > 2)) {
					if(overlong) {
						time_scaling -= scaling_increment;
					} else {
						time_scaling += scaling_increment;
					}
				}
				prev_overlong = overlong;

				if(time_scaling < 100) {
					time_scaling = 100;
				} else if(time_scaling > 2000) {
					time_scaling = 2000;
				}
			}
			lastTicks = currentTicks;

			frameCount++;
#endif
			cycles_since_vsync += actual_cycles;
			if(cycles_since_vsync >= cycles_per_vsync) {
				cycles_since_vsync -= cycles_per_vsync;

			}
			if(dma_control_reg & DMA_VSYNC_NMI_BIT) {
				cpu_core->NMI();
			}
		} else {
			SDL_Delay(100);
		}
		refreshScreen();
		SDL_UpdateWindowSurface(window);
		if(profiler_open) {
			SDL_UpdateWindowSurface(profiler_window);
		}

		while( SDL_PollEvent( &e ) != 0 )
        {
            //User requests quit
            if( e.type == SDL_QUIT )
            {
               running = false;
            } else if(e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
            	switch(e.key.keysym.sym) {
					case SDLK_LSHIFT:
						lshift = (e.type == SDL_KEYDOWN);
						break;
					case SDLK_RSHIFT:
						rshift = (e.type == SDL_KEYDOWN);
						break;
            		case SDLK_ESCAPE:
            			running = false;
            			break;
            		case SDLK_f:
            			gofast = (e.type == SDL_KEYDOWN);
            			break;
            		case SDLK_r:
            			paused = false;
						if(lshift || rshift) {
							randomize_memory();
							vram_to_surface();
						}
            			cpu_core->Reset();
            			break;
            		case SDLK_o:
            			if(e.type == SDL_KEYDOWN) {
            				lTheOpenFileName = open_rom_dialog();
	            			if(lTheOpenFileName) {
								LoadRomFile(lTheOpenFileName);
							} else {
#ifdef TINYFILEDIALOGS_H
								tinyfd_notifyPopup("Alert",
								"No ROM was loaded",
								"warning");
#endif
							}
	            		}
            			break;
					case SDLK_F9:
						if(e.type == SDL_KEYDOWN) {
							openProfilerWindow();
						}
						break;
					case SDLK_F11:
						if(e.type == SDL_KEYDOWN) {
							if(isFullScreen) {
								SDL_SetWindowFullscreen(window, 0);
								isFullScreen = false;
								screenSurface = SDL_GetWindowSurface(window);
								SDL_SetColorKey(screenSurface, SDL_FALSE, 0);
							} else {
								SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
								isFullScreen = true;
								screenSurface = SDL_GetWindowSurface(window);
								SDL_SetColorKey(screenSurface, SDL_FALSE, 0);
							}
							scaling_increment = INITIAL_SCALING_INCREMENT;
						}
						break;
            		default:
            			joysticks->update(&e);
            			break;
            	}
            } else {
				joysticks->update(&e);
			}
        }
		
	if(!running) {
#ifdef WASM_BUILD
		emscripten_cancel_main_loop();
#endif
		printf("Finished running\n");
		SDL_DestroyWindow(window);
		SDL_Quit();
	}
	return running;
}

int main(int argC, char* argV[]) {

	srand(time(NULL));
	randomize_memory();

	palette = (RGB_Color*) gt_palette_vals;

	if(argC > 1) {
		lTheOpenFileName = argV[1];
	} else {
		lTheOpenFileName = open_rom_dialog();
	}

	if(lTheOpenFileName) {
		/*FILE* romFileP = fopen(lTheOpenFileName, "rb");
		if(romFileP) {
			fseek(romFileP, 0L, SEEK_END);
			ROMSIZE = ftell(romFileP);
			rom_buffer = new uint8_t [ROMSIZE];
			rewind(romFileP);
			switch(ROMSIZE) {
				case 8192:
				loadedRomType = RomType::EEPROM8K;
				printf("Detected 8K (EEPROM)\n");
				break;
				case 32768:
				loadedRomType = RomType::EEPROM32K;
				printf("Detected 32K (EEPROM)\n");
				break;
				case 2097152:
				loadedRomType = RomType::FLASH2M;
				printf("Detected 2M (Flash)\n");
				break;
				default:
				loadedRomType = RomType::UNKNOWN;
				printf("Unknown ROM type: Size is %d bytes\n");
				break;
			}
			fread(rom_buffer, sizeof(uint8_t), ROMSIZE, romFileP);
			fclose(romFileP);
		}*/
		LoadRomFile(lTheOpenFileName);
	} else {
		paused = true;
#ifdef TINYFILEDIALOGS_H
		tinyfd_notifyPopup("Alert",
		"No ROM was loaded",
		"warning");
#endif
		rom_buffer = new uint8_t [ROMSIZE];
		for(int i = 0; i < ROMSIZE; i++) {
			rom_buffer[i] = 0;
		}
	}

	joysticks = new JoystickAdapter();
	soundcard = new AudioCoprocessor();
	cpu_core = new mos6502(MemoryRead, MemoryWrite, CPUStopped);
	cpu_core->Reset();

	
	SDL_Init(SDL_INIT_VIDEO);
	atexit(SDL_Quit);

	bmpFont = SDL_LoadBMP("img/font.bmp");
	if(bmpFont == NULL) {
		printf("Couldn't load font.bmp!!!\n");
	}

	window = SDL_CreateWindow( "GameTank Emulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	screenSurface = SDL_GetWindowSurface(window);
	SDL_SetColorKey(screenSurface, SDL_FALSE, 0);

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
	gRAM_Surface = SDL_CreateRGBSurface(0, GT_WIDTH, GT_HEIGHT * 32, 32, rmask, gmask, bmask, amask);

	SDL_SetColorKey(vRAM_Surface, SDL_FALSE, 0);
	SDL_SetColorKey(gRAM_Surface, SDL_FALSE, 0);

	SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 0x00, 0x00, 0x00));

	vram_to_surface();

#ifdef WASM_BUILD
	emscripten_request_animation_frame_loop(mainloop, 0);
#else
	while(running) {
		mainloop(0, NULL);
	}
#endif
	return 0;
}
