/* Bounded image diagnostics using the reconstructed DZLE/DZL2 C modules.
 * The callbacks are a logged software model of the VGA 320x400 kernel contract. */
#include "compat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "dazzle.h"
#include "dzconst.h"

#define IMAGE_COUNT 42
#define FIRST_LOG_COUNT 32
#define FRAME_BYTES 128000UL
#define PALETTE_BYTES 2499UL

#ifndef REF_NATIVE
/* State normally supplied by DOSI and the original video assembly module. */
unsigned video_adapter;
unsigned plot_color;
unsigned int page_paragraph;
unsigned int ega_red_limit, ega_blue_limit, ega_green_limit;

void park_disk_heads(void) { }
void beep(unsigned hz, unsigned ms) { (void)hz; (void)ms; }
void signal_diagnostic(int signal_number, int signal_type, unsigned int far *registers)
{ (void)signal_number; (void)signal_type; (void)registers; }
void restore_runtime_callbacks(void) { }
void install_runtime_callbacks(void) { }
int resolve_startup_configuration(int requested) { (void)requested; return 0; }
void configure_video_backend(int allocate) { (void)allocate; }
void check_executable_integrity(void) { }
void fatal_selfcheck_error(int show_crc, const char far *reason)
{ (void)show_crc; (void)reason; }
unsigned int keyboard_led_flags(unsigned int mask, unsigned int mode)
{ (void)mask; (void)mode; return 0; }
void print_startup_information(void) { }
void detect_video_adapter(void) { }
int parse_command_line(int argc, char far * far *argv, int far *position)
{ (void)argc; (void)argv; (void)position; return 0; }
void save_video_state(void) { }
void initialize_program_defaults(void) { }
void restore_text_attributes(void) { }
void print_closing_text_and_exit(int status) { (void)status; }
void restore_video_state(int mode) { (void)mode; }
void closing_graphic_and_music(void) { }
void auto_loop(void) { }
void semi_auto_loop(void) { }
void prepare_logo_image_resources(void) { }
void far rgb8_to_ega_palette(unsigned char far *destination,
                             unsigned char far *source, unsigned mask)
{ (void)destination; (void)source; (void)mask; }

void far initialize_palette_mapping(PaletteMapSpec far *mapping)
{
    int map_half_height, map_half_width;
    int map_limit_y, map_limit_x;
    unsigned int y_squared, pixel_row_offset, map_radius_squared;
    float color_distance;
    int y, x, dy, dx;
    unsigned char color_index;

    map_half_height = mapping->height / 2;
    map_half_width = mapping->width / 2;
    map_limit_y = map_half_height + map_half_height / 4;
    map_limit_x = map_half_width + map_half_width / 4;
    map_radius_squared = map_half_height < map_half_width ? map_half_height - 1 : map_half_width - 1;
    map_radius_squared *= map_radius_squared;
    for (y = 0; y < mapping->height; ++y) {
        y_squared = abs(map_half_height - y);
        y_squared *= y_squared;
        pixel_row_offset = mapping->width * y;
        for (x = 0; x < mapping->width; ++x) {
            dx = abs(map_half_width - x);
            dx *= dx;
            if (y_squared + dx > map_radius_squared) {
                mapping->pixels[pixel_row_offset + x] = 0;
            } else {
                dx = abs(map_limit_y - y);
                dy = abs(map_limit_x - x);
                if (dx || dy)
                    color_distance = sqrt((double)(unsigned int)(dx * dx + dy * dy));
                else
                    color_distance = 0.0;
                color_index = (int)color_distance % 253;
                if (color_count < 0x80) color_index >>= 1;
                mapping->pixels[pixel_row_offset + x] = (unsigned char)(color_index + FIRST_CYCLING_COLOR);
            }
        }
    }
}

#endif
/* Huge is required: ordinary Borland far-pointer indexing aliases every 64 KiB. */
static unsigned char huge *framebuffer;
static FILE *full_log;
static FILE *first_log;
static FILE *summary;
#ifndef REF_NATIVE
static unsigned long rng_state = 1UL;
static unsigned long rng_calls;
#endif
static unsigned long plot_calls;
static unsigned long cycle_calls;
static unsigned long plot_crc;
static unsigned long crc_table[256];
static unsigned current_image;
static unsigned model_width = 320, model_height = 400;
static unsigned long frame_bytes = FRAME_BYTES;
static unsigned current_step;
static unsigned current_phase;
static unsigned tick_period;
static unsigned first_log_calls;

