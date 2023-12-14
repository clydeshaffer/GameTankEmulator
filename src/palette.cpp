#include "palette.h"
#include "gametank_palette.h"

Uint32 Palette::ConvertColor(SDL_Surface* target, uint8_t index) {
    if(index == 0) return SDL_MapRGB(target->format, 0, 0, 0);
	RGB_Color c = ((RGB_Color*)gt_palette_vals)[index];
	Uint32 res = SDL_MapRGB(target->format, c.r, c.g, c.b);
	if(res == SDL_MapRGB(target->format, 0, 0, 0))
		return SDL_MapRGB(target->format, 1, 1, 1);
	return res;
}