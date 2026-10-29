/* SDL implementation and slice-driver support. Application ports use dzplat.h. */
#ifndef DAZZLE_PLATFORM_INTERNAL_H
#define DAZZLE_PLATFORM_INTERNAL_H
#include "dzplat.h"
#include <SDL3/SDL.h>

/* Initial period-speed estimate: one base plot/read work unit, plus delay work.
 * This is deliberately the only work-rate calibration constant. */
/* Work-unit model calibrated against the original kernels under DOSBox-X at 486DX2-66 speed
 * (docs/native-pacing.md): about 65,300 plot callbacks per second. */
#define DZ_WORK_UNITS_PER_SECOND 131311000u
#define DZ_NS_PER_SECOND UINT64_C(1000000000)

/* Presentation and window policy (docs/native-render.md). Application code never sees these. */
enum dz_presentation {
    DZ_PRESENT_ORIGINAL,  /* original 4:3 display geometry, pillar/letterbox (fidelity reference, default) */
    DZ_PRESENT_CROP,      /* uniform scale to cover the window, cropping the overflow */
    DZ_PRESENT_STRETCH    /* non-uniform fill (distorts circles; explicit user choice only) */
};
enum dz_renderer {
    DZ_RENDERER_AUTO,     /* indexed GPU path when available, else the RGBA reference path */
    DZ_RENDERER_INDEXED,  /* index texture + palette, lookup and scaling on the GPU */
    DZ_RENDERER_RGBA      /* dz_video_rgba() on the CPU (reference / fallback) */
};
struct dz_display_options {
    int screensaver;                  /* /s: fullscreen, hidden cursor, exit on genuine user activity */
    void *preview_parent;             /* /p HWND: render into a child of this window, ignore input */
    int fullscreen;                   /* start fullscreen (developer build: Alt+Enter toggles) */
    int window_width, window_height;  /* developer initial window size; 0 = default */
    enum dz_presentation presentation;
    enum dz_renderer renderer;
};
extern struct dz_display_options dz_display;
SDL_Window *dz_create_window(void);   /* window.c: the window described by dz_display */

int dz_initialize(void);
void dz_clock_reset(void);
uint64_t dz_virtual_ns(void);
void dz_present(void);
void dz_present_invalidate(void); /* window exposed/resized; redraw on the next clock presentation */
SDL_FRect dz_present_viewport(int width, int height, enum dz_presentation policy);
/* Classic raster -> presentation seam; a later Hybrid raster can use the same viewport.
 * The presenter borrows the renderer. draw returns -1 on error, 0 unchanged, 1 drawn;
 * the caller presents (or reads an offscreen render target) only after 1. */
struct dz_presenter;
struct dz_presenter *dz_presenter_create(SDL_Renderer *renderer, enum dz_renderer path);
void dz_presenter_destroy(struct dz_presenter *presenter);
int dz_presenter_draw(struct dz_presenter *presenter, enum dz_presentation policy, int force);
enum dz_renderer dz_presenter_path(const struct dz_presenter *presenter);
uint64_t dz_presented_frames(void);
int dz_save_screenshot(const char *filename);
void dz_video_rgba(uint32_t *pixels); /* width*height; text uses 720*400 */
uint32_t dz_framebuffer_hash(void);
uint32_t dz_palette_hash(void);
void dz_video_shutdown(void);
void dz_toggle_fullscreen(void);
void dz_input_reset(void);
void dz_input_poll(void);
void dz_input_event(const SDL_Event *event);
int dz_input_push(uint16_t key);
uint16_t dz_key_from_sdl(SDL_Scancode key, SDL_Keymod modifiers);
void dz_sound_shutdown(void);
void dz_audio_service(void);
void dz_text_putc(uint8_t character);
void dz_text_write(const char *text);
void dz_text_attribute(uint8_t attribute);
int dz_slice_begin(const char *name);
void dz_slice_step(void);
void dz_slice_end(void);
#endif
