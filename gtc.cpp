#include <SDL.h>
#include <stdio.h>

const int GT_WIDTH = 128;
const int GT_HEIGHT = 128;

const int SCREEN_WIDTH = 256;
const int SCREEN_HEIGHT = 256;

using namespace std;

int main(int argC, char* argV[]) {
	SDL_Window* window = NULL;
	SDL_Surface* screenSurface = NULL;
	SDL_Renderer* renderer = NULL:

	SDL_Init(SDL_INIT_VIDEO);
	atexit(SDL_Quit);
	window = SDL_CreateWindow( "GameTank Emulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN );
	//screenSurface = SDL_GetWindowSurface(window);

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	SDL_RendererInfo info;
	SDL_GetRendererInfo(renderer, &info);

	SDL_Texture* VRAM_A = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		GT_WIDTH, GT_HEIGHT);

	//SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 0x33, 0x33, 0xCC));
	//SDL_UpdateWindowSurface(window);
	

	SDL_Delay(2000);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}