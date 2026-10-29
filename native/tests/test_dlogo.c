/* Closing-logo decode; compare dlogo_frame.raw with tools/logo.py at test time. */
#include "dazzle.h"

#include <assert.h>
#include <stdio.h>

#define ORIGINAL_EXE "D:\\Prog\\dazzle_recon\\assets\\DAZZLE.EXE"
#define FRAME_BYTES (640u * 350u)

int main(void)
{
    FILE *probe = fopen(ORIGINAL_EXE, "rb");
    if (probe == NULL) {
        puts("logo decode skipped: original executable is absent");
        return 0;
    }
    fclose(probe);

    dz_options.original_exe = ORIGINAL_EXE;
    exit_requested = 0;
    video_adapter = 4;
    bios_resolution_mode = 0;
    configure_video_backend(0);
    prepare_logo_image_resources();
    assert(!exit_requested);
    assert(logo_image_height == 350);
    draw_closing_graphic();
    assert(!exit_requested);
    assert(dz_video.mode == DZ_MODE_EGA_350);
    assert(dz_video.width == 640 && dz_video.height == 350);

    FILE *frame = fopen("dlogo_frame.raw", "wb");
    assert(frame != NULL);
    assert(fwrite(dz_video.vram, 1, FRAME_BYTES, frame) == FRAME_BYTES);
    assert(fclose(frame) == 0);
    printf("decoded logo frame: %u x %u indexed pixels\n", 640u, 350u);
    return 0;
}
