#include "ui_utils.h"
#include <string>
#include <sstream>

SDL_Surface* bmpFont = NULL;

void drawText(SDL_Surface* surface, SDL_Rect* area, char* text) {
	SDL_Rect cursor_rect, char_rect;
	cursor_rect.x = area->x;
	cursor_rect.y = area->y;
	cursor_rect.w = BMP_CHAR_SIZE;
	cursor_rect.h = BMP_CHAR_SIZE;

	char_rect.w = BMP_CHAR_SIZE;
	char_rect.h = BMP_CHAR_SIZE;

	while((*text != 0) && (cursor_rect.y < (area->y + area->h))) {
		if(*text == '\n' || ((cursor_rect.x + cursor_rect.w) > area->w)) {
			cursor_rect.x = area->x;
			cursor_rect.y += BMP_CHAR_SIZE;
		} else {
			char_rect.x = ((*text) & 0x0F) << 3;
			char_rect.y = ((*text) & 0xF0) >> 1;
			SDL_BlitSurface(bmpFont, &char_rect, surface, &cursor_rect);
			cursor_rect.x += BMP_CHAR_SIZE;
		}
		++text;
	}
}

SDL_Rect calculateTextMetrics(const std::string& text) {
    std::istringstream stream(text);
    std::string line;
    int maxLineLength = 0;
    int numberOfLines = 0;

    while (std::getline(stream, line)) {
        // Calculate length of each line
        int lineLength = static_cast<int>(line.length());
        if (lineLength > maxLineLength) {
            maxLineLength = lineLength;
        }

        // Increment number of lines
        numberOfLines++;
    }

    SDL_Rect result = { 0, 0, maxLineLength * BMP_CHAR_SIZE, numberOfLines * BMP_CHAR_SIZE};
    return result;
}

SDL_Rect centerRect(const SDL_Rect& outer, const SDL_Rect& inner) {
    SDL_Rect result;

    // Calculate the center of the outer rectangle
    int outerCenterX = outer.x + (outer.w / 2);
    int outerCenterY = outer.y + (outer.h / 2);

    // Calculate the position of the inner rectangle to center it within the outer rectangle
    int innerX = outerCenterX - (inner.w / 2);
    int innerY = outerCenterY - (inner.h / 2);

    // Set the position and size of the resulting rectangle
    result.x = innerX;
    result.y = innerY;
    result.w = inner.w;
    result.h = inner.h;

    return result;
}