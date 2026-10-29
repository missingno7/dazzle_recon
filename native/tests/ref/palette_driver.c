/* Palette diagnostics from the reconstructed DOSI translation unit. */
#include "compat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dazzle.h"
#include "dzconst.h"

#define MAX_BUFFER 2499
#define FIRST_BYTES 24

static unsigned char far *buffer;
static FILE *output;
#ifndef REF_NATIVE
static unsigned long rng_state = 1UL;
static unsigned long rng_calls;
static unsigned int fake_bios_flags;
#endif

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

#else
#define ref_srand dz_srand
#define rng_calls ((unsigned long)dz_rand_calls())
#endif
static unsigned long crc32_bytes(unsigned char far *bytes, unsigned count)
{
    unsigned long crc = 0xffffffffUL;
    unsigned i;
    unsigned bit;
    for (i = 0; i < count; ++i) {
        crc ^= bytes[i];
        for (bit = 0; bit < 8; ++bit)
            crc = (crc & 1UL) ? ((crc >> 1) ^ 0xedb88320UL) : (crc >> 1);
    }
    return crc ^ 0xffffffffUL;
}

static void far palette_sink(unsigned char far *bytes)
{
    (void)bytes;
}

static int run_case(FILE *scenarios)
{
    char name[24];
    char line[128];
    unsigned adapter, seed, alternate, mad, fixed, four, full;
    unsigned length, i;
    unsigned long crc;

    do {
        if (!fgets(line, sizeof(line), scenarios)) return 1;
    } while (line[0] == '#' || line[0] == '\n' || line[0] == '\r');
    if (sscanf(line, "%23s %u %x %u %u %u %u %u", name, &adapter, &seed,
               &alternate, &mad, &fixed, &four, &full) != 8)
        return 1;

    video_adapter = adapter;
    color_count = (adapter == ADAPTER_VGA) ? 256 : 16;
    palette_entries = (adapter == ADAPTER_VGA) ? 576 : 64;
    color_index_mask = color_count - 1;
    palette_cycle_wrap_bytes = palette_entries * 3;
    palette_cycle_ticks = default_palette_cycle_ticks = (adapter == ADAPTER_VGA) ? 1 : 2;
    palette_step = 3;
    palette_cycle_offset = (adapter == ADAPTER_VGA) ? 9 : 3;
    palette_alternate_mode = alternate;
    palette_random_mode = mad;
    palette_fixed_mode = fixed;
    palette_four_ega_colors = four;
    palette_base_green = 10;
    palette_base_blue = 12;
    background_fill_word = 0;
    plot_color = FIRST_CYCLING_COLOR;
    palette_elapsed_ticks = 0;
    display_freeze_active = display_freeze_cycle_requested = 0;
    input_pending = exit_requested = 0;
    window_mode = 1;
    ega_red_limit = 0x2f;
    ega_blue_limit = 0x3f;
    ega_green_limit = 0x1f;
#ifndef REF_NATIVE
    bios_keyboard_flags = &fake_bios_flags;
    fake_bios_flags = 0;
#endif
    palette_buffer = buffer;
    palette_upload_callback = palette_sink;
    palette_buffer_bytes = (adapter == ADAPTER_VGA) ?
        (color_count + palette_entries) * 3 : color_count + palette_entries;
    length = palette_buffer_bytes + 3;
    for (i = 0; i < length; ++i) buffer[i] = 0;
    ref_srand(seed);

    if (adapter == ADAPTER_VGA)
        build_ega_palette(0);
    else
        generate_palette();

    crc = crc32_bytes(buffer, length);
    fprintf(output, "%s,%u,%04X,%u,%u,%u,%u,%lu,%u,%08lX,",
            name, adapter, seed, alternate, mad, fixed, four, rng_calls, length, crc);
    for (i = 0; i < FIRST_BYTES && i < length; ++i)
        fprintf(output, "%02X", (unsigned)buffer[i]);
    if (full) {
        fputc(',', output);
        for (i = 0; i < length; ++i)
            fprintf(output, "%02X", (unsigned)buffer[i]);
    }
    fputc('\n', output);
    fflush(output);
    return 0;
}

#ifndef REF_NATIVE
void main(int argc, char far * far *argv)
{
    FILE *scenarios;

    (void)argc;
    (void)argv;
    output = fopen("PALETTE.TXT", "w");
    scenarios = fopen("PALETTES.TSV", "r");
    if (!output || !scenarios) exit(2);
    fprintf(output, "# name,adapter,seed,alternate,mad,fixed,four_ega,rand_calls,buffer_bytes,crc32,first24_hex[,full_buffer_hex]\n");
    fflush(output);
    buffer = (unsigned char far *)farmalloc(MAX_BUFFER);
    if (!buffer) exit(3);
    while (!feof(scenarios)) {
        int result = run_case(scenarios);
        if (result == 1) break;
        if (result) exit(4);
    }
    fclose(scenarios);
    fclose(output);
    printf("wrote PALETTE.TXT\n");
    exit(0);
}

#endif
