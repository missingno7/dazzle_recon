/* Native C translation of asm/dvdt.asm, in original entry order.
 * Expected behaviour comes from that assembly, not a candidate framebuffer.
 * Hardware state is expressed through dzplat.h's indexed linear memory model.
 */
#include "dazzle.h"
#include "dvdt.h"

uint16_t video_mode_cookie;
uint8_t symmetry_edge, symmetry_y_edge;
uint8_t disk_count;
uint16_t first_disk_cylinders, second_disk_cylinders;
uint16_t input_poll_divider = 5;
uint16_t force_ega, force_vga, video_adapter, saved_bios_mode, plot_color;
uint16_t saved_cursor_shape, saved_text_last_row, ega_memory_code, page_paragraph;
uint16_t plasma_random, plasma_width, plasma_height, plasma_color_limit;
uint16_t ega_row_offsets[350];

/* Explicit signed interpretation and arithmetic shifts avoid host promotion /
 * negative-shift differences. Every arithmetic operand here was a DOS word. */
static int16_t signed_word(uint16_t word)
{
    return word <= INT16_MAX ? (int16_t)word : (int16_t)((int32_t)word - 65536);
}

static int16_t sar_word(uint16_t word, unsigned bits)
{
    int32_t value = signed_word(word);
    int32_t divisor = (int32_t)(1u << bits);
    return (int16_t)(value >= 0 ? value / divisor : -((-value + divisor - 1) / divisor));
}

/* DI wraps BEFORE addition of the page segment base. Beyond the 64 KiB
 * aperture stores are ignored (and reads return zero); never index host RAM.
 * The page base itself must not alias page zero when DI wraps. */
static uint32_t pixel_index(uint16_t offset, unsigned lane, unsigned pixels_per_byte)
{
    uint32_t address = (uint32_t)page_paragraph * 16u + offset;
    return address < DZ_VRAM_PLANE_BYTES
        ? address * pixels_per_byte + lane : DZ_VRAM_PIXELS;
}

static uint16_t chained_offset(uint16_t x, uint16_t y)
{
    /* The assembly uses XCHG AH,AL, then adds AX and AX>>2; 320*y is
     * equivalent only for the ordinary rows 0..255. Retain the byte swap. */
    uint16_t swapped_y = (uint16_t)((uint16_t)(y << 8) | (y >> 8));
    return (uint16_t)(x + swapped_y + (swapped_y >> 2));
}

static void store_pixel(uint16_t x, uint16_t y, unsigned pixels_per_byte, uint8_t color)
{
    uint16_t offset;
    unsigned lane;
    if (pixels_per_byte == 1) {
        offset = chained_offset(x, y);
        lane = 0;
    } else {
        offset = (uint16_t)(80u * y + x / pixels_per_byte);
        lane = x % pixels_per_byte;
    }
    uint32_t index = pixel_index(offset, lane, pixels_per_byte);
    if (index >= DZ_VRAM_PIXELS)
        return;
    if (pixels_per_byte == 8) {
        color &= 15u;
        if (dz_video.ega_xor)
            color ^= dz_video.vram[index] & 15u;
    }
    dz_video.vram[index] = color;
}

static void plot_pass(int16_t x, int16_t y, uint16_t symmetry,
                      unsigned pixels_per_byte, int halve_y, uint8_t color)
{
    symmetry_edge = symmetry_y_edge = 0;
    symmetry_edge = (uint8_t)(screen_half_width <= x);
    uint16_t xp = (uint16_t)(screen_half_width + x);
    if (signed_word(xp) < 0)
        return;
    uint16_t xm = (uint16_t)(screen_half_width - x);
    if (signed_word(xm) < 0)
        return;
    symmetry_y_edge = (uint8_t)(screen_half_height <= y);
    uint16_t yp = (uint16_t)(screen_half_height + y);
    if (signed_word(yp) < 0 || (halve_y && (yp & 1u)))
        return;
    uint16_t ym = (uint16_t)(screen_half_height - y);
    if (signed_word(ym) < 0 || (halve_y && (ym & 1u)))
        return;
    if (halve_y) {
        yp >>= 1;
        ym >>= 1;
    }
    if (!symmetry_edge && !symmetry_y_edge)
        store_pixel(xp, yp, pixels_per_byte, color);
    if (symmetry & 1u) {
        if (!symmetry_edge)
            store_pixel(xp, ym, pixels_per_byte, color);
        if (!symmetry_y_edge)
            store_pixel(xm, yp, pixels_per_byte, color);
        store_pixel(xm, ym, pixels_per_byte, color);
    }
}

