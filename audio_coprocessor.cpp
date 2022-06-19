#include "SDL_inc.h"
#include <stdio.h>
#include <stdlib.h>
#include <fstream>

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
    ACPState *state = (ACPState*) udata;
    uint16_t *stream16 = (uint16_t*) stream;
    for(int i = 0; i < len/sizeof(uint16_t); i++) {
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
            state->cycle_counter = 0;
            if(state->running) {
                state->cpu->IRQ();
                state->cpu->ClearIRQ();
                state->cpu->Run(state->cycles_per_sample, state->cycle_counter);
            }
        }
    }
}

ACPState *singleton_acp_state;

uint8_t ACP_MemoryRead(uint16_t address) {
    return singleton_acp_state->ram[address & 0xFFF];
}

uint8_t ACP_CPUSync(uint16_t address) {
    uint8_t opcode = ACP_MemoryRead(address);
    if(opcode == 0x40) {
        //If opcode is ReTurn from Interrupt
        singleton_acp_state->last_irq_cycles = singleton_acp_state->cycle_counter;
    }
    return opcode;
}

void ACP_MemoryWrite(uint16_t address, uint8_t value) {
    singleton_acp_state->ram[address & 0xFFF] = value;
    if(address & 0x8000) {
        singleton_acp_state->dacReg = value;
    }
}

const char* AudioFormatString(SDL_AudioFormat f) {
    switch(f) {
        case AUDIO_S8: return "AUDIO_S8";
        case AUDIO_U8: return "AUDIO_U8";
        case AUDIO_S16LSB: return "AUDIO_S16LSB";
        case AUDIO_S16MSB: return "AUDIO_S16MSB";
        //case AUDIO_S16SYS: return "AUDIO_S16SYS";
        case AUDIO_U16LSB: return "AUDIO_U16LSB";
        case AUDIO_U16MSB: return "AUDIO_U16MSB";
        //case AUDIO_U16SYS: return "AUDIO_U16SYS";
        case AUDIO_S32LSB: return "AUDIO_S32LSB";
        case AUDIO_S32MSB: return "AUDIO_S32MSB";
        //case AUDIO_S32SYS: return "AUDIO_S32SYS";
        case AUDIO_F32LSB: return "AUDIO_F32LSB";
        case AUDIO_F32MSB: return "AUDIO_F32MSB";
        //case AUDIO_F32SYS: return "AUDIO_F32SYS";
        default: return "UNKNOWN";
    }
}

void ACP_CPUStopped() {
}

AudioCoprocessor::AudioCoprocessor() {
	SDL_AudioSpec wanted, obtained;

    singleton_acp_state = &state;

    state.cpu = new mos6502(ACP_MemoryRead, ACP_MemoryWrite, ACP_CPUStopped, ACP_CPUSync);

    state.irqCounter = 0;
    state.irqRate = 0;
    state.resetting = false;
    state.running = false;
    state.dacReg;
    state.clksPerHostSample = 315000000 / (88 * 44100);
    state.cycles_per_sample = 1024;
    state.last_irq_cycles = 0;
#ifdef WASM_BUILD
    state.clkMult = 2;
#else
    state.clkMult = 4;
#endif

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

    SDL_InitSubSystem(SDL_INIT_AUDIO);

    SDL_AudioDeviceID openedDevice = SDL_OpenAudioDevice(NULL, 0, &wanted, &obtained, 0);

    /* Open the audio device, forcing the desired format */
    if (openedDevice  == 0 ) {
        fprintf(stdout, "Couldn't open audio: %s\n", SDL_GetError());
    } else {
        printf("Opened audio device:\n\tFreq: %d\n\tFormat %s\n\tChannels: %d\n\tSamples: %d\n",
            obtained.freq, AudioFormatString(obtained.format), obtained.channels, obtained.samples);
        state.format = obtained.format;
        SDL_PauseAudioDevice(openedDevice, 0);
    }


	return;
}

void AudioCoprocessor::dump_ram(char* filename) {
    ofstream dumpfile (filename, ios::out | ios::binary);
    dumpfile.write((char*) state.ram, 4096);
    dumpfile.close();
}

uint16_t AudioCoprocessor::get_irq_cycle_count() {
    return state.last_irq_cycles;
}