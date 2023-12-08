#include "../imgui/imgui.h"
#include "profiler_window.h"
#include "implot.h"

static float prof_R[8] = {1, 1, 1, 0, 0, 0.5f, 0.5f, 1};
static float prof_G[8] = {0, 0.5f, 1, 1, 0, 0, 0.5f, 1};
static float prof_B[8] = {0, 0, 0, 0, 1, 0.5f, 0.5f, 1};

void ProfilerWindow::Draw() {
    ImGui::Begin("Profiler", NULL, ImGuiWindowFlags_None);
    ImGui::Text("FPS: %d", _profiler.fps);
    

     
    if(ImPlot::BeginPlot("Timer history")) {
        ImPlot::SetupAxes("", "");
        ImPlot::SetupAxesLimits(0,256, 0, 2, ImPlotCond_Always);
        for(int i = 0; i < PROFILER_ENTRIES; ++i) {
            ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(prof_R[i % 8], prof_G[i % 8], prof_B[i % 8], 1.0f));
            ImPlot::PlotLine<float>("", _profiler.profilingHistory[i], PROFILER_HISTORY, 1, 0, 0, _profiler.history_num );
        }
        ImPlot::EndPlot();
    }
    
    ImGui::BeginChild("Scrolling");
    for(int i = 0; i < PROFILER_ENTRIES; ++i) {
        if(_profiler.profilingTimes[i] != 0) {
            ImGui::TextColored(ImVec4(prof_R[i % 8], prof_G[i % 8], prof_B[i % 8], 1.0f), 
            "Timer %02d: %d / %d", i, _profiler.profilingTimes[i], _profiler.profilingCounts[i]);
        }
    }
    ImGui::EndChild();
    
    ImGui::End();
}


/*

void drawProfilingWindow() {
	SDL_Rect profilerArea, graphRect;
	profilerArea.x = 0;
	profilerArea.y = 0;
	profilerArea.w = PROFILER_WIDTH;
	profilerArea.h = BMP_CHAR_SIZE;

	graphRect.x = profiler_x_axis;
	graphRect.y = 0;
	graphRect.w = 1;
	graphRect.h = PROFILER_HEIGHT;

	char buf[64];

	SDL_FillRect(profilerSurface, &graphRect, SDL_MapRGB(profilerSurface->format, 0, 0, 0));
	graphRect.h = 1;
	graphRect.y = PROFILER_HEIGHT - 128;
	SDL_FillRect(profilerSurface, &graphRect, SDL_MapRGB(profilerSurface->format, 32, 32, 32));
	
	sprintf(buf,"FPS: %d\n", fps);
	SDL_FillRect(profilerSurface, &profilerArea, SDL_MapRGB(profilerSurface->format, 0, 0, 0));
	drawText(profilerSurface, &profilerArea, buf);
	profilerArea.y += BMP_CHAR_SIZE;

	sprintf(buf,"ACP: %4d / 1024\n", soundcard->get_irq_cycle_count());
	SDL_FillRect(profilerSurface, &profilerArea, SDL_MapRGB(profilerSurface->format, 0, 0, 0));
	drawText(profilerSurface, &profilerArea, buf);
	profilerArea.y += BMP_CHAR_SIZE;

	for(int i = 0; i < PROFILER_ENTRIES; ++i) {
		Uint32 id_col = SDL_MapRGB(profilerSurface->format, prof_R[i % 8], prof_G[i % 8], prof_B[i % 8]);
		if(profilingTimes[i] != 0) {
			sprintf(buf,"Timer %d: %04llu - %llu\n", i, profilingCounts[i], profilingTimes[i]);
			profilerArea.x = 0;
			profilerArea.w = 2;
			SDL_FillRect(profilerSurface, &profilerArea, id_col);
			profilerArea.x = 4;
			profilerArea.w = PROFILER_WIDTH-4;
			SDL_FillRect(profilerSurface, &profilerArea, SDL_MapRGB(profilerSurface->format, 0, 0, 0));
			drawText(profilerSurface, &profilerArea, buf);

			if(profilingTimes[i] < 100000) {
				graphRect.y = PROFILER_HEIGHT - (profilingTimes[i] >> 9);
			} else {
				graphRect.y = 128;
			}
			SDL_FillRect(profilerSurface, &graphRect, id_col);
		}
		profilerArea.y += BMP_CHAR_SIZE;
		
	}
	
	profiler_x_axis = (profiler_x_axis + 1) % PROFILER_WIDTH;
}*/