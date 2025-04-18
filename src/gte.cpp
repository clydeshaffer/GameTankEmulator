#include "SDL_inc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cmath>
#include <time.h>
#include <fstream>
#include <cstring>
#include <filesystem>
#include <vector>
#include <thread>
#include <algorithm>
#ifdef WASM_BUILD
#include "emscripten.h"
#include <emscripten/html5.h>
#else
#include "tinyfd/tinyfiledialogs.h"
#endif

#include "joystick_adapter.h"
#include "audio_coprocessor.h"
#include "blitter.h"
#include "palette.h"

#include "timekeeper.h"
#include "system_state.h"
#include "emulator_config.h"
#include "game_config.h"

#include "mos6502/mos6502.h"

#include "devtools/memory_map.h"
#include "devtools/breakpoints.h"
#include "devtools/source_map.h"

#include "ui/ui_utils.h"
#include "devtools/profiler.h"
#include "devtools/disassembler.h"

#ifndef WASM_BUILD
#include "devtools/profiler_window.h"
#include "devtools/mem_browser_window.h"
#include "devtools/vram_window.h"
#include "devtools/stepping_window.h"
#include "devtools/patching_window.h"
#include "devtools/controller_options_window.h"
#include "imgui.h"
#include "implot.h"
#include "imgui/backends/imgui_impl_sdl2.h"
#include "imgui/backends/imgui_impl_sdlrenderer2.h"
#endif

#ifndef WINDOW_TITLE
#define WINDOW_TITLE "GameTank Emulator"
#endif

using namespace std;

const int GT_WIDTH = 128;
const int GT_HEIGHT = 128;

RomType loadedRomType;

mos6502 *cpu_core;
Blitter *blitter;
AudioCoprocessor *soundcard;
JoystickAdapter *joysticks;
SystemState system_state;
CartridgeState cartridge_state;

const int SCREEN_WIDTH = 512;
const int SCREEN_HEIGHT = 512;
RGB_Color *palette;

MemoryMap* loadedMemoryMap;
GameConfig* gameconfig;
std::string currentRomFilePath;
std::string nvramFileFullPath;
std::string flashFileFullPath;

bool vsyncProfileArmed = false;
bool vsyncProfileRunning = false;

void SaveNVRAM() {
	fstream file;
	if(loadedRomType != RomType::FLASH2M_RAM32K) return;
	printf("SAVING %s\n", nvramFileFullPath.c_str());
	file.open(nvramFileFullPath.c_str(), ios_base::out | ios_base::binary | ios_base::trunc);
	file.write((char*) cartridge_state.save_ram, CARTRAMSIZE);
	file.close();
}

void LoadNVRAM() {
	fstream file;
	if(loadedRomType != RomType::FLASH2M_RAM32K) return;
	printf("LOADING %s\n", nvramFileFullPath.c_str());
	file.open(nvramFileFullPath.c_str(), ios_base::in | ios_base::binary);
	file.read((char*) cartridge_state.save_ram, CARTRAMSIZE);
	file.close();
}

std::thread savingThread;

void SaveModifiedFlash() {
	if(EmulatorConfig::noSave) return;
	fstream file_out, file_in;
	uint8_t* rom_cursor = cartridge_state.rom;
	uint8_t buf[256];
	file_in.open(currentRomFilePath, ios_base::in | ios_base::binary);
	file_out.open(flashFileFullPath.c_str(), ios_base::out | ios_base::binary | ios_base::trunc);
	while(file_in) {
		file_in.read((char*) buf, 256);
		size_t bytesRead = file_in.gcount();
		if(bytesRead) {
			for(int i = 0; i < bytesRead; ++i) {
				buf[i] ^= *(rom_cursor++);
			}
			file_out.write((char*) buf, bytesRead);
		}
	}
	file_in.close();
	file_out.close();
#ifdef WASM_BUILD
	EM_ASM(
		FS.syncfs(false, function (err) {
			assert(!err);
			});
	);
#endif
}

fstream orig_rom, xor_file;
void LoadModifiedFlash() {
	uint8_t* rom_cursor = cartridge_state.rom;
	uint8_t buf[256];
	uint8_t bufx[256];
	size_t bytes_read = 0;
	std::cout << "opening " << currentRomFilePath << " and " << flashFileFullPath << "\n";
	orig_rom.open(currentRomFilePath, ios_base::in | ios_base::binary);
	xor_file.open(flashFileFullPath, ios_base::in | ios_base::binary);
	std::cout << "XORing files together... \n";
	while(orig_rom && xor_file) {
		orig_rom.read((char*) buf, 256);
		xor_file.read((char*) bufx, 256); 
		for(int i = 0; i < 256; ++i) {
			*(rom_cursor++) = buf[i] ^ bufx[i];
		}
		bytes_read += 256;
	}
	std::cout << bytes_read << " bytes read from xor file\n";
#ifndef WASM_BUILD
	orig_rom.close();
	xor_file.close();
#endif
}

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

#define RAM_HIGHBITS_SHIFT 7

#define FULL_RAM_ADDRESS(x) (((system_state.banking & BANK_RAM_MASK) << RAM_HIGHBITS_SHIFT) | (x))

extern unsigned char font_map[];

Timekeeper timekeeper;
Profiler profiler(timekeeper);

SDL_Surface* gRAM_Surface = NULL;
SDL_Surface* vRAM_Surface = NULL;

SDL_Window* mainWindow = NULL;
SDL_Window* buffers_window = NULL;
Uint32 rmask, gmask, bmask, amask;

