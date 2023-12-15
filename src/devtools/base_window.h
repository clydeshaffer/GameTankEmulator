#pragma once
#include "../SDL_inc.h"
#include <string>

class BaseWindow {
protected:
    SDL_Window* window;
    bool open = true;
public:
    BaseWindow(int width, int height);
    virtual ~BaseWindow();
    bool IsOpen();
    virtual void Draw() = 0;
    virtual void HandleEvent(SDL_Event& e) = 0;
};