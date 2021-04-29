#include "SDL_inc.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef WASM_BUILD
#include "emscripten.h"
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
const int RAMSIZE = 8192;
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
RGB_Color *palette;
uint8_t *rom_buffer;
uint8_t ram_buffer[RAMSIZE];
bool ram_inited[RAMSIZE];
uint8_t vram_buffer[FRAME_BUFFER_SIZE*2];
uint8_t gram_buffer[FRAME_BUFFER_SIZE*2];

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
AudioCoprocessor *soundcard;
JoystickAdapter *joysticks;

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
			vRect.w = dma_params[DMA_PARAM_WIDTH];
			vRect.h = dma_params[DMA_PARAM_HEIGHT];
			gRect.x = dma_params[DMA_PARAM_GX] & 0x7F;
			gRect.y = dma_params[DMA_PARAM_GY] & 0x7F;
			gRect.w = dma_params[DMA_PARAM_WIDTH];
			gRect.h = dma_params[DMA_PARAM_HEIGHT];
			uint8_t outColor[2];
			uint8_t colorSel = 0;
			if(dma_params[DMA_PARAM_GX] & 0x80) {
				colorSel = 1;
			}
			outColor[1] = ~(dma_params[DMA_PARAM_COLOR]);
#ifdef VIDDEBUG
			printf("Copying from (%d, %d) to (%d, %d) at (%d x %d)\n",
				gRect.x, gRect.y,
				vRect.x, vRect.y,
				gRect.w, gRect.h);
#endif
			uint16_t vOffset = 0, gOffset = 0;
			if(dma_control_reg & DMA_V_PAGE_SELECT_BIT) {
				vOffset = 0x4000;
				vRect.y += GT_HEIGHT;
			}
			if(dma_control_reg & DMA_G_PAGE_SELECT_BIT) {
				gOffset = 0x4000;
				gRect.y += GT_HEIGHT;
			}
			int yShift = 0;
			int vy = dma_params[DMA_PARAM_VY] & 0x7F,
				gy = dma_params[DMA_PARAM_GY] & 0x7F;
			if(dma_control_reg & DMA_V_PAGE_SELECT_BIT) {
				yShift = GT_HEIGHT;
			}
			for(uint16_t y = 0; y < dma_params[DMA_PARAM_HEIGHT]; y++) {
				int vx = dma_params[DMA_PARAM_VX] & 0x7F,
					gx = dma_params[DMA_PARAM_GX] & 0x7F;
				for(uint16_t x = 0; x < dma_params[DMA_PARAM_WIDTH]; x++) {
					outColor[0] = gram_buffer[(gy << 7) | gx | gOffset];
					if((dma_control_reg & DMA_TRANSPARENCY_BIT) || (outColor[colorSel] != 0)) {
						vram_buffer[(vy << 7) | vx | vOffset] = outColor[colorSel];
						put_pixel32(vRAM_Surface, vx, vy + yShift, convert_color(vRAM_Surface, outColor[colorSel]));
					}
					vx++;
					gx++;
				}
				vy++;
				gy++;
			}

			if(dma_control_reg & DMA_COPY_IRQ_BIT) {
				cpu_core->ScheduleIRQ((dma_params[DMA_PARAM_HEIGHT] * dma_params[DMA_PARAM_WIDTH]) / 4);
			}
		} else {
#ifdef VIDDEBUG
			printf("Setting DMA param %d to %d\n", address & 0xF, value);
#endif
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
		return ram_buffer[address & 0x1FFF];
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
			VIA_regs[address & 0xF] = value;
		} else {
			if((address & 0x000F) == 0x0007) {
				dma_control_reg = value;
				if(dma_control_reg & DMA_TRANSPARENCY_BIT) {
					SDL_SetColorKey(gRAM_Surface, SDL_TRUE, SDL_MapRGB(gRAM_Surface->format, 0, 0, 0));
				} else {
					SDL_SetColorKey(gRAM_Surface, SDL_FALSE, 0);
				}
			} else {
				soundcard->register_write(address, value);
			}
		}
	}
	else if(address < 0x2000) {
		ram_inited[address & 0x1FFF] = true;
		ram_buffer[address & 0x1FFF] = value;
	}
}