#ifndef WASM_BUILD
ImGuiContext* main_imgui_ctx;
ImPlotContext* main_implot_ctx;

std::vector<BaseWindow*> toolWindows;
#endif

SDL_Renderer* mainRenderer = NULL;
SDL_Texture* framebufferTexture = NULL;

bool isFullScreen = false;

bool profiler_open = false;
bool buffers_open = false;
int profiler_x_axis = 0;

uint8_t open_bus() {
	return rand() % 256;
}

uint8_t VDMA_Read(uint16_t address) {
	blitter->CatchUp();
	if(system_state.dma_control & DMA_COPY_ENABLE_BIT) {
		return open_bus();
	} else {
		uint8_t* bufPtr;
		uint32_t offset = 0;
		if(system_state.dma_control & DMA_CPU_TO_VRAM) {
			bufPtr = system_state.vram;
			if(system_state.banking & BANK_VRAM_MASK) {
				offset = 0x4000;
			}
		} else {
			bufPtr = system_state.gram;
			offset = (((system_state.banking & BANK_GRAM_MASK) << 2) | (blitter->gram_mid_bits)) << 14;
		}
		return bufPtr[(address & 0x3FFF) | offset];
	}
}

void VDMA_Write(uint16_t address, uint8_t value) {
	blitter->CatchUp();
	if(system_state.dma_control & DMA_COPY_ENABLE_BIT) {
		blitter->SetParam(address, value);
	} else {
		uint8_t* bufPtr;
		uint32_t offset = 0;
		SDL_Surface* targetSurface = NULL;
		uint32_t yShift = 0;
		if(system_state.dma_control & DMA_CPU_TO_VRAM) {
			bufPtr = system_state.vram;
			targetSurface = vRAM_Surface;
			if(system_state.banking & BANK_VRAM_MASK) {
				offset = 0x4000;
				yShift = GT_HEIGHT;
			}
		} else {
			bufPtr = system_state.gram;
			targetSurface = gRAM_Surface;
			yShift = (((system_state.banking & BANK_GRAM_MASK) << 2) | (blitter->gram_mid_bits)) * GT_HEIGHT;
			offset = (((system_state.banking & BANK_GRAM_MASK) << 2) | (blitter->gram_mid_bits)) << 14;
		}
		bufPtr[(address & 0x3FFF) | offset] = value;

		uint8_t x, y;
		x = address & 127;
		y = (address >> 7) & 127;
		put_pixel32(targetSurface, x, y + yShift, Palette::ConvertColor(targetSurface, value));
	}
}

void UpdateFlashShiftRegister(uint8_t nextVal) {
	//TODO: Care about DDR bits
	//For now assuming that if we're using Flash2M hardware we're behaving ourselves
	uint8_t oldVal = system_state.VIA_regs[VIA_ORA];
	uint8_t risingBits = nextVal & ~oldVal;
	if(risingBits & VIA_SPI_BIT_CLK) {
		cartridge_state.bank_shifter = cartridge_state.bank_shifter << 1;
		cartridge_state.bank_shifter &= 0xFE;
		cartridge_state.bank_shifter |= !!(oldVal & VIA_SPI_BIT_MOSI);
	} else if(risingBits & VIA_SPI_BIT_CS) {
		//flash cart CS is connected to latch clock
		if((cartridge_state.bank_mask ^ cartridge_state.bank_shifter) & 0x80) {
			SaveNVRAM();
		}
		cartridge_state.bank_mask = cartridge_state.bank_shifter;
		if(loadedRomType != RomType::FLASH2M_RAM32K) {
			cartridge_state.bank_mask |= 0x80;
		}
		//printf("Flash highbits set to %x\n", cartridge_state.bank_mask);
	}
}

uint8_t MemoryRead_Flash2M(uint16_t address) {
	if(address & 0x4000) {
		return cartridge_state.rom[0b111111100000000000000 | (address & 0x3FFF)];
	} else {
		if(!(cartridge_state.bank_mask & 0x80))
			return cartridge_state.save_ram[(address & 0x3FFF) | ((cartridge_state.bank_mask & 0x40) << 8)];
		else return cartridge_state.rom[((cartridge_state.bank_mask & 0x7F) << 14) | (address & 0x3FFF)];
	}
}

uint8_t MemoryRead_Unknown(uint16_t address) {
	//If cartridge_state.size is smaller than unbanked ROM range, align end with 0xFFFF and wrap
	//If cartridge_state.size is bigger than unbanked ROM range, access mainWindow at end of file.
	//TODO: Decide if unknown ROM type should just terminate emulator :P
	if(cartridge_state.size <= 32768) {
		return cartridge_state.rom[((address & 0x7FFF) + 32768 - cartridge_state.size) % cartridge_state.size];
	} else {
		return cartridge_state.rom[((address & 0x7FFF) + cartridge_state.size - 32768)];
	}
}

uint8_t* GetRAM(const uint16_t address) {
	return &(system_state.ram[FULL_RAM_ADDRESS(address & 0x1FFF)]);
}