static void plot_symmetric(uint16_t symmetry, unsigned pixels_per_byte, int halve_y)
{
    /* AND CH,1Fh is repeated before LOOP. Zero wraps through FFFF, but
     * masking on the next iteration makes the effective delay 8192, not 65536.
     * One base work unit plus one per delay iteration is the native cost. */
    uint32_t iterations = draw_delay_count & 0x1fffu;
    dz_charge(1u + (iterations ? iterations : 8192u));
    uint8_t color = (uint8_t)plot_color;
    plot_pass(draw_x, draw_y, symmetry, pixels_per_byte, halve_y, color);
    if ((symmetry & 3u) == 3u)
        plot_pass(draw_y, draw_x, 1, pixels_per_byte, halve_y, color);
}

static uint16_t read_pixel(int16_t x, int16_t y, unsigned pixels_per_byte, int halve_y)
{
    dz_charge(1);
    uint16_t px = (uint16_t)(screen_half_width + x);
    uint16_t py = (uint16_t)(screen_half_height + y);
    /* The original EGA negative branch returns incidental AL; VGA performs
     * unchecked wrapped reads. Native out-of-contract coordinates return 0.
     * Upper bounds refer to software coordinates, not the displayed page. */
    if (signed_word(px) < 0 || signed_word(py) < 0 ||
        (int32_t)px >= (int32_t)screen_half_width * 2 ||
        (int32_t)py >= (int32_t)screen_half_height * 2)
        return 0;
    if (halve_y)
        py >>= 1;
    uint16_t offset = pixels_per_byte == 1 ? chained_offset(px, py)
        : (uint16_t)(80u * py + px / pixels_per_byte);
    uint32_t index = pixel_index(offset, px % pixels_per_byte, pixels_per_byte);
    if (index >= DZ_VRAM_PIXELS)
        return 0;
    return pixels_per_byte == 8 ? dz_video.vram[index] & 15u : dz_video.vram[index];
}

static void load_dac(unsigned first, unsigned count, const uint8_t *source)
{
    for (unsigned i = 0; i < count; ++i)
        for (unsigned channel = 0; channel < 3; ++channel)
            dz_video.dac[first + i][channel] = source[3u * i + channel] & 63u;
}

/* Quantize RGB8 with strict >33h, >77h, >BBh thresholds.
 * Original 1899:0006, 134 bytes */
void rgb8_to_ega_palette(uint8_t *destination, uint8_t *source, uint16_t count)
{
    /* A zero LOOP count would consume 65536 triples; a zero-length native
     * buffer is instead a no-op. Nonzero callers retain source-index wrap. */
    uint16_t source_offset = 0;
    for (uint32_t i = 0; i < count; ++i) {
        destination[i] = 0;
        for (unsigned channel = 0; channel < 3; ++channel) {
            uint8_t value = source[(uint16_t)(source_offset + channel)];
            uint8_t primary = (uint8_t)(4u >> channel);
            uint8_t secondary = (uint8_t)(32u >> channel);
            if (value > 0xbbu)
                destination[i] |= primary | secondary;
            else if (value > 0x77u)
                destination[i] |= primary;
            else if (value > 0x33u)
                destination[i] |= secondary;
        }
        source_offset = (uint16_t)(source_offset + 3u);
    }
}

/* Expand four consecutive source planes into indexed EGA pixels.
 * Original 1899:008C, 128 bytes */
void ega_write_planar_row(uint8_t *source, uint16_t bytes_per_plane, uint16_t row)
{
    /* Negative row words originally read before the row table; safely ignore. */
    if (row >= 350)
        return;
    uint16_t source_offset = 0;
    for (unsigned plane = 0; plane < 4; ++plane) {
        uint16_t offset = ega_row_offsets[row];
        for (uint32_t column = 0; column < bytes_per_plane; ++column) {
            uint8_t bits = source[source_offset++];
            uint32_t index = (uint32_t)offset++ * 8u;
            uint8_t mask = (uint8_t)(1u << plane);
            for (unsigned bit = 0; bit < 8; ++bit) {
                uint8_t color = dz_video.vram[index + bit] & (uint8_t)~mask;
                if (bits & (0x80u >> bit))
                    color |= mask;
                dz_video.vram[index + bit] = color & 15u;
            }
        }
    }
    dz_charge((uint32_t)bytes_per_plane * 4u + 1u);
}

/* Initialize the physical row-offset table and BIOS mode 10h.
 * Original 1899:010C, 33 bytes */
