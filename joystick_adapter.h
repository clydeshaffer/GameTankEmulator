#include <SDL.h>

using namespace std;

class JoystickAdapter {
private:
	bool pad1State = false;
	bool pad2State = false;
	uint16_t pad1Mask = 0;
	uint16_t pad2Mask = 0;
public:
	JoystickAdapter();
	uint8_t read(uint8_t portNum);
	void update(SDL_Event *e);
};