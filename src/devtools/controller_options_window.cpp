#include "../imgui/imgui.h"
#include "controller_options_window.h"

#include "../data/controller_outline.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../stb/stb_image.h"

SDL_Texture* controller_outline = NULL;

const ImVec2 controller_button_positions[] = {
    {173, 92},
    {173, 207},
    {118, 156},
    {226, 156},
    {349, 101},
    {519, 207},
    {570, 150},
    {471, 103}
};

const char* controller_button_strings[] = {
    "UP",
    "DOWN",
    "LEFT",
    "RIGHT",
    "START",
    "A",
    "B",
    "C"
};

ControllerOptionsWindow::ControllerOptionsWindow(JoystickAdapter* inputAdapter) {
    int width, height, channels;
    this->inputAdapter = inputAdapter;

    if(controller_outline == NULL) {
        unsigned char* imgData = stbi_load_from_memory(img_controller_outline_png, img_controller_outline_png_len, &width, &height, &channels, 4);
        if(!imgData) return;
        
        SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormatFrom(
            imgData, width, height, 32, width * 4, SDL_PIXELFORMAT_RGBA32
        );
        if(!surface) {
            return;
        }

        controller_outline = SDL_CreateTextureFromSurface(this->renderer, surface);
        SDL_FreeSurface(surface);
        stbi_image_free(imgData);
    }
}

ImVec2 ControllerOptionsWindow::Render() {
    ImGui::Begin("Controller Options", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize);
    ImGui::SetWindowSize({659, 333});
    ImGui::SetWindowPos({0, 0});
    ImGui::Text("WIP - NOT FUNCTIONAL YET");
    if(controller_outline) {
        //ImGui::GetBackgroundDrawList()->AddImage(controller_outline, {0, 0},{659,333}, {0, 0}, {1, 1});
        ImGui::Image(controller_outline,{659,333});
    }

    for(int i = 0; i < 8; ++i) {
        ImGui::SetCursorPos(controller_button_positions[i]);
        ImGui::Button(controller_button_strings[i]);
    }

    ImGui::End();
    return {659, 333};
}