uint8_t MemoryReadResolve(const uint16_t address, bool stateful) {
	if(address & 0x8000) {
		switch(loadedRomType) {
			case RomType::EEPROM8K:
			return cartridge_state.rom[address & 0x1FFF];
			case RomType::EEPROM32K:
			return cartridge_state.rom[address & 0x7FFF];
			case RomType::FLASH2M:
			case RomType::FLASH2M_RAM32K:
			return MemoryRead_Flash2M(address);
			case RomType::UNKNOWN:
			return MemoryRead_Unknown(address);
		}
	} else if(address & 0x4000) {
		return VDMA_Read(address);
	} else if((address >= 0x3000) && (address <= 0x3FFF)) {
		return soundcard->ram_read(address);
	} else if((address >= 0x2800) && (address <= 0x2FFF)) {
		return system_state.VIA_regs[address & 0xF];
	} else if(address < 0x2000) {
		if(stateful) {
			if(!system_state.ram_initialized[FULL_RAM_ADDRESS(address & 0x1FFF)]) {
				//printf("WARNING! Uninitialized RAM read at %x (Bank %x)\n", address, system_state.banking >> 5);
			}
		}
		return *GetRAM(address);
	} else if((address == 0x2008) || (address == 0x2009)) {
		return joysticks->read((uint8_t) address, stateful);
	}
	if(stateful) {
		printf("Attempted to read write-only device, may be unintended? %x\n", address);
	}
	return open_bus();
}

uint8_t MemoryRead(uint16_t address) {
	return MemoryReadResolve(address, true);
}

uint8_t MemorySync(uint16_t address) {
	if(timekeeper.clock_mode == CLOCKMODE_NORMAL) {
		if(Breakpoints::checkBreakpoint(address, cartridge_state.bank_mask)) {
			timekeeper.clock_mode = CLOCKMODE_STOPPED;
			Disassembler::Decode(MemoryReadResolve, loadedMemoryMap, address, 32);
			cpu_core->Freeze();
		}
		uint8_t opcode = MemoryReadResolve(address, false);
		if(opcode == 0x20) { //JSR
			uint16_t jsr_dest = MemoryReadResolve(address+1, false) | (MemoryReadResolve(address+2, false) << 8);
			profiler.LogJSR(address, cartridge_state.bank_mask, jsr_dest);
		} else if(opcode == 0x60) { //RTS
			profiler.LogRTS(address, cartridge_state.bank_mask);
		}
	}
	return MemoryRead(address);
}

void MemoryWrite(uint16_t address, uint8_t value) {
	if(address & 0x8000) {
		if(loadedRomType == RomType::FLASH2M_RAM32K) {
			if(!(address & 0x4000)) {
				if(!(cartridge_state.bank_mask & 0x80)) {
					cartridge_state.save_ram[(address & 0x3FFF) | ((cartridge_state.bank_mask & 0x40) << 8)] = value;
				}
			}
		}
		if(loadedRomType == RomType::FLASH2M) {
			if(cartridge_state.write_mode) {
				uint8_t* location;
				if(address & 0x4000) {
					location = &(cartridge_state.rom[0b111111100000000000000 | (address & 0x3FFF)]);
				} else {
					location = &(cartridge_state.rom[((cartridge_state.bank_mask & 0x7F) << 14) | (address & 0x3FFF)]);
				}
				*location &= value;
				cartridge_state.write_mode = false;
			} else {
				//Skipping over details like bypass and unlock commands for now
				//So off-spec flash operation will be inaccurate
				if(value == 0x10) {
					//Chip Erase
					for(int i = 0; i < (1 << 21); ++i) {
						cartridge_state.rom[i] = 0xFF;
					}
				} else if (value == 0x30) {
					//Sector erase
					uint8_t sectorBits = ((address & (1 << 13)) >> 13) | ((cartridge_state.bank_mask & 0x7F) << 1);
					uint8_t sectorNum = sectorBits >> 3;
					if(sectorNum < 31) {
						//most of the sector table
						uint32_t x = sectorNum << 16;
						for(uint32_t i = 0; i < (1 << 16); ++i) {
							cartridge_state.rom[x] = 0xFF;
							++x;
						}
					} else if((sectorBits & 4) == 0) {
						uint32_t x = 0x1F0000;
						for(uint32_t i = 0; i < (1 << 15); ++i) {
							cartridge_state.rom[x] = 0xFF;
							++x;
						}
					} else if(sectorBits == 0b11111100) {
						uint32_t x = 0x1F8000;
						for(uint32_t i = 0; i < (1 << 13); ++i) {
							cartridge_state.rom[x] = 0xFF;
							++x;
						}
					} else if(sectorBits == 0b11111101) {
						uint32_t x = 0x1FA000;
						for(uint32_t i = 0; i < (1 << 13); ++i) {
							cartridge_state.rom[x] = 0xFF;
							++x;
						}
					} else if((sectorBits >> 1) == 0b1111111) {
						uint32_t x = 0x1FC000;
						for(uint32_t i = 0; i < (1 << 14); ++i) {
							cartridge_state.rom[x] = 0xFF;
							++x;
						}
					}
				} else if(value == 0xA0) {
					cartridge_state.write_mode = true;
				} else if(value == 0x90) {
					//first byte of lock command should be a good time to write to file
#ifdef WASM_BUILD
					SaveModifiedFlash();
#else
					if(savingThread.joinable()) {
						savingThread.join();
					}
					savingThread = std::thread(SaveModifiedFlash);
#endif
				}
			}
		}
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
				if((system_state.VIA_regs[VIA_ORB] & 0x80) && !(value & 0x80)) {
					//falling edge of high bit of ORB
					if(value & 0x40) {
						//report duration
						profiler.LogTime(value & 0x3F);
					} else {
						//store timestamp
						profiler.profilingTimeStamps[value & 0x3F] = timekeeper.totalCyclesCount;
					}
				}
			}
			system_state.VIA_regs[address & 0xF] = value;
		} else {
			if((address & 0x000F) == 0x0007) {
				blitter->CatchUp();
				if((value & DMA_VID_OUT_PAGE_BIT) != (system_state.dma_control & DMA_VID_OUT_PAGE_BIT)) {
					profiler.bufferFlipCount++;
					if(profiler.measure_by_frameflip) {
						profiler.ResetTimers();
						profiler.last_blitter_activity = blitter->pixels_this_frame;
						blitter->pixels_this_frame = 0;
					}
				}
				system_state.dma_control = value;
				system_state.dma_control_irq = (system_state.dma_control & DMA_COPY_IRQ_BIT) != 0;
				if(system_state.dma_control & DMA_TRANSPARENCY_BIT) {
					SDL_SetColorKey(gRAM_Surface, SDL_TRUE, SDL_MapRGB(gRAM_Surface->format, 0, 0, 0));
				} else {
					SDL_SetColorKey(gRAM_Surface, SDL_FALSE, 0);
				}
			} else if((address & 0x000F) == 0x0005) {
				blitter->CatchUp();
				system_state.banking = value;
				//printf("banking reg set to %x\n", value);
			} else {
				soundcard->register_write(address, value);
			}
		}
	}
	else if(address < 0x2000) {
		/*if(!system_state.ram_initialized[FULL_RAM_ADDRESS(address & 0x1FFF)]) {
			printf("First RAM write at %x (Bank %x) (Value %x)\n", address, system_state.banking >> 6, value);
		}*/
		system_state.ram_initialized[FULL_RAM_ADDRESS(address & 0x1FFF)] = true;
		system_state.ram[FULL_RAM_ADDRESS(address & 0x1FFF)] = value;
	}
}

