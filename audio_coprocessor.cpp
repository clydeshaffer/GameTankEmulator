#include "SDL_inc.h"
#include <stdio.h>
#include <stdlib.h>

#include "audio_coprocessor.h"

void AudioCoprocessor::ram_write(uint16_t address, uint8_t value) {
	state.ram[address & 0xFFF] = value;
}

uint8_t AudioCoprocessor::ram_read(uint16_t address) {
	return state.ram[address & 0xFFF];
}

void AudioCoprocessor::register_write(uint16_t address, uint8_t value) {
    printf("audio register %x written with %x\n", (address), value);
	switch(address & 7) {
		case ACP_RESET:
			state.resetting = true;
            state.irqCounter = 255;
            break;
		case ACP_NMI:
			state.cpu->NMI();
			break;
		case ACP_RATE:
			state.irqRate = (((value << 1) & 0xFE) | (value & 1));
            state.running = (value & 0x80) != 0;
            state.cycles_per_sample = state.irqRate * state.clkMult;
            break;
		default:
			break;
	}
}

void fill_audio(void *udata, uint8_t *stream, int len) {
    uint16_t *stream16 = (uint16_t*) stream;
	ACPState *state = (ACPState*) udata;
    uint64_t actual_cycles;
	for(int i = 0; i < len/2; i++) {
        stream16[i] = state->dacReg;
        stream16[i] -= 128;
        stream16[i] *= 32;
        state->irqCounter -= state->clksPerHostSample;
        if(state->irqCounter < 0) {
            if(state->resetting) {
                state->resetting = false;
                state->cpu->Reset();
            }
            state->irqCounter += state->irqRate;
            actual_cycles = 0;
            if(state->running) {
                state->cpu->IRQ();
                state->cpu->Run(state->cycles_per_sample, actual_cycles);
            }
        }
        	
	}
}

ACPState *singleton_acp_state;

uint8_t ACP_MemoryRead(uint16_t address) {
    return singleton_acp_state->ram[address & 0xFFF];
}

void ACP_MemoryWrite(uint16_t address, uint8_t value) {
    singleton_acp_state->ram[address & 0xFFF] = value;
    if(address & 0x8000) {
        singleton_acp_state->dacReg = value;
    }
}

void ACP_CPUStopped() {
}

AudioCoprocessor::AudioCoprocessor() {
	SDL_AudioSpec wanted;

    singleton_acp_state = &state;

    state.cpu = new mos6502(ACP_MemoryRead, ACP_MemoryWrite, ACP_CPUStopped);

    state.irqCounter = 0;
    state.irqRate = 0;
    state.resetting = false;
    state.running = false;
    state.dacReg;
    state.clksPerHostSample = 315000000 / (88 * 44100);
    state.cycles_per_sample = 1024;
    state.clkMult = 4;

	for(int i = 0; i < 4096; i ++) {
		state.ram[i] = rand() % 256;
	}

    /* Set the audio format */
    wanted.freq = 44100;
    wanted.format = AUDIO_S16SYS;
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
