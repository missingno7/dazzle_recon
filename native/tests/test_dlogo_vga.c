/* VGA logo path from a generated PCX-style tail resource. */
#include "dazzle.h"

#include <assert.h>
#include <stdio.h>

extern void restore_logo_video_state(void);

static void put_u16(uint8_t *bytes, uint16_t value)
{
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8);
}

static void put_u32(uint8_t *bytes, uint32_t value)
{
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8);
    bytes[2] = (uint8_t)(value >> 16);
    bytes[3] = (uint8_t)(value >> 24);
}

static void write_fixture(void)
{
    uint8_t resource[128 + 200 * 12 + 769] = {0};
    resource[0] = 10; resource[1] = 5; resource[2] = 1; resource[3] = 8;
    put_u16(resource + 8, 319); put_u16(resource + 10, 199);
    put_u16(resource + 12, 320); put_u16(resource + 14, 200);
    resource[65] = 1;
    put_u16(resource + 66, 320);
    put_u32(resource + 0x46, sizeof resource);
    put_u16(resource + 0x4a, 1);

    size_t cursor = 128;
    for (uint16_t row = 0; row < 200; ++row) {
        uint8_t value = (uint8_t)row;
        for (unsigned run = 0; run < 5; ++run) {
            resource[cursor++] = 0xff;
            resource[cursor++] = value;
        }
        resource[cursor++] = 0xc5;
        resource[cursor++] = value;
    }
    assert(cursor == 128 + 200 * 12);
    resource[cursor++] = 0x0c;
    for (unsigned i = 0; i < 768; ++i)
        resource[cursor++] = (uint8_t)(i & 255u);
    assert(cursor == sizeof resource);

    FILE *file = fopen("dlogo_vga_fixture.exe", "wb");
    assert(file != NULL);
    uint8_t mz[28] = {0};
    mz[0] = 'M'; mz[1] = 'Z';
    put_u16(mz + 2, sizeof mz);
    put_u16(mz + 4, 1);
    assert(fwrite(mz, sizeof mz, 1, file) == 1);
    assert(fputc(0x84, file) == 0x84);
    uint8_t metadata[10] = {0};
    put_u16(metadata + 8, 4); /* empty manual length plus no compressed bytes */
    assert(fwrite(metadata, sizeof metadata, 1, file) == 1);
    uint32_t empty_manual = 0;
    assert(fwrite(&empty_manual, sizeof empty_manual, 1, file) == 1);
    assert(fwrite(resource, sizeof resource, 1, file) == 1);
    assert(fclose(file) == 0);
}

int main(void)
{
    write_fixture();
    dz_options.original_exe = "dlogo_vga_fixture.exe";
    exit_requested = 0;
    video_adapter = 8;
    bios_resolution_mode = 0;
    dz_video_set_mode(DZ_MODE_VGA_400, 1);
    configure_video_backend(0);
    prepare_logo_image_resources();
    assert(!exit_requested && image_frame_count == 1 && logo_image_height == 200);
    draw_closing_graphic();
    assert(!exit_requested && dz_video.mode == DZ_MODE_VGA_200);
    for (uint16_t y = 0; y < 200; ++y)
        for (uint16_t x = 0; x < 320; ++x)
            assert(dz_video.vram[(size_t)y * 320u + x] == (uint8_t)y);
    restore_logo_video_state();
    assert(dz_video.mode == DZ_MODE_VGA_400 && bios_resolution_mode == 0);
    assert(remove("dlogo_vga_fixture.exe") == 0);
    puts("VGA logo decode and mode restoration passed");
    return 0;
}