SDL_Event e;
bool running = true;
bool gofast = false;
bool paused = false;
bool lshift = false;
bool rshift = false;

void randomize_vram() {
	for(int i = 0; i < VRAM_BUFFER_SIZE; i ++) {
		system_state.vram[i] = rand() % 256;
		put_pixel32(vRAM_Surface, i & 127, i >> 7, Palette::ConvertColor(vRAM_Surface, system_state.vram[i]));
	}
	for(int i = 0; i < GRAM_BUFFER_SIZE; i ++) {
		system_state.gram[i] = rand() % 256;
		put_pixel32(gRAM_Surface, i & 127, i >> 7, Palette::ConvertColor(gRAM_Surface, system_state.gram[i]));
	}
}

void randomize_memory() {
	for(int i = 0; i < RAMSIZE; i++) {
		system_state.ram[i] = rand() % 256;
		system_state.ram_initialized[i] = false;
	}

	for(int i = 0; i < VRAM_BUFFER_SIZE; i++) {
		system_state.vram[i] = rand() % 256;	
	}

	for(int i = 0; i < GRAM_BUFFER_SIZE; i++) {
		system_state.gram[i] = rand() % 256;	
	}
	
	system_state.dma_control = rand() % 256;
	system_state.dma_control_irq = (system_state.dma_control & DMA_COPY_IRQ_BIT) != 0;
	system_state.banking = rand() % 256;
	blitter->gram_mid_bits = rand() % 4;
}