void ega_init_row_table_and_mode(void)
{
    for (unsigned row = 0; row < 350; ++row)
        ega_row_offsets[row] = (uint16_t)(80u * row);
    dz_video_set_mode(DZ_MODE_EGA_350, 1);
}

/* Doubled software Y; odd translated rows are rejected, including swapped pass.
 * Original 1899:012D, 443 bytes */
void ega_plot_symmetric_640x700(uint16_t symmetry) { plot_symmetric(symmetry, 8, 1); }

/* Physical EGA Y, same literal edge/reflection order as the doubled kernel.
 * Original 1899:02E8, 419 bytes */
void ega_plot_symmetric_640x350(uint16_t symmetry) { plot_symmetric(symmetry, 8, 0); }

/* Unchained VGA, plane X&3 and 80*Y+(X>>2) within page segment.
 * Original 1899:048B, 407 bytes */
void vga_plot_symmetric_320x400(uint16_t symmetry) { plot_symmetric(symmetry, 4, 0); }

/* Chained mode 13h, with the original byte-swap Y address computation.
 * Original 1899:0622, 315 bytes */
void vga_plot_symmetric_320x200(uint16_t symmetry) { plot_symmetric(symmetry, 1, 0); }

/* Halve translated Y even when odd; native invalid reads return zero.
 * Original 1899:075D, 152 bytes */
uint16_t ega_read_pixel_640x700(int16_t x, int16_t y) { return read_pixel(x, y, 8, 1); }

/* Combine four planar bits, expressed as one indexed byte in native memory.
 * Original 1899:07F5, 150 bytes */
uint16_t ega_read_pixel_640x350(int16_t x, int16_t y) { return read_pixel(x, y, 8, 0); }

/* Read selected VGA plane; native invalid reads return zero.
 * Original 1899:088B, 102 bytes */
uint16_t vga_read_pixel_320x400(int16_t x, int16_t y) { return read_pixel(x, y, 4, 0); }

/* Read chained VGA; native invalid reads return zero.
 * Original 1899:08F1, 65 bytes */
uint16_t vga_read_pixel_320x200(int16_t x, int16_t y) { return read_pixel(x, y, 1, 0); }

/* Native restoration supports the platform's 80x25 text mode. Cursor shape
 * is represented by the conventional 06/07 scan lines plus visibility bit 5.
 * Original 1899:0932, 39 bytes */
void save_video_state(void)
{
    saved_bios_mode = dz_video.mode == DZ_MODE_TEXT ? 3u
        : dz_video.mode == DZ_MODE_EGA_350 ? 0x10u : 0x13u;
    saved_cursor_shape = dz_video.cursor_visible ? 0x0607u : 0x2607u;
    saved_text_last_row = 24;
}

/* No native adapter probe/error exits: VGA by default, /E then /V overrides.
 * The original has no defined AX result: callers inspect video_adapter.
 * Original 1899:0959, 332 bytes */
void detect_video_adapter(void)
{
    video_adapter = force_ega && !force_vga ? 4u : 8u;
    ega_memory_code = 3; /* Full EGA memory available in the software aperture. */
}

/* Policy 2 skips the BIOS operation; policy 0 preserves memory.
 * Original 1899:0AA5, 40 bytes */
void ega_set_mode(uint16_t clear_policy)
{
    video_mode_cookie = 0x41;
    if (clear_policy != 2)
        dz_video_set_mode(DZ_MODE_EGA_350, clear_policy != 0);
}

/* DI wraps after each word; the paragraph base remains fixed. All VGA planes
 * receive the byte pattern. EGA bytes expand their low four color bits.
 * Original 1899:0ACD, 47 bytes */
void clear_vram_words(uint16_t pattern, uint16_t count, uint16_t page_para)
{
    dz_wait_retrace();
    uint16_t offset = 0;
    unsigned scale = (unsigned)dz_video.pixels_per_byte;
    for (uint32_t word = 0; word < count; ++word) {
        for (unsigned byte = 0; byte < 2; ++byte) {
            uint32_t address = (uint32_t)page_para * 16u + offset;
            uint8_t color = (uint8_t)(pattern >> (8u * byte));
            if (scale == 8)
                color &= 15u;
            if (address < DZ_VRAM_PLANE_BYTES && (scale == 1 || scale == 4 || scale == 8)) {
                uint32_t index = address * scale;
                for (unsigned lane = 0; lane < scale; ++lane)
                    dz_video.vram[index + lane] = color;
            }
            offset = (uint16_t)(offset + 1u);
        }
    }
    dz_charge((uint32_t)count * 2u + 1u);
}

