#include "midi-adapter.h"
#include "imgui.h"
#include <iostream>
#include <iomanip>

ExpansionPortMidiAdapter::ExpansionPortMidiAdapter()
    : midi{
        libremidi::input_configuration{
            .on_message = [this](const libremidi::message& msg) {
                this->handle_message(msg);
            }
        }
    }
{
    input_port = nullptr;
    msg_buf_head = 0;
    msg_buf_tail = 0;
}

void ExpansionPortMidiAdapter::handle_message(const libremidi::message& message)
{
    for(int i = 0; i < message.bytes.size(); ++i) {
        /*std::cout << std::uppercase << std::hex
            << std::setw(2) << std::setfill('0')
            << static_cast<unsigned>(message.bytes[i])
            << std::dec; // restore
        */
       msg_buffer[msg_buf_tail] = message.bytes[i];
       ++msg_buf_tail;
       if(msg_buf_tail == MIDI_ADAPTER_BUF_SIZE) {
        msg_buf_tail = 0;
       }
    }
}

void ExpansionPortMidiAdapter::ExpansionSettingsMenu() {
    if(ImGui::BeginMenu("Midi Adapter (EXPERIMENTAL)")) {
        for(const libremidi::input_port& port : obs.get_input_ports()) {
            if(ImGui::MenuItem(port.port_name.c_str())) {
                 midi.open_port(port);
            }
        }
        
        ImGui::EndMenu();
    }
}

void ExpansionPortMidiAdapter::ExpansionPortWrite(uint8_t byte, uint8_t addr) {

}

uint8_t ExpansionPortMidiAdapter::ExpansionPortRead(uint8_t addr) {
    uint8_t msgByte = msg_buffer[msg_buf_head];
    if(msg_buf_head != msg_buf_tail) {
        ++msg_buf_head;
        if(msg_buf_head == MIDI_ADAPTER_BUF_SIZE) {
            msg_buf_head = 0;
        }
    } else {
        return 0xFF;
    }
    return msgByte;
}