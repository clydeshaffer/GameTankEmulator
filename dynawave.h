using namespace std;

#define SQUARE1 0
#define SQUARE2 1
#define NOISE 2
#define WAVE 3

#define SQUARE1_NOTE 0
#define SQUARE1_CTRL 1
#define SQUARE2_NOTE 2
#define SQUARE2_CTRL 3
#define NOISE_CTRL   4
#define WAVE_NOTE 5
#define WAVE_CTRL 6

typedef struct AudioState {
	uint8_t regs[7];
	uint8_t wavetable[4096];
	uint16_t clocks[4];
	uint16_t periods[4];
	uint8_t volumes[4];
	uint16_t out[4];
	uint16_t lfsr;
	uint16_t wave_index;
} AudioState;

class DynaWave {
private:
	//emulated registers/memory
	AudioState state;
public:
	DynaWave();
	void wavetable_write(uint16_t address, uint8_t value);
	uint8_t wavetable_read(uint16_t address);
	void register_write(uint16_t address, uint8_t value);
};