/* BIOS mode 13h initializes the DAC from its chained framebuffer BEFORE the
 * unchaining/clear, then the first nine entries come from palette_buffer.
 * Original 1899:0AFC, 162 bytes */
void vga_set_mode_320x400(uint16_t clear_memory)
{
    video_mode_cookie = 0x43;
    dz_video_set_mode(DZ_MODE_VGA_400, clear_memory != 0);
    /* A000:0..767 is the chained mode-13h CPU byte stream, represented by
     * consecutive pixels here, even though the final mode is unchained. */
    load_dac(0, 256, dz_video.vram);
    if (clear_memory)
        clear_vram_words(background_fill_word, 0xfa00, 0);
    if (palette_buffer)
        load_dac(0, 9, palette_buffer);
}

/* BIOS mode 13h/93h and optional 32000-word background fill.
 * Original 1899:0B9E, 47 bytes */
void vga_set_mode_320x200(uint16_t clear_memory)
{
    video_mode_cookie = 0x44;
    dz_video_set_mode(DZ_MODE_VGA_200, clear_memory != 0);
    if (clear_memory)
        clear_vram_words(background_fill_word, 0x7d00, 0);
}

/* Native restoration returns to DZ_MODE_TEXT; tall DOS fonts and hardware
 * cursor scan shapes have no platform representation. Zero leaves mode alone.
 * Original 1899:0BCD, 70 bytes */
void restore_video_state(int16_t restore_mode)
{
    if (restore_mode) {
        dz_video_set_mode(DZ_MODE_TEXT, 1);
        dz_video.cursor_visible = (saved_cursor_shape & 0x2000u) == 0;
    }
}

static void upload_ega(uint8_t *palette, int retrace)
{
    uint8_t old[4] = { palette[0], palette[1], palette[2], palette[16] };
    palette[0] = (uint8_t)background_fill_word;
    palette[1] = (uint8_t)palette_base_green;
    palette[2] = (uint8_t)palette_base_blue;
    palette[16] = (uint8_t)ega_border_color;
    if (retrace)
        dz_wait_retrace();
    else
        dz_charge(17);
    for (int index = 15; index >= 0; --index)
        dz_video.ega_attr[index] = palette[index] & 63u;
    dz_video.ega_border = palette[16] & 63u;
    palette[16] = old[3];
    palette[2] = old[2];
    palette[1] = old[1];
    palette[0] = old[0];
}

/* Substitute reserved/border entries during the BIOS upload, then restore.
 * Original 1899:0C13, 105 bytes */
void ega_upload_palette_bios(uint8_t *palette) { upload_ega(palette, 0); }

/* Same substitutions, attribute registers 15..0 and border after retrace.
 * Original 1899:0C7C, 152 bytes */
void ega_upload_palette_ports(uint8_t *palette) { upload_ega(palette, 1); }

/* BIOS timing has no explicit retrace in the original.
 * Original 1899:0D14, 19 bytes */
void vga_upload_palette_bios(uint8_t *palette)
{
    dz_charge(759);
    load_dac(3, 253, palette);
}

/* Literal BIOS /2 anomaly: second call starts at byte 126 (RGB entry 42),
 * so DAC 129..255 repeats source entries 42..168, rather than 126..252.
 * Original 1899:0D27, 33 bytes */
void vga_upload_palette_bios_split(uint8_t *palette)
{
    dz_charge(378);
    load_dac(3, 126, palette);
    dz_charge(381);
    load_dac(129, 127, palette + 126);
}

/* Direct full upload at next retrace.
 * Original 1899:0D48, 44 bytes */
void vga_upload_palette_rep(uint8_t *palette)
{
    dz_wait_retrace();
    load_dac(3, 253, palette);
}

/* Direct split source advances by 378 bytes, unlike BIOS /2.
 * Original 1899:0D74, 75 bytes */
void vga_upload_palette_rep_split(uint8_t *palette)
{
    dz_wait_retrace();
    load_dac(3, 126, palette);
    dz_wait_retrace();
    load_dac(129, 127, palette + 378);
}

/* Byte-OUT fallback has the same observable DAC and retrace as REP.
 * Original 1899:0DBF, 47 bytes */
void vga_upload_palette_8086(uint8_t *palette) { vga_upload_palette_rep(palette); }

/* Byte-OUT two-part fallback.
 * Original 1899:0DEE, 82 bytes */
