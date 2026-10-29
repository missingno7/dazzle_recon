/* Window creation for the developer program and the screensaver (dz_display policy). */
#include "platform.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>
#endif

struct dz_display_options dz_display = {0, NULL, 0, 0, 0, DZ_PRESENT_ORIGINAL, DZ_RENDERER_AUTO};

#ifdef _WIN32
struct black_display {
    SDL_Window *window;
    SDL_Renderer *renderer;
};
static struct black_display *black_displays;
static int black_display_count;
static HWND preview_child;
static SDL_Window *saver_window;

static HANDLE set_parent_dpi_context(HWND parent)
{
    HMODULE user32 = GetModuleHandleA("user32.dll");
    FARPROC get_proc = GetProcAddress(user32, "GetWindowDpiAwarenessContext");
    FARPROC set_proc = GetProcAddress(user32, "SetThreadDpiAwarenessContext");
    HANDLE (WINAPI *get_context)(HWND) = NULL;
    HANDLE (WINAPI *set_context)(HANDLE) = NULL;
    if (sizeof get_context != sizeof get_proc || sizeof set_context != sizeof set_proc) return NULL;
    memcpy(&get_context, &get_proc, sizeof get_context);
    memcpy(&set_context, &set_proc, sizeof set_context);
    if (!get_context || !set_context) return NULL;
    HANDLE context = get_context(parent);
    return context ? set_context(context) : NULL;
}

static void restore_dpi_context(HANDLE previous)
{
    if (!previous) return;
    HMODULE user32 = GetModuleHandleA("user32.dll");
    FARPROC set_proc = GetProcAddress(user32, "SetThreadDpiAwarenessContext");
    HANDLE (WINAPI *set_context)(HANDLE) = NULL;
    if (sizeof set_context != sizeof set_proc) return;
    memcpy(&set_context, &set_proc, sizeof set_context);
    if (set_context) set_context(previous);
}
#endif
static int first_window_mode = -1;

static SDL_Window *create_black_display(const SDL_Rect *bounds)
{
    SDL_Window *window = SDL_CreateWindow("DAZZLE 5.0", bounds->w, bounds->h,
        SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALWAYS_ON_TOP | SDL_WINDOW_NOT_FOCUSABLE |
        SDL_WINDOW_HIDDEN);
    if (!window) return NULL;
    SDL_SetWindowPosition(window, bounds->x, bounds->y);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        SDL_DestroyWindow(window);
        return NULL;
    }
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
    SDL_ShowWindow(window);
#ifdef _WIN32
    black_displays[black_display_count++] = (struct black_display){window, renderer};
#endif
    return window;
}

static void create_secondary_black_displays(SDL_DisplayID primary)
{
#ifdef _WIN32
    int count = 0;
    SDL_DisplayID *displays = SDL_GetDisplays(&count);
    if (!displays || count < 2) {
        SDL_free(displays);
        return;
    }
    black_displays = SDL_calloc((size_t)count - 1, sizeof *black_displays);
    if (!black_displays) {
        SDL_free(displays);
        return;
    }
    for (int i = 0; i < count; ++i) {
        SDL_Rect bounds;
        if (displays[i] != primary && SDL_GetDisplayBounds(displays[i], &bounds))
            create_black_display(&bounds);
    }
    SDL_free(displays);
#else
    (void)primary;
#endif
}

