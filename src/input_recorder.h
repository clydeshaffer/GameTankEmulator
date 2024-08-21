#include <string>

//just make an hour-long buffer for now
#define INPUT_RECORDING_LENGTH 216000

uint16_t convert_tas_frame_to_gamepad_mask(uint8_t tas_frame);

class InputRecordingSession {
private:
    uint32_t input_frames;
    uint8_t inputs_list[INPUT_RECORDING_LENGTH];
    std::string save_path;
public:
    InputRecordingSession(std::string path);
    void RecordFrame(uint16_t buttons_down_mask_p1, uint16_t buttons_down_mask_p2);
    void Close();
};