static void write_word(FILE *stream, unsigned value)
{
    fputc((int)(value & 0xff), stream);
    fputc((int)((value >> 8) & 0xff), stream);
}

static void crc_byte(unsigned long *crc, unsigned char value)
{
    *crc = (*crc >> 8) ^ crc_table[(*crc ^ value) & 0xffUL];
}

static void make_crc_table(void)
{
    unsigned i, bit;
    unsigned long value;
    for (i = 0; i < 256; ++i) {
        value = i;
        for (bit = 0; bit < 8; ++bit)
            value = (value & 1UL) ? ((value >> 1) ^ 0xedb88320UL) : (value >> 1);
        crc_table[i] = value;
    }
}

static unsigned long crc_frame(void)
{
    unsigned long crc = 0xffffffffUL;
    unsigned long i;
    for (i = 0; i < frame_bytes; ++i)
        crc_byte(&crc, framebuffer[i]);
    return crc ^ 0xffffffffUL;
}

#ifndef REF_NATIVE
int ref_rand(void)
{
    rng_state = rng_state * 22695477UL + 1UL;
    ++rng_calls;
    return (int)((rng_state >> 16) & 0x7fffUL);
}

void ref_srand(unsigned seed)
{
    rng_state = seed;
    rng_calls = 0;
}

void initialize_math_tables(void)
{
    float angle;
    int i;
    angle = 0.0;
    for (i = 0; i < 64; ++i) {
        trig_table[i].cosine = cos(angle);
        trig_table[i].sine = sin(angle);
        angle += 0.098174770421875;
    }
}

#else
#define ref_srand dz_srand
#define ref_rand dz_rand
#define rng_calls ((unsigned long)dz_rand_calls())
#endif

static void timer_tick(void)
{
    ++palette_elapsed_ticks;
    if (countdown_ticks) --countdown_ticks;
    if (show_duration_ticks) --show_duration_ticks;
}

void cycle_palette(void)
{
    ++cycle_calls;
    palette_cycle_offset += palette_step;
    if (palette_cycle_offset >= palette_cycle_wrap_bytes)
        palette_cycle_offset = 9;
    palette_elapsed_ticks = 0;
    if (palette_upload_callback)
        palette_upload_callback(palette_buffer + palette_cycle_offset);
}

RefInt poll_graphics_keys(void)
{
    return 0;
}

void set_ega_raster_xor(RefInt mode)
{
    (void)mode;
}

void far ega_plot_symmetric_640x700(RefWord symmetry);

static void write_pixel(RefInt x, RefInt y)
{
    if (x >= 0 && (unsigned)x < model_width && y >= 0 && (unsigned)y < model_height) {
        if (video_adapter == ADAPTER_EGA) {
            if (y & 1) return;
            y /= 2;
        }
        framebuffer[(unsigned long)y * model_width + (unsigned)x] =
            (unsigned char)(plot_color & color_index_mask);
    }
}

static void plot_pass(RefInt x, RefInt y, RefWord symmetry)
{
    RefInt xp = screen_half_width + x;
    RefInt xm = screen_half_width - x;
    RefInt yp = screen_half_height + y;
    RefInt ym = screen_half_height - y;
    int fx, fy;

    if (xp < 0 || xm < 0 || yp < 0 || ym < 0) return;
    fx = screen_half_width <= x;
    fy = screen_half_height <= y;
    if (!fx && !fy) write_pixel(xp, yp);
    if (symmetry & 1) {
        if (!fx) write_pixel(xp, ym);
        if (!fy) write_pixel(xm, yp);
        write_pixel(xm, ym);
    }
    if ((symmetry & 3) == 3) plot_pass(y, x, 1);
}

