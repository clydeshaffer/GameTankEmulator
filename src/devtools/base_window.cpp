#include "base_window.h"

bool BaseWindow::IsOpen() {
    return open;
}

BaseWindow::BaseWindow(int width, int height) {
    open = true;
    window = SDL_CreateWindow("",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
	 	width, height, 0);
}

BaseWindow::~BaseWindow() {
    SDL_DestroyWindow(window);
}