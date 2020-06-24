#include <SDL.h>

using namespace std;

typedef struct AudioState {
	uint8_t regs[7];
	uint8_t wavetable[4096];
	uint16_t clocks[4];
	uint16_t periods[4];
	uint8_t volumes[4];
	uint8_t out[4];
} SoundMem;

class DynaWave {
private:
	//emulated registers/memory
	AudioState state;

	uint8_t audio_buffer[2048];
	Uint8 *audio_chunk;
    Uint32 audio_len;
    Uint8 *audio_pos;
public:
	DynaWave();
	void wavetable_write(uint16_t address, uint8_t value);
	uint8_t wavetable_read(uint16_t address);
	void register_write(uint16_t address, uint8_t value);
};