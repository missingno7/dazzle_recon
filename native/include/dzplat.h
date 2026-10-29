/* Native platform interface for the DAZZLE 5.0 SDL3 port (docs/native.md).
 *
 * The ported application code (src/app/*.c, src/dvdt.c) talks to SDL only through this header.
 * Everything here is single-threaded: call it from the thread that runs the program.
 */
#ifndef DAZZLE_DZPLAT_H
#define DAZZLE_DZPLAT_H

#include <stdint.h>
#include <stddef.h>

/* ---- Video memory --------------------------------------------------------------------------
 * Display memory is one linear indexed array: pixel (row, x) = dz_video.vram[row * width + x].
 * VGA unchained (plane x&3, offset 80*y + x/4) and EGA planar (offset 80*y + x/8) both map to this
 * layout; one VGA/EGA plane offset unit (byte) = 4 (VGA) or 8 (EGA) pixels.
 */
enum dz_video_mode {
    DZ_MODE_TEXT,      /* 80x25 colour text (BIOS mode 3) */
    DZ_MODE_VGA_400,   /* unchained 320x400x256, two pages (DAZZLE VGA default) */
    DZ_MODE_VGA_200,   /* mode 13h 320x200x256 (VGA /T) */
    DZ_MODE_EGA_350    /* 640x350x16 (EGA, default 640x700 software coordinates or /T 640x350) */
};

#define DZ_VRAM_PLANE_BYTES 65536u                 /* bytes per plane addressed by the CRTC */
#define DZ_VRAM_PIXELS (DZ_VRAM_PLANE_BYTES * 8u)  /* enough for EGA (8 px/byte) and VGA (4 px/byte) */

struct dz_video {
    enum dz_video_mode mode;
    int width;                  /* pixels per row: 320 or 640 (text: 80 cells) */
    int height;                 /* visible physical rows: 400, 200, 350 (text: 25) */
    int pixels_per_byte;        /* 4 (VGA unchained), 1 (mode 13h), 8 (EGA): plane-offset -> pixel scale */
    uint8_t vram[DZ_VRAM_PIXELS];
    unsigned display_start_row; /* CRTC start address / 80 (set_display_start) */
    unsigned line_compare;      /* physical row where the display restarts at address 0; >= height = off */
    uint8_t dac[256][3];        /* VGA DAC, 6-bit components */
    uint8_t ega_attr[16];       /* EGA attribute palette: pixel value -> 6-bit EGA colour (rgbRGB) */
    uint8_t ega_border;         /* EGA overscan colour */
    int ega_xor;                /* EGA graphics-controller raster op: 0 replace, 1 XOR */
    /* 80x25 text console (mode 3): character, attribute */
    uint8_t text[25][80][2];
    int cursor_x, cursor_y, cursor_visible;
};

extern struct dz_video dz_video;

void dz_video_set_mode(enum dz_video_mode mode, int clear_memory);
/* Linear pixel address of the given byte offset (plane offset) within the current mode. */
static inline uint32_t dz_vram_index(uint32_t plane_offset) {
    return plane_offset * (uint32_t)dz_video.pixels_per_byte;
}

/* ---- Time ----------------------------------------------------------------------------------
 * Virtual time advances only through these calls (docs/native.md "Execution and time"). Crossing a
 * BIOS timer tick runs the registered tick handler; crossing an event-poll point drains SDL input and
 * presents a frame when one is due. In real-time mode the call may sleep; never in deterministic mode.
 */
#define DZ_BIOS_TICK_HZ 18.2065

void dz_charge(uint32_t work_units);  /* account for work done (plots, reads, polls) */
void dz_wait_retrace(void);           /* advance to the next 70 Hz vertical retrace */
void dz_delay_ms(unsigned ms);        /* Borland delay() */
void dz_service(void);                /* explicit poll point inside waits on ISR-updated variables */
uint64_t dz_virtual_ticks(void);      /* BIOS ticks elapsed since start */
uint32_t dz_bios_time(void);          /* BIOS 0040:006C equivalent (ticks since midnight, wraps daily) */

/* Handlers the application registers (ported ISR bodies; NULL = none). */
extern void (*dz_timer_tick_handler)(void);    /* INT 1Ch */
extern void (*dz_keyboard_irq_handler)(void);  /* INT 9 effect, called per key press/release */
extern void (*dz_mouse_event_handler)(void);   /* INT 33h user callback effect */

/* ---- Keyboard (BIOS buffer semantics) --------------------------------------------------------- */
uint16_t dz_bioskey(int command);  /* Borland bioskey: 0 read (waits), 1 peek (0 if empty), 2 shift flags */
int dz_kbhit(void);

/* ---- Mouse ------------------------------------------------------------------------------------ */
int dz_mouse_present(void);
void dz_mouse_counters(int *dx, int *dy, unsigned *buttons);

/* ---- PC speaker ------------------------------------------------------------------------------- */
void dz_sound(unsigned hz);   /* Borland sound(): start a square wave */
void dz_nosound(void);

/* ---- Random numbers (Borland C++ 3.0 RTL compatible) ------------------------------------------ */
void dz_srand(uint32_t seed);
int16_t dz_rand(void);                     /* 0..32767 */
/* Borland random(num) macro: (int)(((long)rand()*(num))/(RAND_MAX+1)). `n` must already be the value of
 * the original 16-bit argument expression in its original type: pass (int16_t)expr for an int argument,
 * (uint16_t)expr for an unsigned one (values 32768..65535 stay positive), or a 32-bit value for a long. */
static inline int16_t dz_random(int32_t n)
{ return (int16_t)(((int32_t)dz_rand() * n) / 32768L); }
uint64_t dz_rand_calls(void);              /* number of dz_rand() calls so far (divergence tracing) */

/* ---- Run configuration ------------------------------------------------------------------------ */
struct dz_options {
    int deterministic;          /* no wall clock, no sleeping; seed from `seed` */
    uint32_t seed;
    int headless;               /* no window (tests) */
    double speed;               /* work-unit rate multiplier (1.0 = calibrated original speed) */
    const char *original_exe;   /* original DAZZLE.EXE for the appended manual/logo resources */
};
extern struct dz_options dz_options;

/* Called by the program at exit paths; flushes audio, closes the window. */
void dz_shutdown(void);

#endif
