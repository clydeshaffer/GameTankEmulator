using namespace std;

#include "mos6502/mos6502.h"
#include "SDL_inc.h"

#define ACP_RESET 0
#define ACP_NMI 1
#define ACP_RATE 6

typedef struct ACPState {
	uint8_t ram[4096];
	mos6502 *cpu;
    int16_t irqCounter;
    uint8_t irqRate;
	bool running;
    bool resetting;
    uint8_t dacReg;
    uint16_t clksPerHostSample;
    uint64_t cycles_per_sample;
	uint8_t clkMult;
	SDL_AudioFormat format;
	uint16_t last_irq_cycles;
	uint64_t cycle_counter;
} ACPState;

class AudioCoprocessor {
private:
	//emulated registers/memory
	ACPState state;
public:
	AudioCoprocessor();
	void ram_write(uint16_t address, uint8_t value);
	uint8_t ram_read(uint16_t address);
	void register_write(uint16_t address, uint8_t value);
	void dump_ram(char* filename);
	uint16_t get_irq_cycle_count();
};