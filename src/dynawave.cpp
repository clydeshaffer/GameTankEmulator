#include "SDL_inc.h"
#include <stdio.h>
#include <stdlib.h>

#include "dynawave.h"

void DynaWave::wavetable_write(uint16_t address, uint8_t value) {
	state.wavetable[address & 0xFFF] = value;
}

uint8_t DynaWave::wavetable_read(uint16_t address) {
	return state.wavetable[address & 0xFFF];
}

uint8_t volume_convert[8] = {252, 141, 80, 45, 25, 14, 8, 0};

//if high bit set, average n&F values per sample
//if high bit clear, advance once evert n samples
uint8_t reclock_table[8] = {0x8A, 0x85, 0x83, 0x81, 0x02, 0x03, 0x06, 0x0D};

void DynaWave::register_write(uint16_t address, uint8_t value) {
	state.regs[address & 7] = value;
	switch(address & 7) {
		case SQUARE1_CTRL:
			state.volumes[SQUARE1] = volume_convert[(state.regs[SQUARE1_CTRL] >> 3) & 0x7];
		case SQUARE1_NOTE:
			state.periods[SQUARE1] = ((state.regs[SQUARE1_NOTE]+1) << ((state.regs[SQUARE1_CTRL] & 7) + 5)) / 325;
			break;
		case SQUARE2_CTRL:
			state.volumes[SQUARE2] = volume_convert[(state.regs[SQUARE2_CTRL] >> 3) & 0x7];
		case SQUARE2_NOTE:
			state.periods[SQUARE2] = ((state.regs[SQUARE2_NOTE]+1) << ((state.regs[SQUARE2_CTRL] & 7) + 5)) / 325;
			break;
		case NOISE_CTRL:
			state.periods[NOISE] = reclock_table[state.regs[NOISE_CTRL] & 7];
			state.volumes[NOISE] = volume_convert[(state.regs[NOISE_CTRL] >> 3) & 0x7];
			break;
		//No case for WAVE_NOTE, it gets updated when the sample loops
		case WAVE_CTRL:
			state.periods[WAVE] = reclock_table[state.regs[WAVE_CTRL] & 7];
			state.volumes[WAVE] = volume_convert[(state.regs[WAVE_CTRL] >> 3) & 0x7];
			break;
		default:
			break;
	}
}

void fill_audio(void *udata, uint8_t *stream, int len) {
	AudioState *state = (AudioState*) udata;
	for(int i = 0; i < len; i++) {
		state->clocks[SQUARE1] --;
		if(state->clocks[SQUARE1] == 0) {
			state->clocks[SQUARE1] = state->periods[SQUARE1];
			state->out[SQUARE1] = !(state->out[SQUARE1]) * state->volumes[SQUARE1];
		}
		state->clocks[SQUARE2] --;
		if(state->clocks[SQUARE2] == 0) {
			state->clocks[SQUARE2] = state->periods[SQUARE2];
			state->out[SQUARE2] = !(state->out[SQUARE2]) * state->volumes[SQUARE2];
		}

		if(state->periods[NOISE] & 0x80) {
			//multiple noise samples per out sample
			uint64_t total = 0;
			uint8_t samplecount = 0xF & state->periods[NOISE];
			for(int n = 0; n < samplecount; n++) {
				state->lfsr = (state->lfsr << 1) | (!!(state->lfsr & 0x8000) ^ !!(state->lfsr & 0x0100));
				total += !!(state->lfsr & 0x8000) * state->volumes[NOISE];
			}
			state->out[NOISE] = total / samplecount;
		} else {
			//update noise sample after multiple out samples
			state->clocks[NOISE] --;
			if(state->clocks[NOISE] == 0) {
				state->clocks[NOISE] = state->periods[NOISE];
				state->lfsr = (state->lfsr << 1) | ~(!!(state->lfsr & 0x8000) ^ !!(state->lfsr & 0x0100));
				state->out[NOISE] = !!(state->lfsr & 0x8000) * state->volumes[NOISE];
			}
		}

		if(state->periods[WAVE] & 0x80) {
			//multiple noise samples per out sample
			uint64_t total = 0;
			uint8_t samplecount = 0xF & state->periods[WAVE];
			for(int n = 0; n < samplecount; n++) {
				state->wave_index++;
				if(state->wavetable[state->wave_index % 4096] == 0xFF) {
					state->wave_index = state->regs[WAVE_NOTE] << 4;
				}
				total += state->wavetable[state->wave_index % 4096] * state->volumes[WAVE] >> 8;
			}
			state->out[WAVE] = total / samplecount;
		} else {
			//update noise sample after multiple out samples
			state->clocks[WAVE] --;
			if(state->clocks[WAVE] == 0) {
				state->clocks[WAVE] = state->periods[WAVE];
				state->wave_index++;
				if(state->wavetable[state->wave_index % 4096] == 0xFF) {
					state->wave_index = state->regs[WAVE_NOTE] << 4;
				}
				state->out[WAVE] = state->wavetable[state->wave_index % 4096] * state->volumes[WAVE] >> 8;
			}
		}

		stream[i] = (state->out[SQUARE2] + state->out[SQUARE1] + state->out[NOISE] + state->out[WAVE]) / 8;
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
	state.out[2] = 0;
	state.out[3] = 0;

	state.lfsr = 0;
	state.wave_index = 0;

	for(int i = 0; i < 4096; i ++) {
		state.wavetable[i] = rand() % 256;
	}

    /* Set the audio format */
    wanted.freq = 44100;
    wanted.format = AUDIO_S8;
    wanted.channels = 1;    /* 1 = mono, 2 = stereo */
    wanted.samples = 512;  /* Good low-latency value for callback */
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
