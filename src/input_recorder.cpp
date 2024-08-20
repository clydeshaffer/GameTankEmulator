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

#define CHECK_TAS_BUTTON(mask, x) if(GameTankButtons::GamepadButtonMask::x & mask) { \
        newframe |= TAS_BUTTON_##x; \
    }

void InputRecordingSession::RecordFrame(uint16_t buttons_down_mask_p1, uint16_t buttons_down_mask_p2) {
    uint8_t newframe = 0;
    CHECK_TAS_BUTTON(buttons_down_mask_p1, UP)
    CHECK_TAS_BUTTON(buttons_down_mask_p1, DOWN)
    CHECK_TAS_BUTTON(buttons_down_mask_p1, LEFT)
    CHECK_TAS_BUTTON(buttons_down_mask_p1, RIGHT)
    CHECK_TAS_BUTTON(buttons_down_mask_p1, A)
    CHECK_TAS_BUTTON(buttons_down_mask_p1, B)
    CHECK_TAS_BUTTON(buttons_down_mask_p1, C)
    CHECK_TAS_BUTTON(buttons_down_mask_p1, START)
    inputs_list[input_frames++] = newframe;
    newframe = 0;
    CHECK_TAS_BUTTON(buttons_down_mask_p2, UP)
    CHECK_TAS_BUTTON(buttons_down_mask_p2, DOWN)
    CHECK_TAS_BUTTON(buttons_down_mask_p2, LEFT)
    CHECK_TAS_BUTTON(buttons_down_mask_p2, RIGHT)
    CHECK_TAS_BUTTON(buttons_down_mask_p2, A)
    CHECK_TAS_BUTTON(buttons_down_mask_p2, B)
    CHECK_TAS_BUTTON(buttons_down_mask_p2, C)
    CHECK_TAS_BUTTON(buttons_down_mask_p2, START)
    inputs_list[input_frames++] = newframe;
    inputs_list[input_frames++] = 0;
    inputs_list[input_frames++] = 0;
}

void InputRecordingSession::Close() {
    std::fstream outfile;
    outfile.open(save_path, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
    outfile.write(reinterpret_cast<char*>(inputs_list), input_frames);
    outfile.close();
}