void far image_plot(RefWord symmetry)
{
    unsigned x = (unsigned)draw_x;
    unsigned y = (unsigned)draw_y;
    unsigned color = plot_color;
    unsigned long crc = plot_crc;

    ++plot_calls;
    write_word(full_log, current_image);
    write_word(full_log, x);
    write_word(full_log, y);
    write_word(full_log, color);
    write_word(full_log, symmetry);
    write_word(full_log, (unsigned)(rng_calls & 0xffffUL));
    write_word(full_log, (unsigned)(rng_calls >> 16));
    crc_byte(&crc, (unsigned char)(x & 0xff));
    crc_byte(&crc, (unsigned char)(x >> 8));
    crc_byte(&crc, (unsigned char)(y & 0xff));
    crc_byte(&crc, (unsigned char)(y >> 8));
    crc_byte(&crc, (unsigned char)(color & 0xff));
    crc_byte(&crc, (unsigned char)(color >> 8));
    crc_byte(&crc, (unsigned char)(symmetry & 0xff));
    crc_byte(&crc, (unsigned char)(symmetry >> 8));
    plot_crc = crc;

    if (first_log_calls < FIRST_LOG_COUNT) {
        fprintf(first_log, "%u,%u,%u,%u,%d,%d,%u,%u\n", current_image,
                current_phase, current_step, (unsigned)plot_calls, draw_x, draw_y,
                color, symmetry);
        ++first_log_calls;
    }

    plot_pass(draw_x, draw_y, symmetry);
    if (tick_period && plot_calls % tick_period == 0) timer_tick();
}

static RefWord far image_read(RefInt x, RefInt y)
{
    x += screen_half_width;
    y += screen_half_height;
    if (x < 0 || (unsigned)x >= model_width || y < 0 || (unsigned)y >= model_height) return 0;
    if (video_adapter == ADAPTER_EGA) y /= 2;
    return framebuffer[(unsigned long)y * model_width + (unsigned)x];
}

static void far palette_sink(unsigned char far *bytes)
{
    (void)bytes;
}

static void far display_start_sink(RefWord start, RefWord split)
{
    (void)start;
    (void)split;
}

static void reset_frame(void)
{
    unsigned long i;
    for (i = 0; i < frame_bytes; ++i) framebuffer[i] = 0;
}

/* Match the enhanced EGA function-pointer check while still logging callbacks. */
void far ega_plot_symmetric_640x700(RefWord symmetry) { image_plot(symmetry); }

static void setup_video_model(unsigned adapter)
{
    unsigned long i;
    unsigned ball_scale_divisor;
    video_adapter = adapter;
    model_width = adapter == ADAPTER_EGA ? 640 : 320;
    model_height = adapter == ADAPTER_EGA ? 700 : 400;
    frame_bytes = adapter == ADAPTER_EGA ? 224000UL : FRAME_BYTES;
    screen_half_width = model_width / 2;
    screen_half_height = model_height / 2;
    screen_extent = screen_half_height + 1;
#ifdef REF_NATIVE
    video_backend = adapter == ADAPTER_EGA ? BACKEND_EGA_640X700 : BACKEND_VGA_320X400;
#endif
    page_paragraph = 0;
    color_count = adapter == ADAPTER_EGA ? 16 : 256;
    color_index_mask = color_count - 1;
    palette_entries = adapter == ADAPTER_EGA ? 64 : 576;
    palette_cycle_wrap_bytes = palette_entries * 3;
    palette_cycle_ticks = default_palette_cycle_ticks = 1;
    palette_step = 3;
    palette_cycle_offset = 9;
    palette_buffer_bytes = (color_count + palette_entries) * 3;
    maximum_image_size = 0xffff / (screen_extent + (screen_extent >> 2)) - 1;
    ega_red_limit = 0x2f;
    ega_blue_limit = 0x3f;
    ega_green_limit = 0x1f;
    if (!palette_buffer) palette_buffer = (unsigned char far *)farmalloc(PALETTE_BYTES);
    if (!palette_buffer) exit(3);
    for (i = 0; i < PALETTE_BYTES; ++i) palette_buffer[i] = 0;
    plot_callback = adapter == ADAPTER_EGA ? ega_plot_symmetric_640x700 : image_plot;
    read_pixel_callback = image_read;
    palette_upload_callback = palette_sink;
    display_start_callback = display_start_sink;
    draw_delay_count = 0;
    if (!linewipe_history) linewipe_history = farmalloc(0x7d8UL);
    if (!linewipe_history) exit(3);
    ref_srand(1);
    ball_scale_divisor = (ref_rand() * 5) / 32768 + 2;
    ball_bitmap_height = screen_half_height / ball_scale_divisor;
    ball_bitmap_width = screen_half_width / ball_scale_divisor;
    if (!ball_bitmap_pixels) ball_bitmap_pixels = (unsigned char far *)farmalloc(65536UL);
    if (!ball_bitmap_pixels) exit(3);
    /* balls fetches before testing coordinates. Give the complete wrapped DOS
     * offset domain a defined zero padding value, instead of sampling heap data. */
    for (i = 0; i < 65536UL; ++i) ball_bitmap_pixels[i] = 0;
    {
        PaletteMapSpec mapping;
        /* Original DOS globals are height,width,pointer; the cast intentionally
         * interprets them as width,height,pointer. Preserve that transpose. */
        mapping.width = ball_bitmap_height;
        mapping.height = ball_bitmap_width;
        mapping.pixels = ball_bitmap_pixels;
        initialize_palette_mapping(&mapping);
    }
}