extern "C" {
void PauseEmulation() {
  paused = true;

  AudioCoprocessor::singleton_acp_state->isEmulationPaused = true;
}

void ResumeEmulation() {
  paused = false;

  AudioCoprocessor::singleton_acp_state->isEmulationPaused = false;
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

const char * open_rom_dialog() {
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
	// Attempts to load a rom by filename into a buffer
	// 0 on success
	// -1 on failure (e.g. file by name doesn't exist)
	int LoadRomFile(const char* filename) {
		std::filesystem::path filepath(filename);
		currentRomFilePath = filepath.string();
#ifdef WASM_BUILD
		std::filesystem::path nvramPath("/idbfs");
		nvramPath /= std::filesystem::path(currentRomFilePath).filename();
#else
		std::filesystem::path nvramPath(filename);
#endif
		nvramPath.replace_extension("sav");
		nvramFileFullPath = nvramPath.string();
		if (EmulatorConfig::xorFile != NULL) {
		  flashFileFullPath = std::string(EmulatorConfig::xorFile);
		} else {
		    nvramPath.replace_extension("xor");
		    flashFileFullPath = nvramPath.string();
		}
		nvramPath.replace_extension("gtrcfg");

		gameconfig = new GameConfig(nvramPath.string().c_str());

		std::filesystem::path defaultMemMapFilePath = filepath.parent_path().append("../build/out.map");
		std::filesystem::path defaultSourceMapFilePath = filepath.parent_path().append("../build/sourcemap.dbg");

		if(std::filesystem::exists(defaultMemMapFilePath)) {
			printf("found default memory map file location %s\n", defaultMemMapFilePath.c_str());
			loadedMemoryMap = new MemoryMap(defaultMemMapFilePath.string());
			Breakpoints::linkBreakpoints(*loadedMemoryMap);
		} else {
			loadedMemoryMap = new MemoryMap();
			printf("default memory map file %s not found\n", defaultMemMapFilePath.c_str());
		}

		if(std::filesystem::exists(defaultSourceMapFilePath)) {
			printf("found default source map file location %s\n", defaultSourceMapFilePath.c_str());
			std::string sourceMapPathString = defaultSourceMapFilePath.string();
			SourceMap::singleton = new SourceMap(sourceMapPathString);
		} else {
			printf("default source map file %s not found\n", defaultSourceMapFilePath.c_str());
		}

		printf("loading %s\n", filename);
		FILE* romFileP = fopen(filename, "rb");
		if(!romFileP) {
			printf("Unable to open file: %s\n", filename);
			return -1;
		}

		fseek(romFileP, 0L, SEEK_END);
		cartridge_state.size = ftell(romFileP);
		//cartridge_state.rom = new uint8_t [cartridge_state.size];
		cartridge_state.write_mode = false;
		rewind(romFileP);
		switch(cartridge_state.size) {
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
			printf("Unknown ROM type: Size is %d bytes\n", cartridge_state.size);
			break;
		}
		fread(cartridge_state.rom, sizeof(uint8_t), cartridge_state.size, romFileP);
		fclose(romFileP);
		if(cpu_core) {
			paused = false;
			cpu_core->Reset();
			cartridge_state.write_mode = false;
		}

		if(loadedRomType == RomType::FLASH2M) {

			if(std::filesystem::exists(flashFileFullPath.c_str())) {
				std::cout << "Loading flash save from " << flashFileFullPath << "\n";
				LoadModifiedFlash();
			} else {
				std::cout << "Couldn't find " << flashFileFullPath << "\n";
			}

			if(
				(cartridge_state.rom[0x1FFFF0] == 'S') &&
				(cartridge_state.rom[0x1FFFF1] == 'A') &&
				(cartridge_state.rom[0x1FFFF2] == 'V') &&
				(cartridge_state.rom[0x1FFFF3] == 'E')) {
					loadedRomType = RomType::FLASH2M_RAM32K;
					if(std::filesystem::exists(nvramFileFullPath.c_str())) {
						LoadNVRAM();
					}
				}
		}
		return 0;
	}

	void SetButtons(int buttonMask) {
		if(joysticks != NULL) {
			joysticks->SetHeldButtons(buttonMask);
		}
	}

	void takeScreenShot() {
		SDL_Surface *screenshot = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
		SDL_RenderReadPixels(mainRenderer, NULL, SDL_PIXELFORMAT_ARGB8888, screenshot->pixels, screenshot->pitch);
		SDL_SaveBMP(screenshot, "screenshot.bmp");
		SDL_FreeSurface(screenshot);
	}
}
#ifndef WASM_BUILD
template <typename T>
void closeToolByType() {
    toolWindows.erase(
        std::remove_if(
            toolWindows.begin(),
            toolWindows.end(),
            [](BaseWindow* window) {
                if(dynamic_cast<T*>(window) != nullptr) {
					delete window;
					return true;
				}
				return false;
            }
        ),
        toolWindows.end()
    );
}

template <typename T>
bool toolTypeIsOpen() {
    for (const auto& window : toolWindows) {
        if (dynamic_cast<T*>(window) != nullptr) {
            return true;
        }
    }
    return false;
}

void toggleProfilerWindow() {
	if(!toolTypeIsOpen<ProfilerWindow>()) {
		toolWindows.push_back(new ProfilerWindow(profiler));
	} else {
		closeToolByType<ProfilerWindow>();
	}
}

void toggleMemBrowserWindow() {
	if(!toolTypeIsOpen<MemBrowserWindow>()) {
		toolWindows.push_back(new MemBrowserWindow(loadedMemoryMap, MemoryReadResolve, GetRAM, *gameconfig));
	} else {
		closeToolByType<MemBrowserWindow>();
	}
}

void toggleVRAMWindow() {
	if(!toolTypeIsOpen<VRAMWindow>()) {
		toolWindows.push_back(new VRAMWindow(vRAM_Surface, gRAM_Surface,
			&system_state, cpu_core, &cartridge_state));
	} else {
		closeToolByType<VRAMWindow>();
	}
}

void toggleSteppingWindow() {
	if(!toolTypeIsOpen<SteppingWindow>()) {
		toolWindows.push_back(new SteppingWindow(timekeeper, loadedMemoryMap, cpu_core, *gameconfig, cartridge_state));
	} else {
		closeToolByType<SteppingWindow>();
	}
}

void togglePatchingWindow() {
	if(!toolTypeIsOpen<PatchingWindow>()) {
		toolWindows.push_back(new PatchingWindow(loadedMemoryMap, gameconfig));
	} else {
		closeToolByType<PatchingWindow>();
	}
}

void doRamDump() {
	soundcard->dump_ram("audio_debug.dat");
	ofstream dumpfile ("ram_debug.dat", ios::out | ios::binary);
	dumpfile.write((char*) system_state.ram, RAMSIZE);
	dumpfile.close();
}

void toggleControllerOptionsWindow() {
	if(!toolTypeIsOpen<ControllerOptionsWindow>()) {
		toolWindows.push_back(new ControllerOptionsWindow(joysticks));
	} else {
		closeToolByType<ControllerOptionsWindow>();
	}
}

#endif

void toggleFullScreen() {
	if(isFullScreen) {
		SDL_SetWindowFullscreen(mainWindow, 0);
		isFullScreen = false;
	} else {
		SDL_SetWindowFullscreen(mainWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
		isFullScreen = true;
	}
	timekeeper.scaling_increment = INITIAL_SCALING_INCREMENT;
}

void toggleMute() {
	AudioCoprocessor::singleton_acp_state->isMuted = !AudioCoprocessor::singleton_acp_state->isMuted;
}

typedef struct HotkeyAssignment {
	void (*func)();
	SDL_Keycode  key;
} HotkeyAssignment;

HotkeyAssignment hotkeys[] = {
	{&toggleFullScreen, SDLK_F11},
	{&toggleMute, SDLK_m},
#ifndef WASM_BUILD
	{&doRamDump, SDLK_F6},
	{&toggleSteppingWindow, SDLK_F7},
	{&takeScreenShot, SDLK_F8},
	{&toggleMemBrowserWindow, SDLK_F9},
	{&toggleVRAMWindow, SDLK_F10},
	{&toggleProfilerWindow, SDLK_F12},
#endif
};

bool checkHotkey(SDL_Keycode  key) {
	for(HotkeyAssignment assignment : hotkeys) {
		if(assignment.key == key) {
			assignment.func();
			return true;
		}
	}
	return false;
}

#ifndef EM_BOOL
#define EM_BOOL int
#endif

void refreshScreen() {
	SDL_Rect src, dest;
	int scr_w, scr_h;
	src.x = 0;
	src.y = (system_state.dma_control & DMA_VID_OUT_PAGE_BIT) ? GT_HEIGHT : 0;
	src.w = GT_WIDTH;
	src.h = GT_HEIGHT;
	SDL_GetWindowSize(mainWindow, &scr_w, &scr_h);
	dest.w = min(scr_w, scr_h);
	dest.h = dest.w;
	dest.x = (scr_w - dest.w) / 2;
	dest.y = (scr_h - dest.h) / 2;
	//SDL_BlitScaled(vRAM_Surface, &src, screenSurface, &dest);
	SDL_UpdateTexture(framebufferTexture, NULL, vRAM_Surface->pixels, vRAM_Surface->pitch);

	SDL_RenderClear(mainRenderer);
	SDL_RenderCopy(mainRenderer, framebufferTexture, &src, &dest);

#ifndef WASM_BUILD
	ImGui::SetCurrentContext(main_imgui_ctx);

    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();
	if(ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if(ImGui::MenuItem("Open Rom")) {
				const char* rom_file_name = open_rom_dialog();
				if(rom_file_name) {
					LoadRomFile(rom_file_name);
				}	
			}
			ImGui::EndMenu();
		}
		if(ImGui::BeginMenu("Settings")) {
			if(ImGui::MenuItem("Controllers")) {
				toggleControllerOptionsWindow();
			}
			ImGui::MenuItem("Toggle Instant Blits", NULL, &(blitter->instant_mode));
			ImGui::SliderInt("Volume", &AudioCoprocessor::singleton_acp_state->volume, 0, 256);
			ImGui::Checkbox("Mute", &AudioCoprocessor::singleton_acp_state->isMuted);
			if(ImGui::BeginMenu("Pallete")) {
				ImGui::RadioButton("Unscaled Capture", &palette_select, PALETTE_SELECT_CAPTURE);
				ImGui::RadioButton("Full Contrast", &palette_select, PALETTE_SELECT_SCALED);
				ImGui::RadioButton("Cheap HDMI converter", &palette_select, PALETTE_SELECT_HDMI);
				ImGui::RadioButton("Flawed Theory (Legacy)", &palette_select, PALETTE_SELECT_OLD);
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}
		if(ImGui::BeginMenu("Tools")) {
			if(ImGui::MenuItem("Profiler (F12)")) {
				toggleProfilerWindow();
			}
			if(ImGui::MenuItem("Memory Browser (F9)")) {
				toggleMemBrowserWindow();
			}
			if(ImGui::MenuItem("VRAM Viewer (F10)")) {
				toggleVRAMWindow();
			}
			if(ImGui::MenuItem("Code Stepper (F7)")) {
				toggleSteppingWindow();
			}
			if(ImGui::MenuItem("Patching Window")) {
				togglePatchingWindow();
			}
			if(ImGui::MenuItem("Update Patches")) {
				gameconfig->UpdateAllPatches(cartridge_state.rom);
			}
			if(ImGui::MenuItem("Dump RAM to file (F6)")) {
				doRamDump();
			}
			if(ImGui::MenuItem("Deep Profile Single Vsync")) {
				vsyncProfileArmed = true;
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
	ImGui::Render();
	ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
#endif
	SDL_RenderPresent(mainRenderer);
}

char titlebuf[256];
int32_t intended_cycles = 0;

#ifdef WASM_BUILD
double target_frame_period_ms = 1000.0 / 60.0;
double last_raf_time = 0;
double frame_time_accumulator = 0;
#endif

EM_BOOL mainloop(double time, void* userdata) {
#ifdef WASM_BUILD
        double delta_time = time - last_raf_time;
        frame_time_accumulator += delta_time;
        last_raf_time = time;
        if(frame_time_accumulator < target_frame_period_ms) {
                return true;
        }
        frame_time_accumulator -= target_frame_period_ms;
#endif

	if(!paused) {
			timekeeper.actual_cycles = timekeeper.totalCyclesCount;
#ifndef WASM_BUILD
			switch(timekeeper.clock_mode) {
				case CLOCKMODE_NORMAL:
					cpu_core->freeze = false;
					intended_cycles = timekeeper.cycles_per_vsync;
					break;
				case CLOCKMODE_SINGLE:
					cpu_core->freeze = false;
					Disassembler::Decode(MemoryReadResolve, loadedMemoryMap, cpu_core->pc, 32);
					intended_cycles = 1;
					timekeeper.clock_mode = CLOCKMODE_STOPPED;
					break;
				case CLOCKMODE_STOPPED:
					intended_cycles = 0;
					break;
			}
			if(intended_cycles) {
				cpu_core->Run(intended_cycles, timekeeper.totalCyclesCount);
			}
#else
			intended_cycles = timekeeper.cycles_per_vsync;
			cpu_core->Run(intended_cycles, timekeeper.totalCyclesCount);
#endif
			timekeeper.actual_cycles = timekeeper.totalCyclesCount - timekeeper.actual_cycles;
			if(cpu_core->illegalOpcode) {
				printf("Hit illegal opcode %x\npc = %x\n", cpu_core->illegalOpcodeSrc, cpu_core->pc);
				paused = true;
			} else if((timekeeper.clock_mode == CLOCKMODE_NORMAL) && (timekeeper.actual_cycles == 0)) {
				profiler.zeroConsec++;
				if(profiler.zeroConsec == 10) {
					printf("(Got stuck at 0x%x)\n", cpu_core->pc);
					paused = true;
				}
				timekeeper.totalCyclesCount += intended_cycles;
			} else {
				profiler.zeroConsec = 0;
			}

#ifndef WASM_BUILD
			if(!gofast) {
				SDL_Delay(timekeeper.time_scaling * intended_cycles/timekeeper.system_clock);
			} else {
				timekeeper.lastTicks = 0;
			}
			timekeeper.currentTicks = SDL_GetTicks();

			if(timekeeper.clock_mode == CLOCKMODE_NORMAL) {
				if(timekeeper.lastTicks != 0) {
					int time_error = (timekeeper.currentTicks - timekeeper.lastTicks) - (1000 * intended_cycles/timekeeper.system_clock);
					if(timekeeper.frameCount == 100) {
					  sprintf(titlebuf, "%s | %s | s: %.1f inc: %.1f err: %d\n", WINDOW_TITLE, currentRomFilePath.c_str(), timekeeper.time_scaling, timekeeper.scaling_increment, time_error);
						SDL_SetWindowTitle(mainWindow, titlebuf);
						profiler.fps = profiler.bufferFlipCount * 60 / 100;
						timekeeper.frameCount = 0;
						profiler.bufferFlipCount = 0;
					}
					bool overlong = time_error > 0;

					if(overlong == timekeeper.prev_overlong) {
						//scaling_increment = 1;
					} else if(timekeeper.scaling_increment > 1) {
						timekeeper.scaling_increment -= 1;
					}
					if((timekeeper.scaling_increment > 1) || (abs(time_error) > 2)) {
						if(overlong) {
							timekeeper.time_scaling -= timekeeper.scaling_increment;
						} else {
							timekeeper.time_scaling += timekeeper.scaling_increment;
						}
					}
					timekeeper.prev_overlong = overlong;

					if(timekeeper.time_scaling < 100) {
						timekeeper.time_scaling = 100;
					} else if(timekeeper.time_scaling > 2000) {
						timekeeper.time_scaling = 2000;
					}
				}
				timekeeper.lastTicks = timekeeper.currentTicks;
				timekeeper.frameCount++;
			}
#endif
			timekeeper.totalCyclesCount -= timekeeper.actual_cycles;
			timekeeper.totalCyclesCount += intended_cycles;
			timekeeper.cycles_since_vsync += intended_cycles;
			if(timekeeper.cycles_since_vsync >= timekeeper.cycles_per_vsync) {
				timekeeper.cycles_since_vsync -= timekeeper.cycles_per_vsync;
				if(system_state.dma_control & DMA_VSYNC_NMI_BIT) {
					cpu_core->NMI();
					if(vsyncProfileArmed) {
						profiler.DeepProfileStart();
						vsyncProfileArmed = false;
						vsyncProfileRunning = true;
					} else if(vsyncProfileRunning) {
						profiler.DeepProfileStop(loadedMemoryMap, SourceMap::singleton);
						vsyncProfileRunning = false;
					}
				}
				if(!profiler.measure_by_frameflip) {
					profiler.ResetTimers();
					profiler.last_blitter_activity = blitter->pixels_this_frame;
					blitter->pixels_this_frame = 0;
				}
			}
		} else {
			SDL_Delay(100);
		}
		blitter->CatchUp();
		refreshScreen();
		SDL_UpdateWindowSurface(mainWindow);

		if(EmulatorConfig::noSound) {
			AudioCoprocessor::fill_audio(AudioCoprocessor::singleton_acp_state, NULL, intended_cycles / AudioCoprocessor::singleton_acp_state->cycles_per_sample);
		}

		while( SDL_PollEvent( &e ) != 0 )
        {
#ifndef WASM_BUILD
			if(SDL_GetMouseFocus() == mainWindow) {
				ImGui::SetCurrentContext(main_imgui_ctx);
				ImPlot::SetCurrentContext(main_implot_ctx);
				ImGui_ImplSDL2_ProcessEvent(&e);
			}
			for (auto toolWindow : toolWindows) {
				toolWindow->HandleEvent(e);
			}

			if(ImGui::GetIO().WantCaptureKeyboard && ((e.type == SDL_KEYDOWN) || (e.type == SDL_KEYUP))) {
				continue;
			}
#endif
            //User requests quit
            if( e.type == SDL_QUIT )
            {
               running = false;
            } else if(e.type == SDL_WINDOWEVENT)
			{
				if(e.window.event == SDL_WINDOWEVENT_CLOSE) {
					SDL_Window* closedWindow = SDL_GetWindowFromID(e.window.windowID);
					if(closedWindow == mainWindow) {
						running = false;
					}
				}
			} else if((e.key.repeat == 0) && (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP)) {
				if((e.type == SDL_KEYUP) || !checkHotkey(e.key.keysym.sym)) {
					switch(e.key.keysym.sym) {
						case SDLK_LSHIFT:
							lshift = (e.type == SDL_KEYDOWN);
							break;
						case SDLK_RSHIFT:
							rshift = (e.type == SDL_KEYDOWN);
							break;
						case SDLK_ESCAPE:
							#ifndef DISABLE_ESC
								running = false;
							#endif
							break;
						case SDLK_BACKQUOTE:
							gofast = (e.type == SDL_KEYDOWN);
							break;
						case SDLK_r:
							//TODO add menu item for reset
							paused = false;
							if(lshift || rshift) {
								randomize_memory();
								randomize_vram();
							}
							cpu_core->Reset();
							cartridge_state.write_mode = false;
							break;
						case SDLK_o:
							if(e.type == SDL_KEYDOWN) {
								const char* rom_file_name = open_rom_dialog();
								if(rom_file_name) {
									LoadRomFile(rom_file_name);
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
            } else if(e.key.repeat == 0) {
				joysticks->update(&e);
			}
        }

#ifndef WASM_BUILD
		for (auto& window : toolWindows) {
			window->Draw();
		}

		auto const to_be_removed = std::partition(begin(toolWindows), end(toolWindows), [](auto w){ return w->IsOpen(); });
		std::for_each(to_be_removed, end(toolWindows), [](auto w) {
			delete w;
		});
		toolWindows.erase(to_be_removed, end(toolWindows));
#endif
		
	if(!running) {
#ifdef WASM_BUILD
		emscripten_cancel_main_loop();
#else
		for (auto& window : toolWindows) {
			delete window;
		}
		toolWindows.clear();

		ImPlot::DestroyContext(main_implot_ctx);
    	ImGui::DestroyContext(main_imgui_ctx);
#endif
		printf("Finished running\n");
		
    	SDL_DestroyRenderer(mainRenderer);
		SDL_DestroyWindow(mainWindow);
		SDL_Quit();
	}
	return running;
}

int main(int argC, char* argV[]) {
	srand(time(NULL));
	cartridge_state.rom = new uint8_t[1 << 21];

	const char* rom_file_name = NULL;

#ifdef WASM_BUILD
	rom_file_name = EMBED_ROM_FILE;
#else
	for(int argIdx = 1; argIdx < argC; ++argIdx) {
		if((argV[argIdx])[0] == '-') {
			EmulatorConfig::parseArg(argV[argIdx]);
		} else if(!rom_file_name) {
			rom_file_name = argV[argIdx];
		}
	}
#endif

	//cartridge_state.rom = new uint8_t [cartridge_state.size];
		for(int i = 0; i < cartridge_state.size; i++) {
			cartridge_state.rom[i] = 0;
		}

	joysticks = new JoystickAdapter();
	soundcard = new AudioCoprocessor();
	cpu_core = new mos6502(MemoryRead, MemoryWrite, CPUStopped, MemorySync);
	cpu_core->Reset();
	cartridge_state.write_mode = false;
	blitter = new Blitter(cpu_core, &timekeeper, &system_state, vRAM_Surface);
	randomize_memory();
	
	SDL_Init(SDL_INIT_VIDEO);
	atexit(SDL_Quit);

	bmpFont = SDL_CreateRGBSurfaceFrom(font_map, 128, 128, 32, 4 * 128, rmask, gmask, bmask, amask);

	vRAM_Surface = SDL_CreateRGBSurface(0, GT_WIDTH, GT_HEIGHT * 2, 32, rmask, gmask, bmask, amask);
	gRAM_Surface = SDL_CreateRGBSurface(0, GT_WIDTH, GT_HEIGHT * 32, 32, rmask, gmask, bmask, amask);

	SDL_SetColorKey(vRAM_Surface, SDL_FALSE, 0);
	SDL_SetColorKey(gRAM_Surface, SDL_FALSE, 0);

	mainWindow = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	mainRenderer = SDL_CreateRenderer(mainWindow, -1, EmulatorConfig::defaultRendererFlags);
	framebufferTexture = SDL_CreateTexture(mainRenderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, GT_WIDTH, GT_HEIGHT * 2);

#ifndef WASM_BUILD
	main_imgui_ctx = ImGui::CreateContext();
	main_implot_ctx = ImPlot::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigViewportsNoAutoMerge = true;
	io.IniFilename = NULL;
	ImGui::StyleColorsDark();
	ImGui_ImplSDL2_InitForSDLRenderer(mainWindow, mainRenderer);
	ImGui_ImplSDLRenderer2_Init(mainRenderer);
#endif

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

	randomize_vram();

	if(!rom_file_name || LoadRomFile(rom_file_name) == -1) {
		paused = true;
#ifdef TINYFILEDIALOGS_H
		if(rom_file_name) {
			tinyfd_notifyPopup("Alert",
			"No ROM was loaded",
			"warning");
		}
#endif
		
	}

#ifdef WASM_BUILD

	emscripten_request_animation_frame_loop(mainloop, 0);
#else
	SDL_RaiseWindow(mainWindow);
	while(running) {
		mainloop(0, NULL);
	}
	joysticks->SaveBindings();
#endif
	return 0;
}