void vga_upload_palette_8086_split(uint8_t *palette) { vga_upload_palette_rep_split(palette); }

/* EGA stores only line bits 0..8; CRTC max-scan write clears bit 9.
 * Original 1899:0E40, 51 bytes */
void ega_set_line_compare(uint16_t line)
{
    dz_wait_retrace();
    dz_video.line_compare = line & 0x1ffu;
}

/* VGA stores line bits 0..9.
 * Original 1899:0E73, 76 bytes */
void vga_set_line_compare(uint16_t line)
{
    dz_wait_retrace();
    dz_video.line_compare = line & 0x3ffu;
}

/* No horizontal PEL panning: only complete 80-byte rows are represented by
 * dz_video. x/8 still contributes before truncation to display_start_row.
 * Original DIV CL faults at X>=2048; native uses the low quotient byte safely.
 * Original 1899:0EBF, 60 bytes */
void set_display_start(uint16_t x, uint16_t physical_y)
{
    uint16_t start = (uint16_t)(80u * physical_y + ((x / 8u) & 255u));
    dz_wait_retrace();
    dz_video.display_start_row = start / 80u;
}

/* Obsolete disk positioning/console messages: no native work or side effects.
 * Original 1899:0EFB, 232 bytes */
void park_disk_heads(void) { }

/* No CMOS exists in the native model: every register reads zero, including
 * register 12h (no legacy hard disks to park). Explicit argument replaces AL.
 * Original 1899:0FE3, 18 bytes */
uint8_t read_cmos_byte(uint8_t index) { (void)index; return 0; }

/* Native CPU always has the post-8086 shift behaviour; original AX contract.
 * Original 1899:0FF5, 37 bytes */
int16_t cpu_has_post_8086_shifts(void) { return 1; }

static uint16_t next_plasma_random(void)
{
    uint16_t value = (uint16_t)(plasma_random + 0x9248u);
    plasma_random = (uint16_t)((value >> 3) | (uint16_t)(value << 13));
    return plasma_random;
}

static uint16_t clamp_plasma(uint16_t word)
{
    int16_t color = signed_word(word);
    if (color < 4)
        return 3;
    return color > signed_word(plasma_color_limit) ? plasma_color_limit : word;
}

static int plasma_unassigned(uint16_t color)
{
    return (uint8_t)color == (uint8_t)background_fill_word ||
        (uint8_t)color == (uint8_t)(background_fill_word >> 8);
}

static uint16_t plot_plasma(int16_t x, int16_t y, uint16_t color)
{
    plot_color = color;
    draw_x = x;
    draw_y = y;
    plot_callback(1);
    /* The assembly reloads plot_color AFTER the callback. */
    return plot_color;
}

static uint16_t plasma_edge(int16_t x, int16_t y, uint16_t a, uint16_t b, uint16_t noise)
{
    uint16_t color = read_pixel_callback(x, y);
    if (!plasma_unassigned(color))
        return color;
    int32_t product = (int32_t)signed_word(noise) * signed_word(next_plasma_random());
    uint16_t high = (uint16_t)((uint32_t)product >> 16);
    uint16_t sum = (uint16_t)(high + a + b);
    color = clamp_plasma((uint16_t)sar_word(sum, 1));
    return plot_plasma(x, y, color);
}

/* Recursive assembly algorithm: center, top, left, right, bottom; then recurse
 * TL, TR, BR, BL. Noise uses signed IMUL's high word and arithmetic halving.
 * Original 1899:101A, 803 bytes */
