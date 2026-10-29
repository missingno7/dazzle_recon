#include "scr.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

static int parse_hwnd(const char *text, HWND *handle)
{
    char *end;
    errno = 0;
    int base = text[0] == '0' && (text[1] == 'x' || text[1] == 'X') ? 16 : 10;
    unsigned long long value = strtoull(text, &end, base);
    if (!*text || *text == '-' || *end || errno || value > (unsigned long long)UINTPTR_MAX)
        return 0;
    *handle = (HWND)(uintptr_t)value;
    return value != 0;
}

int dz_scr_parse_command_line(int argc, char **argv, struct dz_scr_launch *launch)
{
    launch->command = DZ_SCR_SETTINGS;
    launch->owner = NULL;
    if (argc <= 1) return 1;
    if (argc == 2 && (_stricmp(argv[1], "/s") == 0 || _stricmp(argv[1], "-s") == 0)) {
        launch->command = DZ_SCR_ANIMATE;
        return 1;
    }
    if (argc >= 2) {
        const char *arg = argv[1];
        if ((arg[0] == '/' || arg[0] == '-') && (arg[1] == 'p' || arg[1] == 'P')) {
            const char *value = arg[2] == ':' ? arg + 3 : arg[2] == 0 && argc == 3 ? argv[2] : NULL;
            if (value && argc <= (arg[2] == ':' ? 2 : 3) && parse_hwnd(value, &launch->owner)) {
                launch->command = DZ_SCR_PREVIEW;
                return 1;
            }
        }
        if ((arg[0] == '/' || arg[0] == '-') && (arg[1] == 'c' || arg[1] == 'C')) {
            const char *value = arg[2] == ':' ? arg + 3 : arg[2] == 0 && argc == 3 ? argv[2] : NULL;
            if (value && argc <= (arg[2] == ':' ? 2 : 3)) parse_hwnd(value, &launch->owner);
            return 1;
        }
    }
    return 1;
}
