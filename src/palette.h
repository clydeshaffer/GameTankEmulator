#pragma once
#include "SDL_inc.h"

typedef struct RGB_Color {
	uint8_t r, g, b;
} RGB_Color;

class Palette {
public:
    static Uint32 ConvertColor(SDL_Surface* target, uint8_t index);
};