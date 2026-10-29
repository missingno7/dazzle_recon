/* The canonical capture must describe scanout, including CRTC page/split state. */
#include "dazzle.h"
#include "platform/platform.h"
#include "platform/testctl.h"
#include <assert.h>

static void check(const char *path, size_t size, unsigned first, unsigned second, unsigned split)
{
    FILE *file = fopen(path, "rb"); assert(file);
    char line[2304];
    int end = 0, palette = 0;
    while (fgets(line, sizeof line, file)) {
        if (!strncmp(line, "palette=", 8)) palette = 1;
        if (!strcmp(line, "END\n")) { end = 1; break; }
    }
    assert(end && palette);
    for (size_t i = 0; i < size; ++i) {
        int byte = fgetc(file); assert(byte != EOF);
        unsigned row = (unsigned)(i / (size_t)dz_video.width);
        if (row == 0) assert((unsigned)byte == first);
        if (row == 1) assert((unsigned)byte == second);
        if (row == 2) assert((unsigned)byte == split);
    }
    assert(fgetc(file) == EOF); fclose(file); assert(remove(path) == 0);
}

int main(void)
{
    dz_options = (struct dz_options){1, 1, 1, 1.0, NULL};
    assert(dz_initialize());
    dz_video_set_mode(DZ_MODE_VGA_400, 1);
    dz_video.display_start_row = 2;
    dz_video.line_compare = 1;
    for (unsigned y = 0; y < 400; ++y) memset(dz_video.vram + y * 320, (int)y, 320);
    assert(dz_testctl_snapshot(0, "test-vga.snap"));
    dz_testctl_poll();
    check("test-vga.snap", 320u * 400u, 2, 3, 0);

    dz_video_set_mode(DZ_MODE_EGA_350, 1);
    dz_video.display_start_row = 4;
    dz_video.line_compare = 1;
    for (unsigned y = 0; y < 350; ++y) memset(dz_video.vram + y * 640, (int)(y & 15u), 640);
    assert(dz_testctl_snapshot(1000000, "test-ega.snap"));
    dz_delay_ms(1);
    check("test-ega.snap", 640u * 350u, 4, 5, 0);
    dz_testctl_finish(); assert(!dz_testctl_failed());
    dz_shutdown();
    return 0;
}
