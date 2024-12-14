#pragma once
#include "SDL_inc.h"

#define PALETTE_SELECT_OLD 0
#define PALETTE_SELECT_CAPTURE 256
#define PALETTE_SELECT_SCALED 512

extern int palette_select;

typedef struct RGB_Color {
	uint8_t r, g, b;
} RGB_Color;

class Palette {
public:
    static Uint32 ConvertColor(SDL_Surface* target, uint8_t index);
};