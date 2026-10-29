/* SDL implementation and slice-driver support. Application ports use dzplat.h. */
#ifndef DAZZLE_PLATFORM_INTERNAL_H
#define DAZZLE_PLATFORM_INTERNAL_H
#include "dzplat.h"
#include <SDL3/SDL.h>

/* Initial period-speed estimate: one base plot/read work unit, plus delay work.
 * This is deliberately the only work-rate calibration constant. */
#define DZ_WORK_UNITS_PER_SECOND 12000u
#define DZ_NS_PER_SECOND UINT64_C(1000000000)

int dz_initialize(void);
void dz_clock_reset(void);
uint64_t dz_virtual_ns(void);
void dz_present(void);
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