void subdivide_plasma(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                     uint16_t top_left, uint16_t top_right,
                     uint16_t bottom_right, uint16_t bottom_left)
{
    if (palette_elapsed_ticks >= palette_cycle_ticks)
        cycle_palette();
    if (input_pending && poll_graphics_keys())
        return;
    if ((uint8_t)current_graphics_key == 0x6e)
        return;
    if (signed_word((uint16_t)(x1 - x0)) < 2 && signed_word((uint16_t)(y1 - y0)) < 2)
        return;
    /* SHR then ADC rounds the already wrapped UNSIGNED sum upward. */
    uint16_t sum_x = (uint16_t)(x1 + x0), sum_y = (uint16_t)(y1 + y0);
    int16_t mid_x = signed_word((uint16_t)((sum_x >> 1) + (sum_x & 1u)));
    int16_t mid_y = signed_word((uint16_t)((sum_y >> 1) + (sum_y & 1u)));
    uint16_t center = read_pixel_callback(mid_x, mid_y);
    if (plasma_unassigned(center)) {
        center = (uint16_t)(top_left + top_right + bottom_right + bottom_left);
        center = plot_plasma(mid_x, mid_y, (uint16_t)(center >> 2));
    }
    uint16_t noise = (uint16_t)((uint16_t)(x1 - x0 + y1 - y0) * 2u);
    uint16_t top = plasma_edge(mid_x, y0, top_left, top_right, noise);
    uint16_t left = plasma_edge(x0, mid_y, top_left, bottom_left, noise);
    uint16_t right = plasma_edge(x1, mid_y, top_right, bottom_right, noise);
    uint16_t bottom = plasma_edge(mid_x, y1, bottom_right, bottom_left, noise);
    subdivide_plasma(x0, y0, mid_x, mid_y, top_left, top, center, left);
    subdivide_plasma(mid_x, y0, x1, mid_y, top, top_right, right, center);
    subdivide_plasma(mid_x, mid_y, x1, y1, center, right, bottom_right, bottom);
    subdivide_plasma(x0, mid_y, mid_x, y1, left, center, bottom, bottom_left);
}

/* Seed TL, TR, BR, BL using the local rotate/add RNG (no Borland rand calls).
 * DOS AH=2Ch's seconds/hundredths seed is replaced by virtual BIOS time, so
 * deterministic runs never read the wall clock. Quantization is one BIOS tick.
 * Original 1899:133D, 394 bytes */
void seed_plasma(void)
{
    plasma_width = (uint16_t)(screen_half_width - 1);
    plasma_height = (uint16_t)(screen_half_height - 1);
    plasma_color_limit = (uint16_t)(color_count - 1u);
    /* 18.2065 Hz = 36413/2000: integer conversion also preserves exact
     * hundredth boundaries without floating-point rounding across hosts. */
    uint64_t hundredths = (uint64_t)dz_bios_time() * 200000u / 36413u;
    plasma_random = (uint16_t)(((hundredths / 100u) % 60u) << 8 | (hundredths % 100u));
    uint16_t top_left = clamp_plasma(next_plasma_random() >> 8);
    plot_plasma(0, 0, top_left);
    uint16_t top_right = clamp_plasma(next_plasma_random() >> 8);
    plot_plasma(signed_word(plasma_width), 0, top_right);
    uint16_t bottom_right = clamp_plasma(next_plasma_random() >> 8);
    plot_plasma(signed_word(plasma_width), signed_word(plasma_height), bottom_right);
    uint16_t bottom_left = clamp_plasma(next_plasma_random() >> 8);
    plot_plasma(0, signed_word(plasma_height), bottom_left);
    subdivide_plasma(0, 0, signed_word(plasma_width), signed_word(plasma_height),
                     top_left, top_right, bottom_right, bottom_left);
}

/* Plain handler body; the platform owns interrupt scheduling, with no chaining.
 * Every fifth tick also notices BIOS keys and the saver header's exit bit.
 * Original 1899:14C7, 133 bytes */
void graphics_timer_tick_isr(void)
{
    palette_elapsed_ticks = (uint16_t)(palette_elapsed_ticks + 1u);
    if (countdown_ticks)
        --countdown_ticks;
    if (show_duration_ticks)
        --show_duration_ticks;
    input_poll_divider = (uint16_t)(input_poll_divider - 1u);
    if (input_poll_divider)
        return;
    input_poll_divider = 5;
    if (dz_bioskey(1) && !input_pending)
        ++input_pending;
    if (mouse_callback_installed && current_image_header &&
        current_image_header[9] == 0x777 && (current_image_header[6] & 0x1000u)) {
        exit_requested = (uint16_t)(exit_requested + 1u);
        input_pending = (uint16_t)(input_pending + 1u);
        current_image_header[6] &= 0xefffu;
    }
}

/* Plain keyboard IRQ effect, without saved-vector chaining.
 * Original 1899:154C, 19 bytes */
void keyboard_pending_isr(void) { input_pending = (uint16_t)(input_pending + 1u); }

/* Event registers were ignored; preserve both 16-bit counter increments.
 * Original 1899:155F, 18 bytes */
void mouse_pending_callback(void)
{
    input_pending = (uint16_t)(input_pending + 1u);
    mouse_abort_pending = (uint16_t)(mouse_abort_pending + 1u);
}

/* Segment identity has no native meaning; retain an inert compatibility entry.
 * Original 1899:1571, 3 bytes */
uint16_t dvdt_seg(void) { return 0; }
