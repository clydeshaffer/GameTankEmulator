#include "input_recorder.h"
#include "joystick_adapter.h"
#include <filesystem>
#include <iostream>
#include <fstream>

#define TAS_BUTTON_UP 1
#define TAS_BUTTON_DOWN 2
#define TAS_BUTTON_LEFT 4
#define TAS_BUTTON_RIGHT 8
#define TAS_BUTTON_A 16
#define TAS_BUTTON_B 32
#define TAS_BUTTON_C 64
#define TAS_BUTTON_START 128


InputRecordingSession::InputRecordingSession(std::string path) {
    input_frames = 0;
    save_path = path;
}

#define CHECK_TAS_BUTTON(x) if(GameTankButtons::GamepadButtonMask::x & buttons_down_mask) { \
        newframe |= TAS_BUTTON_##x; \
    }

void InputRecordingSession::RecordFrame(uint16_t buttons_down_mask) {
    uint8_t newframe = 0;
    CHECK_TAS_BUTTON(UP)
    CHECK_TAS_BUTTON(DOWN)
    CHECK_TAS_BUTTON(LEFT)
    CHECK_TAS_BUTTON(RIGHT)
    CHECK_TAS_BUTTON(A)
    CHECK_TAS_BUTTON(B)
    CHECK_TAS_BUTTON(C)
    CHECK_TAS_BUTTON(START)
    inputs_list[input_frames++] = newframe;
}

void InputRecordingSession::Close() {
    std::fstream outfile;
    outfile.open(save_path, std::ios_base::out | std::ios_base::trunc);
    outfile.write(reinterpret_cast<char*>(inputs_list), input_frames);
    outfile.close();
}