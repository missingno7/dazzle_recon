#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "dazzle.h"
#include "platform/platform.h"
#include "scr.h"
#include "resource.h"

static HINSTANCE module_instance;
static HANDLE saver_mutex;

static void close_saver_mutex(void)
{
    if (saver_mutex) CloseHandle(saver_mutex);
    saver_mutex = NULL;
}

static char **narrow_arguments(int *count)
{
    int wide_count = 0;
    wchar_t **wide = CommandLineToArgvW(GetCommandLineW(), &wide_count);
    if (!wide) return NULL;
    char **arguments = calloc((size_t)wide_count + 1, sizeof *arguments);
    if (!arguments) { LocalFree(wide); return NULL; }
    for (int i = 0; i < wide_count; ++i) {
        int bytes = WideCharToMultiByte(CP_ACP, 0, wide[i], -1, NULL, 0, NULL, NULL);
        arguments[i] = bytes > 0 ? malloc((size_t)bytes) : NULL;
        if (!arguments[i] || !WideCharToMultiByte(CP_ACP, 0, wide[i], -1, arguments[i], bytes, NULL, NULL)) {
            for (int j = 0; j <= i; ++j) free(arguments[j]);
            free(arguments); LocalFree(wide); return NULL;
        }
    }
    LocalFree(wide);
    *count = wide_count;
    return arguments;
}

static void free_arguments(char **arguments, int count)
{
    if (!arguments) return;
    for (int i = 0; i < count; ++i) free(arguments[i]);
    free(arguments);
}

static int show_settings(HWND owner)
{
    INITCOMMONCONTROLSEX common = {sizeof common, ICC_TAB_CLASSES | ICC_LISTVIEW_CLASSES | ICC_UPDOWN_CLASS};
    InitCommonControlsEx(&common);
    return DialogBoxParamA(module_instance, MAKEINTRESOURCEA(IDD_SETTINGS), owner,
        dz_scr_settings_dialog, 0) == IDOK;
}

static int prepare_config_argument(const char *path, char buffer[80])
{
    if (!path || !*path) return 0;
    char directory[4096];
    size_t length = strlen(path);
    if (length >= sizeof directory) return 0;
    memcpy(directory, path, length + 1);
    char *slash = strrchr(directory, '\\');
    if (!slash) slash = strrchr(directory, '/');
    const char *filename = path;
    if (slash) {
        filename = slash + 1;
        if (slash == directory) slash[1] = 0;
        else *slash = 0;
        if (!SetCurrentDirectoryA(directory)) return 0;
    }
    if (strlen(filename) >= 80) return 0;
    strcpy(buffer, filename);
    return 1;
}

static int run_engine(HWND preview_parent)
{
    struct dz_scr_settings settings;
    char dzl_path[4096] = "";
    dz_scr_settings_load(&settings, dzl_path, sizeof dzl_path);
    dz_scr_settings_apply_globals(&settings);
    dz_display.screensaver = preview_parent == NULL;
    dz_display.preview_parent = preview_parent;
    dz_display.fullscreen = preview_parent == NULL;
    dz_options.headless = 0;
    dz_options.deterministic = 0;
    dz_options.speed = 1.0;
    dz_options.seed = (uint16_t)time(NULL);
    if (!dz_initialize()) return 1;
    atexit(dz_window_preview_cleanup);
    atexit(dz_shutdown);
    /* DOSI initializes text bookkeeping first; keep it offscreen until graphics mode is live. */
    dz_options.headless = 1;

    char executable[MAX_PATH] = "DAZZLE.SCR";
    GetModuleFileNameA(module_instance, executable, sizeof executable);
    char delay[24];
    snprintf(delay, sizeof delay, "/D=%u", settings.draw_delay);
    char config[80];
    int have_config = prepare_config_argument(dzl_path, config);
    char *arguments[12];
    int argc = 0;
    arguments[argc++] = executable;
    arguments[argc++] = "/W";
    arguments[argc++] = "/B";
    if (!settings.sound) arguments[argc++] = "/Q";
    switch (settings.mode) {
    case DZ_SCR_VGA200: arguments[argc++] = "/V"; arguments[argc++] = "/T"; break;
    case DZ_SCR_EGA350: arguments[argc++] = "/E"; arguments[argc++] = "/T"; break;
    case DZ_SCR_EGA700: arguments[argc++] = "/E"; break;
    default: arguments[argc++] = "/V"; break;
    }
    if (settings.palette == DZ_SCR_PALETTE_ALTERNATE) arguments[argc++] = "/A";
    else if (settings.palette == DZ_SCR_PALETTE_RANDOM) arguments[argc++] = "/M";
    else if (settings.palette == DZ_SCR_PALETTE_FIXED) arguments[argc++] = "/F";
    arguments[argc++] = delay;
    arguments[argc++] = have_config ? config : "/*";
    int result = dazzle_main(argc, arguments);
    const char *test_hook = getenv("DAZZLE_SCR_TEST_HOOKS");
    if (dz_display.screensaver && test_hook && strcmp(test_hook, "1") == 0) {
        char receipt[256];
        int length = snprintf(receipt, sizeof receipt,
            "scr_test: first_mode=%d dismissed=%d reason=%s delay_ms=%llu result=%d\n",
            dz_window_first_mode(), dz_input_saver_dismissed(), dz_input_saver_dismiss_reason(),
            (unsigned long long)dz_input_saver_test_delay_ms(), result);
        HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD written;
        if (output && output != INVALID_HANDLE_VALUE) WriteFile(output, receipt, (DWORD)length, &written, NULL);
    }
    return result;
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR command_line, int show_command)
{
    (void)previous; (void)command_line; (void)show_command;
    module_instance = instance;
    int argc = 0;
    char **argv = narrow_arguments(&argc);
    if (!argv) return 1;
    struct dz_scr_launch launch;
    dz_scr_parse_command_line(argc, argv, &launch);
    free_arguments(argv, argc);
    if (launch.command == DZ_SCR_SETTINGS) {
        HWND owner = launch.owner && IsWindow(launch.owner) ? launch.owner : NULL;
        show_settings(owner);
        return 0;
    }
    if (launch.command == DZ_SCR_PREVIEW) {
        if (!launch.owner || !IsWindow(launch.owner)) return 0;
        return run_engine(launch.owner);
    }
    saver_mutex = CreateMutexA(NULL, TRUE, "Local\\DAZZLE50ScreenSaver");
    if (!saver_mutex) return 0;
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        close_saver_mutex();
        return 0;
    }
    atexit(close_saver_mutex);
    int result = run_engine(NULL);
    return result;
}
