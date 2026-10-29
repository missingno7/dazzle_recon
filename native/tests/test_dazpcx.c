/* Deterministic PCX header, RLE and framebuffer round-trip checks. */
#include "dazzle.h"
#include "dzconst.h"
#include "platform/platform.h"

#include <assert.h>
#include <stdio.h>

static uint16_t read_u16(const uint8_t *data)
{
    return (uint16_t)(data[0] | ((uint16_t)data[1] << 8));
}

static void verify_file(const char *path, uint16_t width, uint16_t height,
                        uint8_t planes, uint16_t bytes_per_line, const uint8_t *pixels)
{
    FILE *file = fopen(path, "rb");
    assert(file != NULL);
    assert(fseek(file, 0, SEEK_END) == 0);
    long file_size = ftell(file);
    assert(file_size >= 128);
    rewind(file);
    uint8_t *data = (uint8_t *)malloc((size_t)file_size);
    assert(data != NULL);
    assert(fread(data, (size_t)file_size, 1, file) == 1);
    fclose(file);

    assert(data[0] == 10 && data[1] == 5 && data[2] == 1);
    assert(read_u16(data + 4) == 0 && read_u16(data + 6) == 0);
    assert(read_u16(data + 8) == width - 1u);
    assert(read_u16(data + 10) == height - 1u);
    assert(data[65] == planes && read_u16(data + 66) == bytes_per_line);
    assert(read_u16(data + 12) == width && read_u16(data + 14) == height);

    size_t decoded_size = (size_t)height * bytes_per_line * planes;
    uint8_t *decoded = (uint8_t *)malloc(decoded_size);
    assert(decoded != NULL);
    size_t source = 128, output = 0;
    while (output < decoded_size) {
        assert(source < (size_t)file_size);
        uint8_t code = data[source++];
        size_t count = 1;
        uint8_t value = code;
        if ((code & 0xc0u) == 0xc0u) {
            count = code & 0x3fu;
            assert(count != 0 && source < (size_t)file_size);
            value = data[source++];
        }
        assert(output + count <= decoded_size);
        memset(decoded + output, value, count);
        output += count;
    }

    if (planes == 1) {
        assert(source + 769u == (size_t)file_size);
        assert(data[source++] == PCX_VGA_PALETTE_MARKER);
        assert(source + 768u == (size_t)file_size);
    } else {
        assert(source == (size_t)file_size);
        for (uint16_t y = 0; y < height; ++y) {
            for (uint16_t x = 0; x < width; ++x) {
                uint8_t color = 0;
                uint16_t byte_index = (uint16_t)(x >> 3);
                uint8_t bit = (uint8_t)(0x80u >> (x & 7u));
                for (uint16_t plane = 0; plane < 4; ++plane) {
                    size_t index = (size_t)y * bytes_per_line * 4u +
                        (size_t)plane * bytes_per_line + byte_index;
                    if (decoded[index] & bit)
                        color |= (uint8_t)(1u << plane);
                }
                assert(color == pixels[(size_t)y * width + x]);
            }
        }
    }
    if (planes == 1)
        assert(memcmp(decoded, pixels, decoded_size) == 0);
    free(decoded);
    free(data);
}

static void fill_vga(void)
{
    for (unsigned y = 0; y < 400; ++y)
        for (unsigned x = 0; x < 320; ++x)
            dz_video.vram[y * 320u + x] = (uint8_t)(((x / 5u) + (y / 3u) * 7u) & 255u);
}

static void fill_vga_page(uint32_t base)
{
    for (unsigned y = 0; y < 400; ++y)
        for (unsigned x = 0; x < 320; ++x)
            dz_video.vram[base + y * 320u + x] = (uint8_t)(((x / 7u) + (y / 5u) * 11u) & 255u);
}

static void fill_vga_200(void)
{
    for (unsigned y = 0; y < 200; ++y)
        for (unsigned x = 0; x < 320; ++x)
            dz_video.vram[y * 320u + x] = (uint8_t)(((x / 3u) + (y / 2u) * 17u) & 255u);
}

static void fill_ega(void)
{
    for (unsigned y = 0; y < 350; ++y)
        for (unsigned x = 0; x < 640; ++x)
            dz_video.vram[y * 640u + x] = (uint8_t)(((x / 8u) + (y / 4u)) & 15u);
}

int main(void)
{
    dz_options = (struct dz_options){.deterministic = 1, .seed = 1, .headless = 1, .speed = 1.0};
    assert(dz_initialize());
    palette_buffer = (uint8_t *)calloc(2500, 1);
    assert(palette_buffer != NULL);
    palette_buffer_bytes = 2500;
    palette_cycle_offset = 9;
    for (size_t i = 0; i < 2500; ++i)
        palette_buffer[i] = (uint8_t)((i * 13u) & 63u);
    page_paragraph = 0;
    display_pan_state = 0;

    remove("daz_0000.pcx");
    video_adapter = 8;
    bios_resolution_mode = 0;
    configure_video_backend(0);
    dz_video_set_mode(DZ_MODE_VGA_400, 1);
    fill_vga();
    save_pcx();
    verify_file("daz_0000.pcx", 320, 400, 1, 320, dz_video.vram);
    remove("daz_0000.pcx");

    page_paragraph = 2000;
    display_pan_state = 2;
    fill_vga_page(128000u);
    save_pcx();
    verify_file("daz_0001.pcx", 320, 400, 1, 320, dz_video.vram + 128000u);
    remove("daz_0001.pcx");

    page_paragraph = 0;
    display_pan_state = 0;
    bios_resolution_mode = 1;
    configure_video_backend(0);
    dz_video_set_mode(DZ_MODE_VGA_200, 1);
    fill_vga_200();
    save_pcx();
    verify_file("daz_0002.pcx", 320, 200, 1, 320, dz_video.vram);
    remove("daz_0002.pcx");

    video_adapter = 4;
    bios_resolution_mode = 1;
    configure_video_backend(0);
    dz_video_set_mode(DZ_MODE_EGA_350, 1);
    fill_ega();
    save_pcx();
    verify_file("daz_0003.pcx", 640, 350, 4, 80, dz_video.vram);
    remove("daz_0003.pcx");

    free(palette_buffer);
    palette_buffer = NULL;
    dz_shutdown();
    puts("PCX VGA/EGA headers and RLE round trips passed");
    return 0;
}
