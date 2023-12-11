#include "../SDL_inc.h"
#include <string>
#define BMP_CHAR_SIZE 8

extern SDL_Surface* bmpFont;

void drawText(SDL_Surface* surface, SDL_Rect* area, char* text);

SDL_Rect calculateTextMetrics(const std::string& text);

SDL_Rect centerRect(const SDL_Rect& outer, const SDL_Rect& inner);