SDL_Window *dz_create_window(void)
{
    extern struct dz_video dz_video;
    if (first_window_mode < 0) first_window_mode = (int)dz_video.mode;
#ifdef _WIN32
    if (dz_display.preview_parent) {
        HWND parent = (HWND)dz_display.preview_parent;
        if (!IsWindow(parent)) return NULL;
        HANDLE previous_context = set_parent_dpi_context(parent);
        RECT client;
        if (!GetClientRect(parent, &client)) {
            restore_dpi_context(previous_context);
            return NULL;
        }
        preview_child = CreateWindowExA(0, "STATIC", "", WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
            0, 0, client.right, client.bottom, parent, NULL, GetModuleHandleA(NULL), NULL);
        if (!preview_child) {
            restore_dpi_context(previous_context);
            return NULL;
        }
        SDL_PropertiesID properties = SDL_CreateProperties();
        SDL_SetPointerProperty(properties, SDL_PROP_WINDOW_CREATE_WIN32_HWND_POINTER, preview_child);
        SDL_SetStringProperty(properties, SDL_PROP_WINDOW_CREATE_TITLE_STRING, "DAZZLE 5.0");
        SDL_SetNumberProperty(properties, SDL_PROP_WINDOW_CREATE_X_NUMBER, 0);
        SDL_SetNumberProperty(properties, SDL_PROP_WINDOW_CREATE_Y_NUMBER, 0);
        SDL_SetNumberProperty(properties, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, client.right);
        SDL_SetNumberProperty(properties, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, client.bottom);
        SDL_SetBooleanProperty(properties, SDL_PROP_WINDOW_CREATE_BORDERLESS_BOOLEAN, true);
        SDL_SetBooleanProperty(properties, SDL_PROP_WINDOW_CREATE_HIGH_PIXEL_DENSITY_BOOLEAN, false);
        SDL_Window *window = SDL_CreateWindowWithProperties(properties);
        SDL_DestroyProperties(properties);
        if (!window) {
            DestroyWindow(preview_child);
            preview_child = NULL;
        } else {
            SetWindowPos(preview_child, NULL, 0, 0, client.right, client.bottom,
                SWP_NOZORDER | SWP_NOACTIVATE);
            ShowWindow(preview_child, SW_SHOW);
        }
        restore_dpi_context(previous_context);
        return window;
    }
    if (dz_display.screensaver) {
        SDL_DisplayID primary = SDL_GetPrimaryDisplay();
        SDL_Rect bounds = {0, 0, 640, 480};
        SDL_GetDisplayBounds(primary, &bounds);
        SDL_Window *window = SDL_CreateWindow("DAZZLE 5.0", bounds.w, bounds.h,
            SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALWAYS_ON_TOP | SDL_WINDOW_FULLSCREEN);
        if (!window) return NULL;
        saver_window = window;
        SDL_SetWindowPosition(window, bounds.x, bounds.y);
        SDL_HideCursor();
        create_secondary_black_displays(primary);
        SDL_RaiseWindow(window);
        return window;
    }
#endif
    int width = dz_display.window_width > 0 ? dz_display.window_width : 960;
    int height = dz_display.window_height > 0 ? dz_display.window_height : 720;
    SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE;
    if (dz_display.fullscreen) flags |= SDL_WINDOW_FULLSCREEN;
    return SDL_CreateWindow("DAZZLE 5.0", width, height, flags);
}

int dz_preview_parent_alive(void)
{
#ifdef _WIN32
    return !dz_display.preview_parent || IsWindow((HWND)dz_display.preview_parent);
#else
    return 1;
#endif
}

void dz_window_activity_dismiss(const SDL_Event *event)
{
#ifdef _WIN32
    if (!dz_display.screensaver) return;
    Uint32 id = 0;
    if (event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_KEY_UP)
        id = event->key.windowID;
    else if (event->type == SDL_EVENT_MOUSE_MOTION)
        id = event->motion.windowID;
    else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN || event->type == SDL_EVENT_MOUSE_BUTTON_UP)
        id = event->button.windowID;
    else if (event->type == SDL_EVENT_WINDOW_FOCUS_LOST || event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
        id = event->window.windowID;
    SDL_Window *window = saver_window ? saver_window : id ? SDL_GetWindowFromID(id) : SDL_GetKeyboardFocus();
    if (window) SDL_HideWindow(window);
    for (int i = 0; i < black_display_count; ++i)
        if (black_displays[i].window) SDL_HideWindow(black_displays[i].window);
#else
    (void)event;
#endif
}

int dz_window_first_mode(void)
{
    return first_window_mode;
}

int dz_window_saver_focused(void)
{
#ifdef _WIN32
    return saver_window && (SDL_GetWindowFlags(saver_window) & SDL_WINDOW_INPUT_FOCUS) != 0;
#else
    return 0;
#endif
}

void dz_window_preview_cleanup(void)
{
#ifdef _WIN32
    if (preview_child && IsWindow(preview_child)) DestroyWindow(preview_child);
    preview_child = NULL;
    SDL_free(black_displays);
    black_displays = NULL;
    black_display_count = 0;
    saver_window = NULL;
#endif
}