static void continue_image(ImageDrawRoutine draw)
{
#ifndef REF_NATIVE
    /* Control the two known uninitialized DZL2 gate slots. BP-2 / BP-18 map
     * to SP-10 / SP-26 before the argument and far return address are pushed.
     * A positive gate retains the steps. gate_driver.c probes both signs. */
    if (draw == diamonds) {
        asm mov bx, sp
        asm mov word ptr ss:[bx-10], 1
        diamonds(0);
        return;
    }
    if (draw == wedges) {
        asm mov bx, sp
        asm mov word ptr ss:[bx-26], 1
        wedges(0);
        return;
    }
#endif
    draw(0);
}

static int run_one(FILE *scenarios, unsigned index)
{
    char name[24];
    unsigned seed, steps, tick, symmetry, adapter = ADAPTER_VGA, image;
    ImageConfigRow far *row = 0;
    unsigned long half_crc = 0, end_crc;
    unsigned i;
    unsigned long init_crc;
    char line[128];

    do {
        if (!fgets(line, sizeof(line), scenarios)) return 1;
    } while (line[0] == '#' || line[0] == '\n' || line[0] == '\r');
    if (sscanf(line, "%23s %x %u %u %u %u", name, &seed, &steps, &tick, &symmetry, &adapter) < 5)
        return 1;
    for (image = 0; image < IMAGE_COUNT; ++image) {
        ImageConfigRow far *candidate = image < 36 ? &alt_image_records[image] : &ctrl_image_records[image - 36];
        if (!strcmp(name, candidate->name)) { row = candidate; break; }
    }
    if (!row) { fprintf(stderr, "unknown image %s\n", name); return 2; }
    setup_video_model(adapter);
    current_image = index;
    current_image_index = image;
    current_image_draw = row->draw;
    tick_period = tick;
    current_step = 0;
    current_phase = 0;
    first_log_calls = 0;
    plot_calls = cycle_calls = 0;
    plot_crc = 0xffffffffUL;
    palette_elapsed_ticks = countdown_ticks = show_duration_ticks = 0;
    palette_cycle_offset = 9;
    plot_color = FIRST_CYCLING_COLOR;
    draw_x = draw_y = 0;
    ega_color_substep = 0;
    image_work_count = 0;
    effect_parameter = 0;
    draw_symmetry = symmetry;
    ref_srand(seed);
    reset_frame();

    fprintf(first_log, "# %u %s seed=%04X steps=%u tick=%u symmetry=%u\n",
            index, name, seed, steps, tick, symmetry);
    fprintf(summary, "# BEGIN %u %s\n", index, name);
    fflush(summary);
    row->draw(1);
    init_crc = crc_frame();
    current_phase = 1;
    for (i = 1; i <= steps; ++i) {
        current_step = i;
        continue_image(row->draw);
        plot_callback(draw_symmetry);
        if (i == steps / 2) half_crc = crc_frame();
    }
    end_crc = crc_frame();
    fprintf(summary, "%s,%04X,%u,%u,%u,%u,%lu,%08lX,%lu,%lu,%08lX,%08lX,%08lX,%u\n",
            name, seed, steps, tick, symmetry, adapter, plot_calls,
            plot_crc ^ 0xffffffffUL, rng_calls, cycle_calls,
            init_crc, half_crc, end_crc, palette_elapsed_ticks);
    fflush(summary);
    fflush(first_log);
    return 0;
}

