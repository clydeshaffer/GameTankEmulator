#include "debug_window.h"

#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "../emulator_config.h"

DebugWindow::DebugWindow():BaseWindow(10, 10) {
    ImGuiContext* oldCtx = ImGui::GetCurrentContext();
    ImPlotContext* oldPlotCtx = ImPlot::GetCurrentContext();
    ctx = ImGui::CreateContext();
    plot_ctx = ImPlot::CreateContext();

    ImGui::SetCurrentContext(ctx);
    ImPlot::SetCurrentContext(plot_ctx);

    ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigViewportsNoDecoration = false;
	io.ConfigViewportsNoAutoMerge = true;
    io.IniFilename = NULL;
	ImGui::StyleColorsDark();

    renderer = SDL_CreateRenderer(window, -1, EmulatorConfig::defaultRendererFlags);
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
	ImGui_ImplSDLRenderer2_Init(renderer);

    ImGui::SetCurrentContext(oldCtx);
    ImPlot::SetCurrentContext(oldPlotCtx);
}

DebugWindow::~DebugWindow() {
    ImPlot::DestroyContext(plot_ctx);
    ImGui::DestroyContext(ctx);
    SDL_DestroyRenderer(renderer);
}

void DebugWindow::Draw() {
    ImGui::SetCurrentContext(ctx);
    ImPlot::SetCurrentContext(plot_ctx);

    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();
    ImVec2 size = Render();

    if(size_dirty) {
        SDL_SetWindowSize(window, size.x, size.y);
        size_dirty = false;
    }

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    ImGui::Render();
    ImGuiIO io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
    SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
    SDL_SetRenderDrawColor(renderer, (Uint8)(clear_color.x * 255), (Uint8)(clear_color.y * 255), (Uint8)(clear_color.z * 255), (Uint8)(clear_color.w * 255));
    SDL_RenderClear(renderer);
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
    SDL_RenderPresent(renderer);
}

void DebugWindow::HandleEvent(SDL_Event& e) {
    if(e.type == SDL_WINDOWEVENT) {
        if(e.window.event == SDL_WINDOWEVENT_CLOSE) {
            SDL_Window* closedWindow = SDL_GetWindowFromID(e.window.windowID);
            if(closedWindow == window) {
                open = false;
            }
        }
    }

    if(SDL_GetMouseFocus() != window) return;
    ImGui::SetCurrentContext(ctx);
    ImPlot::SetCurrentContext(plot_ctx);
    ImGui_ImplSDL2_ProcessEvent(&e);
}