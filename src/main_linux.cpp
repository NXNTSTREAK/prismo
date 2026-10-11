// Linux front end: SDL3 window + SDL_Renderer.
#include <SDL3/SDL.h>

#include <cstdio>
#include <cstring>

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include "prismo.h"
#include "ui.h"

static int cli_apply() {
    std::string err;
    if (!prismo::apply(prismo::load_settings(), err)) {
        std::fprintf(stderr, "%s\n", err.c_str());
        return 1;
    }
    return 0;
}

int main(int argc, char** argv) {
    if (argc > 1) {
        if (!std::strcmp(argv[1], "--apply")) return cli_apply();
        std::fprintf(stderr, "Usage: prismo [--apply]\n  --apply  re-apply the saved settings and exit\n");
        return 2;
    }

    SDL_SetHint(SDL_HINT_APP_ID, "prismo");  // matches prismo.desktop (launcher icon)
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    const int base_w = 480, base_h = 540;
    SDL_Window* win = SDL_CreateWindow("Prismo", base_w, base_h, SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!win) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return 1;
    }
    // On Wayland the window is already in logical units (pixel density covers HiDPI);
    // on X11 we have to scale the UI ourselves.
    float scale = SDL_GetWindowDisplayScale(win) / SDL_GetWindowPixelDensity(win);
    if (!(scale > 0)) scale = 1;
    SDL_SetWindowSize(win, int(base_w * scale), int(base_h * scale));
    SDL_SetWindowResizable(win, false);  // fixed size -> tiling WMs float it

    SDL_Renderer* ren = SDL_CreateRenderer(win, nullptr);
    if (!ren) {
        std::fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        return 1;
    }
    SDL_SetRenderVSync(ren, 1);
    SDL_ShowWindow(win);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ui::setup(scale, SDL_GetSystemTheme() != SDL_SYSTEM_THEME_LIGHT);
    ImGui_ImplSDL3_InitForSDLRenderer(win, ren);
    ImGui_ImplSDLRenderer3_Init(ren);

    bool running = true, animating = true;
    int frames_left = 3;  // keep drawing a few frames after input so ImGui settles
    while (running) {
        if (!animating && frames_left <= 0) SDL_WaitEventTimeout(nullptr, 500);
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            ImGui_ImplSDL3_ProcessEvent(&e);
            if (e.type == SDL_EVENT_QUIT || e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) running = false;
            frames_left = 3;
        }
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        animating = ui::frame(true);
        ImGui::Render();

        ImGuiIO& io = ImGui::GetIO();
        SDL_SetRenderScale(ren, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        ImVec4 bg = ui::background();
        SDL_SetRenderDrawColorFloat(ren, bg.x, bg.y, bg.z, 1);
        SDL_RenderClear(ren);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), ren);
        SDL_RenderPresent(ren);
        --frames_left;
    }

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
