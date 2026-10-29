#include <assert.h>
#include <stdint.h>
#include "scr.h"

static void expect(int argc, char **argv, enum dz_scr_command command, uintptr_t owner)
{
    struct dz_scr_launch launch;
    assert(dz_scr_parse_command_line(argc, argv, &launch));
    assert(launch.command == command);
    assert((uintptr_t)launch.owner == owner);
}

int main(void)
{
    char *none[] = {"dazzle.scr"};
    char *save_upper[] = {"dazzle.scr", "/S"};
    char *save_dash[] = {"dazzle.scr", "-s"};
    char *config[] = {"dazzle.scr", "/c"};
    char *config_handle[] = {"dazzle.scr", "-C:1234"};
    char *config_separate[] = {"dazzle.scr", "/c", "0x1234"};
    char *preview[] = {"dazzle.scr", "/p", "4321"};
    char *preview_colon[] = {"dazzle.scr", "-P:0x4321"};
    char *unknown[] = {"dazzle.scr", "/unknown"};
    char *bad_preview[] = {"dazzle.scr", "/p:not-a-handle"};
    expect(1, none, DZ_SCR_SETTINGS, 0);
    expect(2, save_upper, DZ_SCR_ANIMATE, 0);
    expect(2, save_dash, DZ_SCR_ANIMATE, 0);
    expect(2, config, DZ_SCR_SETTINGS, 0);
    expect(2, config_handle, DZ_SCR_SETTINGS, 1234);
    expect(3, config_separate, DZ_SCR_SETTINGS, 0x1234);
    expect(3, preview, DZ_SCR_PREVIEW, 4321);
    expect(2, preview_colon, DZ_SCR_PREVIEW, 0x4321);
    expect(2, unknown, DZ_SCR_SETTINGS, 0);
    expect(2, bad_preview, DZ_SCR_SETTINGS, 0);
    return 0;
}
