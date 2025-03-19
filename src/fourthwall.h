#include <cstdint>
#include "system_state.h"
#include "SDL_inc.h"

// vram toolkit
void draw_square(SystemState &system_state, SDL_Surface *surface, uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint32_t color) {
	uint16_t src_y = (system_state.banking & BANK_VRAM_MASK) ? 128 : 0;
    for (uint8_t pixel_x = x; pixel_x < x + width; ++pixel_x) {
        for (uint8_t pixel_y = src_y; pixel_y < src_y + height; ++pixel_y) {
            put_pixel32(surface, pixel_x, pixel_y, color);
        } 
    }
}


struct FourthWall {
	uint8_t* game_ram_pointer;

	uint8_t upper_byte;
	uint8_t lower_byte;
	bool is_upper_set;
	bool is_lower_set;
    bool is_broken;

    void show_overlay(SystemState &system_state, SDL_Surface *surface) {
		draw_square(system_state, surface, 0,0,20,20,0xFFFFFFFF);
    }
};