SDL_Event e;
bool running = true;
bool gofast = false;
bool paused = false;

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

extern "C" {
	void LoadRomFile(const char* filename) {
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
		paused = false;
		cpu_core->Reset();
	}
}

char const * lTheOpenFileName = NULL;
SDL_Window* window = NULL;
Uint32 rmask, gmask, bmask, amask;
uint64_t system_clock = 315000000/88;
uint64_t actual_cycles = 0;
uint64_t cycles_since_vsync = 0;
uint64_t cycles_per_vsync = system_clock / 60;
double time_scaling = 1000;
double scaling_increment = 100;
uint32_t lastTicks = 0, currentTicks;
uint8_t frameCount = 0;
bool prev_overlong = false;
int zeroConsec = 0;

void mainloop() {
	if(!paused) {
			actual_cycles = 0;
			cpu_core->Run(cycles_per_vsync, actual_cycles);
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

			if(!gofast) {
				SDL_Delay(time_scaling * actual_cycles/system_clock);
			} else {
				lastTicks = 0;
			}
			currentTicks = SDL_GetTicks();
			if(lastTicks != 0) {
				int time_error = (currentTicks - lastTicks) - (1000 * actual_cycles/system_clock);
				if(frameCount % 64 == 0)
					printf("scaling: %f\tincrement: %f\terror: %d\n", time_scaling, scaling_increment, time_error);
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

		while( SDL_PollEvent( &e ) != 0 )
        {
            //User requests quit
            if( e.type == SDL_QUIT )
            {
               running = false;
            } else if(e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
            	switch(e.key.keysym.sym) {
            		case SDLK_ESCAPE:
            			running = false;
            			break;
            		case SDLK_f:
            			gofast = (e.type == SDL_KEYDOWN);
            			break;
            		case SDLK_r:
            			paused = false;
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
            		default:
            			joysticks->update(&e);
            			break;
            	}
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
}

int main(int argC, char* argV[]) {

	srand(time(NULL));
	for(int i = 0; i < RAMSIZE; i++) {
		ram_buffer[i] = rand() % 256;
		ram_inited[i] = false;
	}

	palette = (RGB_Color*) gt_palette_vals;

	if(argC > 1) {
		lTheOpenFileName = argV[1];
	} else {
		lTheOpenFileName = open_rom_dialog();
	}

	if(lTheOpenFileName) {
		FILE* romFileP = fopen(lTheOpenFileName, "rb");
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
	window = SDL_CreateWindow( "GameTank Emulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN );
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
	gRAM_Surface = SDL_CreateRGBSurface(0, GT_WIDTH, GT_HEIGHT * 2, 32, rmask, gmask, bmask, amask);

	SDL_SetColorKey(vRAM_Surface, SDL_FALSE, 0);
	SDL_SetColorKey(gRAM_Surface, SDL_FALSE, 0);

	SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 0x00, 0x00, 0x00));

	for(int i = 0; i < FRAME_BUFFER_SIZE*2; i ++) {
		vram_buffer[i] = rand() % 256;
		put_pixel32(vRAM_Surface, i & 127, i >> 7, convert_color(vRAM_Surface, vram_buffer[i]));
	}
	for(int i = 0; i < FRAME_BUFFER_SIZE*2; i ++) {
		gram_buffer[i] = rand() % 256;
		put_pixel32(gRAM_Surface, i & 127, i >> 7, convert_color(gRAM_Surface, gram_buffer[i]));
	}

#ifdef WASM_BUILD
	emscripten_set_main_loop(mainloop, 0, 1);
#else
	while(running) {
		mainloop();
	}
#endif
	return 0;
}
