#pragma once
#include <libremidi/libremidi.hpp>

#define MIDI_ADAPTER_BUF_SIZE 1024

class ExpansionPortMidiAdapter {
    private:
    libremidi::observer obs;
    libremidi::input_port* input_port;
    libremidi::midi_in midi;
    void handle_message(const libremidi::message& msg);
    uint8_t msg_buffer[MIDI_ADAPTER_BUF_SIZE];
    uint8_t msg_buf_head, msg_buf_tail;
    public:
    ExpansionPortMidiAdapter();
    void ExpansionSettingsMenu();
    void ExpansionPortWrite(uint8_t byte, uint8_t addr);
    uint8_t ExpansionPortRead(uint8_t addr);
};