#include "SDL_inc.h"

#define GAMEPAD_MASK_UP 0b0000100000001000
#define GAMEPAD_MASK_DOWN 0b0000010000000100
#define GAMEPAD_MASK_LEFT 0b0000001000000000
#define GAMEPAD_MASK_RIGHT 0b0000000100000000
#define GAMEPAD_MASK_A 0b0000000000010000
#define GAMEPAD_MASK_B 0b0001000000000000
#define GAMEPAD_MASK_C 0b0010000000000000
#define GAMEPAD_MASK_START 0b0000000000100000

#define GAMEPAD_MASK_ALLDIRS (GAMEPAD_MASK_UP|GAMEPAD_MASK_DOWN|GAMEPAD_MASK_LEFT|GAMEPAD_MASK_RIGHT)

using namespace std;

class JoystickAdapter {
private:
	bool pad1State = false;
	bool pad2State = false;
	uint16_t pad1Mask = 0;
	uint16_t pad2Mask = 0;
	uint16_t held1Mask = 0;
	SDL_Joystick* gGameController = NULL;
public:
	JoystickAdapter();
	~JoystickAdapter();
	uint8_t read(uint8_t portNum, bool stateful);
	void update(SDL_Event *e);
	void SetHeldButtons(uint16_t heldMask);
};
