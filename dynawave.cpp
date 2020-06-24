#include <stdio.h>

#include "dynawave.h"

void DynaWave::wavetable_write(uint16_t address, uint8_t value) {
	state.wavetable[address & 0xFFF] = value;
}

uint8_t DynaWave::wavetable_read(uint16_t address) {
	return state.wavetable[address & 0xFFF];
}

uint8_t volume_convert[8] = {252, 141, 80, 45, 25, 14, 8, 0};
void DynaWave::register_write(uint16_t address, uint8_t value) {
	state.regs[address & 7] = value;
	state.periods[0] = (state.regs[0] << (state.regs[1] & 7)) >> 3;
	state.volumes[0] = volume_convert[(state.regs[1] >> 3) & 0x7];
	state.periods[1] = (state.regs[2] << (state.regs[3] & 7)) >> 3;
	state.volumes[1] = volume_convert[(state.regs[3] >> 3) & 0x7];
}

uint8_t mix_buffer[2048];
void fill_audio(void *udata, uint8_t *stream, int len) {
	AudioState *state = (AudioState*) udata;
	for(int i = 0; i < len; i++) {
		state->clocks[0] --;
		if(state->clocks[0] == 0) {
			state->clocks[0] = state->periods[0];
			state->out[0] = !(state->out[0]) * state->volumes[0];
		}
		state->clocks[1] --;
		if(state->clocks[1] == 0) {
			state->clocks[1] = state->periods[1];
			state->out[1] = !(state->out[1]) * state->volumes[1];
		}
		//mix_buffer[i] = state->out[1] * 0xE0;
		stream[i] = (state->out[1] + state->out[0]) / 2;
	}
	//SDL_MixAudio(stream, mix_buffer, len, SDL_MIX_MAXVOLUME / 2);
}

DynaWave::DynaWave() {
	SDL_AudioSpec wanted;

	state.clocks[0] = 1;
	state.clocks[1] = 1;
	state.clocks[2] = 1;
	state.clocks[3] = 1;

	state.periods[0] = 1;
	state.periods[1] = 1;
	state.periods[2] = 1;
	state.periods[3] = 1;

	state.volumes[0] = 0;
	state.volumes[1] = 0;
	state.volumes[2] = 0;
	state.volumes[3] = 0;

	state.out[0] = 0;
	state.out[1] = 0;

    /* Set the audio format */
    wanted.freq = 44100;
    wanted.format = AUDIO_U8;
    wanted.channels = 1;    /* 1 = mono, 2 = stereo */
    wanted.samples = 1024;  /* Good low-latency value for callback */
    wanted.callback = fill_audio;
    wanted.userdata = &state;

    /* Open the audio device, forcing the desired format */
    if ( SDL_OpenAudio(&wanted, NULL) < 0 ) {
        fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
    } else {
    	SDL_PauseAudio(0);
    }


	return;
}