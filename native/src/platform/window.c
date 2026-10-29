/* Window creation for the developer program and the screensaver (dz_display policy). */
#include "platform.h"

struct dz_display_options dz_display = {0, NULL, 0, 0, 0, DZ_PRESENT_ORIGINAL, DZ_RENDERER_AUTO};

SDL_Window *dz_create_window(void)
{
    int width = dz_display.window_width > 0 ? dz_display.window_width : 960;
    int height = dz_display.window_height > 0 ? dz_display.window_height : 720;
    SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE;
    if (dz_display.fullscreen) flags |= SDL_WINDOW_FULLSCREEN;
    return SDL_CreateWindow("DAZZLE 5.0", width, height, flags);
}
