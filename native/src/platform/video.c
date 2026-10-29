/* Indexed display memory and SDL presentation. Font: Unscii-16 by viznut,
 * https://github.com/viznut/unscii/blob/main/fontfiles/unscii-16.hex
 * Public Domain / CC0, per https://github.com/viznut/unscii/blob/main/README.md
 * (not the GPL unscii-16-full variant). See font_cp437.h for glyph provenance. */
#include "platform.h"
#include "hybrid.h"
#include <string.h>
#include "font_cp437.h"

extern void dzconio_cursor_rows(int *start, int *end);

struct dz_video dz_video;
static const uint8_t text_palette[16] = {0,1,2,3,4,5,20,7,56,57,58,59,60,61,62,63};

static uint32_t ega_rgb(uint8_t color)
{
    unsigned r = ((color >> 2) & 1u) * 170u + ((color >> 5) & 1u) * 85u;
    unsigned g = ((color >> 1) & 1u) * 170u + ((color >> 4) & 1u) * 85u;
    unsigned b = (color & 1u) * 170u + ((color >> 3) & 1u) * 85u;
    return UINT32_C(0xff000000) | (r << 16) | (g << 8) | b;
}

void dz_video_set_mode(enum dz_video_mode mode, int clear_memory)
{
    if (dz_hybrid_enabled()) dz_hybrid_retire();
    dz_video.mode = mode;
    dz_video.width = mode == DZ_MODE_TEXT ? 80 : mode == DZ_MODE_EGA_350 ? 640 : 320;
    dz_video.height = mode == DZ_MODE_TEXT ? 25 : mode == DZ_MODE_EGA_350 ? 350 : mode == DZ_MODE_VGA_200 ? 200 : 400;
    dz_video.pixels_per_byte = mode == DZ_MODE_EGA_350 ? 8 : mode == DZ_MODE_VGA_400 ? 4 : 1;
    dz_video.display_start_row = 0;
    dz_video.line_compare = (unsigned)dz_video.height;
    dz_video.ega_xor = 0;
    dz_video.ega_border = 0;
    memcpy(dz_video.ega_attr, text_palette, 16);
    for (unsigned i = 0; i < 256; ++i) {
        uint32_t rgb = ega_rgb(text_palette[i & 15u]);
        dz_video.dac[i][0] = (uint8_t)(((rgb >> 16) & 255u) * 63u / 255u);
        dz_video.dac[i][1] = (uint8_t)(((rgb >> 8) & 255u) * 63u / 255u);
        dz_video.dac[i][2] = (uint8_t)((rgb & 255u) * 63u / 255u);
    }
    if (clear_memory) {
        if (dz_hybrid_enabled()) {
            for (uint32_t a = 0; a < DZ_VRAM_PIXELS; ++a) {
                uint8_t old = dz_video.vram[a];
                dz_video.vram[a] = 0;
                dz_hybrid_write(a, old, 0, DZ_WRITE_BULK, 0);
            }
        } else memset(dz_video.vram, 0, sizeof dz_video.vram);
        for (int y = 0; y < 25; ++y)
            for (int x = 0; x < 80; ++x) {
                dz_video.text[y][x][0] = ' ';
                dz_video.text[y][x][1] = 7;
            }
        dz_video.cursor_x = dz_video.cursor_y = 0;
        dz_video.cursor_visible = 1;
    }
}

void dz_video_rgba(uint32_t *pixels)
{
    if (dz_video.mode == DZ_MODE_TEXT) {
        int blink_on = ((dz_virtual_ns() / UINT64_C(500000000)) & 1u) == 0;
        int cursor_start, cursor_end;
        dzconio_cursor_rows(&cursor_start, &cursor_end);
        for (int y = 0; y < 400; ++y)
            for (int x = 0; x < 720; ++x) {
                uint8_t ch = dz_video.text[y / 16][x / 9][0];
                uint8_t attr = dz_video.text[y / 16][x / 9][1];
                int bit = x % 9 < 8 ? 7 - x % 9 : 0;
                int ink = (x % 9 < 8 || (ch >= 0xc0 && ch <= 0xdf)) &&
                    ((cp437_8x16[ch][y % 16] >> bit) & 1);
                if ((attr & 128u) && !blink_on) ink = 0;
                if (dz_video.cursor_visible && blink_on && y / 16 == dz_video.cursor_y &&
                    x / 9 == dz_video.cursor_x && y % 16 >= cursor_start &&
                    y % 16 <= cursor_end) ink = 1;
                pixels[y * 720 + x] = ega_rgb(text_palette[ink ? attr & 15u : (attr >> 4) & 7u]);
            }
        return;
    }
    uint32_t palette[256];
    for (unsigned i = 0; i < 256; ++i) {
        if (dz_video.mode == DZ_MODE_EGA_350) palette[i] = ega_rgb(dz_video.ega_attr[i & 15u]);
        else {
            unsigned r = (dz_video.dac[i][0] & 63u) * 255u / 63u;
            unsigned g = (dz_video.dac[i][1] & 63u) * 255u / 63u;
            unsigned b = (dz_video.dac[i][2] & 63u) * 255u / 63u;
            palette[i] = UINT32_C(0xff000000) | (r << 16) | (g << 8) | b;
        }
    }
    uint32_t capacity = DZ_VRAM_PLANE_BYTES * (uint32_t)dz_video.pixels_per_byte;
    for (unsigned y = 0; y < (unsigned)dz_video.height; ++y) {
        unsigned row = y > dz_video.line_compare ? y - dz_video.line_compare - 1u : dz_video.display_start_row + y;
        for (unsigned x = 0; x < (unsigned)dz_video.width; ++x) {
            uint32_t address = (row * (unsigned)dz_video.width + x) % capacity;
            pixels[y * (unsigned)dz_video.width + x] = palette[dz_video.vram[address]];
        }
    }
}

static uint32_t fnv(const uint8_t *bytes, size_t length)
{
    uint32_t hash = UINT32_C(2166136261);
    for (size_t i = 0; i < length; ++i) hash = (hash ^ bytes[i]) * UINT32_C(16777619);
    return hash;
}
uint32_t dz_framebuffer_hash(void) { return fnv(dz_video.vram, sizeof dz_video.vram); }
uint32_t dz_palette_hash(void) { return fnv(&dz_video.dac[0][0], sizeof dz_video.dac); }
