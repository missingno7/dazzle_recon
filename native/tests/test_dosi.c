/* DOSI integration tests with tiny fixtures for modules still ported in parallel.
 * RNG constants below come from the documented Borland LCG, not native snapshots. */
#include "dazzle.h"
#include "dzconst.h"
#include "dzconio.h"
#include "platform/platform.h"
#include "platform/dosi_support.h"
#include <assert.h>
#include <process.h>
#include <direct.h>
#include <windows.h>
#include <signal.h>

ImageConfigRow alt_image_records[36], ctrl_image_records[6];
FadeConfigRow fade_records[24];
char *executable_name_pointer = "DAZZLE.EXE", *help_program_name = "DAZZLE";
char *program_revision = "5.0", *registration_notice_pointer = "Registration notice";
uint16_t menu_active;
static unsigned help_calls, config_calls, pcx_calls, reserved_calls, errors, boundaries;
void help_screen(void) { ++help_calls; }
void configuration_screen(void) { ++config_calls; }
void save_pcx(void) { ++pcx_calls; }
void draw_closing_graphic(void) {}
void extract_manual(void) { dz_dosi_printf("manual extraction fixture\n"); }
void upload_reserved_dac_entries(uint16_t count) { assert(count == 3); ++reserved_calls; }
void menu_error_message(char *message) { (void)message; ++errors; }
void display_boundary_fade_and_pan(void) { ++boundaries; current_graphics_key = previous_graphics_key = 0; }

static void empty_draw(int16_t rebuild) { (void)rebuild; }
static void records(void)
{
    alt_image_count = 36; ctrl_image_count = 6; fade_count = 24;
    for (unsigned i = 0; i < 36; ++i)
        alt_image_records[i] = (ImageConfigRow){empty_draw, (uint16_t)(5 + i % 3), "Alt", (uint16_t)(i % 3 != 0), (uint16_t)(i % 2), (uint8_t)(i < 10 ? '0' + i : 'a' + i - 10)};
    for (unsigned i = 0; i < 6; ++i)
        ctrl_image_records[i] = (ImageConfigRow){empty_draw, (uint16_t)(9 + i), "Ctrl", (uint16_t)(i % 2), (uint16_t)(i % 2), (uint8_t)('a' + i)};
    for (unsigned i = 0; i < 24; ++i)
        fade_records[i] = (FadeConfigRow){(uint8_t)('a' + i), "Fade", (uint16_t)(i % 2)};
}
static void reset_options(void)
{
    initialize_program_defaults();
    option_bang_requested = option_plus_requested = split_vga_palette_updates = 0;
    palette_four_ega_colors = palette_alternate_mode = bashful_mode = use_bios_palette = 0;
    force_ega = force_vga = palette_fixed_mode = hide_ega_border = insecure_argument_index = 0;
    keep_typematic = palette_random_mode = no_disk_park = show_mode = 0;
    muted = 0;
    bios_resolution_mode = window_mode = use_8086_palette_io = option_y_requested = 0;
    show_duration_ticks = 0; muted = 1;
}
static int16_t parse(const char *argument, int16_t *position)
{
    char name[] = "dazzle", value[160];
    snprintf(value, sizeof value, "%s", argument);
    char *args[] = {name, value, NULL};
    return parse_command_line(2, args, position);
}
static void test_options(void)
{
    static const struct { const char *option; const void *target; uint16_t value; } cases[] = {
        {"/!", &option_bang_requested, 1}, {"/+", &option_plus_requested, 1},
        {"/2", &split_vga_palette_updates, 1}, {"/4", &palette_four_ega_colors, 1},
        {"/A", &palette_alternate_mode, 1}, {"/B", &bashful_mode, 1},
        {"/C", &use_bios_palette, 1}, {"/D", &draw_delay_count, 256},
        {"/E", &force_ega, 1}, {"/F", &palette_fixed_mode, 1},
        {"/H", &hide_ega_border, 1}, {"/I", &insecure_argument_index, 1},
        {"/K", &keep_typematic, 1}, {"/M", &palette_random_mode, 1},
        {"/N", &no_disk_park, 1}, {"/Q", &muted, 1}, {"/S", &show_mode, 1},
        {"/T", &bios_resolution_mode, 1}, {"/V", &force_vga, 1},
        {"/W", &window_mode, 1}, {"/X", &use_8086_palette_io, 1},
        {"/Y", &option_y_requested, 1}, {"/d=8191", &draw_delay_count, 8191},
        {"/d=0", &draw_delay_count, 1}, {"/d=65537", &draw_delay_count, 1},
        {"/s=3600", &show_duration_ticks, 64800}
    };
    int16_t position;
    for (unsigned i = 0; i < sizeof cases / sizeof *cases; ++i) {
        reset_options(); muted = 0;
        uint64_t calls = dz_rand_calls();
        assert(parse(cases[i].option, &position) == 0);
        uint16_t actual; memcpy(&actual, cases[i].target, sizeof actual);
        assert(actual == cases[i].value);
        assert(dz_rand_calls() == calls);
        assert(position == (cases[i].target == &insecure_argument_index ? 1 : -1));
    }
    reset_options(); assert(parse("-a/m/q", &position) == 0);
    assert(!palette_alternate_mode && palette_random_mode && muted);
    reset_options(); assert(parse("-ma", &position) == 0);
    assert(palette_alternate_mode && palette_random_mode);
    assert(parse("-*", &position) == -1);
    assert(parse("*", &position) == -1 && !strcmp(configuration_filename, default_config_name));
    assert(parse("custom", &position) == 1 && !strcmp(configuration_filename, "custom.DZL"));
    assert(parse("mixed.Name", &position) == 1 && !strcmp(configuration_filename, "mixed.Name"));
    assert(parse("-/i", &position) == 0 && position == 2);
    char immutable[] = "/q", before[3]; memcpy(before, immutable, 3);
    char *args[] = {"dazzle", immutable};
    assert(parse_command_line(2, args, &position) == 0 && !memcmp(before, immutable, 3));
}
static char temporary[MAX_PATH], original_directory[MAX_PATH];
static void begin_directory(void)
{
    char root[MAX_PATH]; assert(GetTempPathA(sizeof root, root));
    assert(_getcwd(original_directory, sizeof original_directory));
    snprintf(temporary, sizeof temporary, "%sdazzle-dosi-%lu", root, (unsigned long)GetCurrentProcessId());
    assert(_mkdir(temporary) == 0); assert(_chdir(temporary) == 0);
}
static void end_directory(void)
{
    assert(_chdir(original_directory) == 0); assert(_rmdir(temporary) == 0);
}
static void test_configuration(void)
{
    ImageConfigRow alt[36], ctrl[6]; FadeConfigRow fades[24];
    char path[] = "RoundTrip.DZL";
    records(); maximum_image_size = 300;
    minimum_images_per_display = 4; maximum_images_per_display = 11; auto_fade_pause_ticks = 126;
    memcpy(alt, alt_image_records, sizeof alt); memcpy(ctrl, ctrl_image_records, sizeof ctrl);
    memcpy(fades, fade_records, sizeof fades);
    assert(!save_configuration_file(path) && !configuration_file_stream);
    FILE *file = fopen(path, "rt"); assert(file);
    char contents[8192]; size_t count = fread(contents, 1, sizeof contents - 1, file); contents[count] = 0;
    fclose(file); assert(strstr(contents, "#10500;") && strstr(contents, "$I [004]") && strstr(contents, "$F [007]"));
    memset(alt_image_records, 0, sizeof alt_image_records);
    memset(ctrl_image_records, 0, sizeof ctrl_image_records);
    memset(fade_records, 0, sizeof fade_records);
    /* Keys are stable descriptors, not loaded by the configuration format. */
    for (unsigned i = 0; i < 36; ++i) alt_image_records[i].key = alt[i].key;
    for (unsigned i = 0; i < 6; ++i) ctrl_image_records[i].key = ctrl[i].key;
    for (unsigned i = 0; i < 24; ++i) fade_records[i].key = fades[i].key;
    minimum_images_per_display = maximum_images_per_display = auto_fade_pause_ticks = 0;
    assert(!load_configuration_file(path) && !configuration_file_stream);
    for (unsigned i = 0; i < 36; ++i) assert(alt_image_records[i].size == alt[i].size);
    for (unsigned i = 0; i < 6; ++i) assert(ctrl_image_records[i].size == ctrl[i].size);
    for (unsigned i = 0; i < 24; ++i) assert(fade_records[i].enabled == fades[i].enabled);
    assert(minimum_images_per_display == 4 && maximum_images_per_display == 11 && auto_fade_pause_ticks == 126);
    file = fopen(path, "wt"); assert(file);
    fputs("#10500\n; ignored\n  ~0 [9999] ; cap\n^a [-1]\n$I [-7]\n$X [257]\n$F [3]\nA [Y]\n`0 [7]\n~1 [oops]\n", file); fclose(file);
    assert(!load_configuration_file(path));
    assert(alt_image_records[0].size == 300 && ctrl_image_records[0].size == 300);
    assert(alt_image_records[1].size == alt[1].size);
    assert(minimum_images_per_display == 1 && maximum_images_per_display == 256 && auto_fade_pause_ticks == 54);
    assert(fade_records[0].enabled == 1);
    file = fopen(path, "wt"); assert(file); fputs("#bad\n", file); fclose(file);
    assert(load_configuration_file(path) == 1 && !configuration_file_stream && errors == 1);
    strcpy(configuration_filename, path); assert(!resolve_startup_configuration(-1));
    file = fopen(path, "wt"); assert(file); fputs("#10500\n", file); fclose(file);
    assert(!resolve_startup_configuration(1)); assert(!strcmp(configuration_filename, path));
    assert(remove(path) == 0);
    assert(load_configuration_file(path) == 1 && !configuration_file_stream);
    char exec[] = "Mixed.ExE"; assert(reject_exec_filename(exec) == 1);
    assert(!strcmp(exec, default_config_name));
}
static void key(uint16_t word)
{
    assert(dz_input_push(word)); keyboard_pending_isr(); assert(!poll_graphics_keys());
}
static unsigned build_calls, upload_calls;
static void palette_build(int16_t update) { assert(update == 1); ++build_calls; }
static void palette_upload(uint8_t *palette) { assert(palette == palette_buffer + palette_cycle_offset); ++upload_calls; }
static void test_input(void)
{
    uint8_t buffer[2600] = {0};
    reset_options(); records(); exit_requested = input_pending = mouse_abort_pending = 0;
    startup_active = menu_active = palette_callback_active = show_mode = 0;
    video_adapter = 8; previous_graphics_key = current_graphics_key = 'p';
    palette_cycle_ticks = 8; key('+'); assert(palette_cycle_ticks == 4 && current_graphics_key == 'p');
    key('-'); assert(palette_cycle_ticks == 6);
    palette_cycle_ticks = 1; key('+'); assert(palette_cycle_ticks == 1);
    palette_cycle_ticks = 64; key('-'); assert(palette_cycle_ticks == 64);
    palette_step = 3; key(BIOS_KEY_PAGE_UP); assert(palette_step == 6);
    key(BIOS_KEY_PAGE_DOWN); assert(palette_step == 3);
    key(BIOS_KEY_PAGE_DOWN); assert(palette_step == 3);
    palette_step = 57; key(BIOS_KEY_PAGE_UP); assert(palette_step == 57);
    palette_buffer = buffer; palette_cycle_offset = 9;
    palette_build_callback = palette_build; palette_upload_callback = palette_upload;
    key(BIOS_KEY_SHIFT_F10); assert(build_calls == 1 && upload_calls == 1 && !palette_callback_active);
    ega_green_limit = 31; ega_blue_limit = 63;
    dz_srand(1); key(BIOS_KEY_CTRL_F10); assert(reserved_calls == 1 && upload_calls == 2);
    menu_active = 1; key(BIOS_KEY_SHIFT_F10); key(BIOS_KEY_CTRL_F10);
    assert(build_calls == 1 && upload_calls == 2);
    key(BIOS_KEY_PAGE_UP); assert(current_graphics_key == BIOS_KEY_PAGE_UP);
    menu_active = 0;
    key(BIOS_KEY_F1); key(BIOS_KEY_F2); assert(help_calls == 1 && config_calls == 1);
    key(BIOS_KEY_F5); assert(pcx_calls == 1);
    startup_active = 1; key(BIOS_KEY_F5); assert(pcx_calls == 1); startup_active = 0;
    key(0x1000); assert((current_graphics_key & 255u) == 0x8a); /* Alt-Q */
    key(0x1e01); assert(current_graphics_key == 1); /* Ctrl-A */
    key('H'); assert(current_graphics_key == 'h');
    semi_auto_mode = 0; key(BIOS_KEY_ALT_F9); assert(semi_auto_mode == 1 && current_graphics_key == 'n');
    key(BIOS_KEY_F8); assert(semi_auto_continuous);
    key(BIOS_KEY_F8); assert(!semi_auto_continuous);
    key(BIOS_KEY_ALT_F9); assert(!semi_auto_mode);
    display_freeze_active = display_freeze_cycle_requested = 0;
    /* Nested dispatcher consumes TAB (enable cycling), then Space (resume). */
    assert(dz_input_push(9)); assert(dz_input_push(9)); assert(dz_input_push(' ')); input_pending = 3;
    assert(!poll_graphics_keys()); assert(!display_freeze_active && !display_freeze_cycle_requested);
    key('n'); assert(current_graphics_key == 'n');
    display_freeze_active = 1; display_freeze_cycle_requested = 0;
    assert(freeze_display(0) == 1 && display_freeze_cycle_requested);
    assert(!freeze_display(2) && !display_freeze_active);
    keyboard_led_flags(0x70, 1); keyboard_led_flags(0x30, 0);
    assert(keyboard_led_flags(0, 0) == 0x30);
    window_mode = 1; keyboard_led_flags(0x70, 1); assert(keyboard_led_flags(0, 0) == 0x30);
    window_mode = 0; keyboard_led_flags(0x10, 2); assert(keyboard_led_flags(0, 0) == 0x10);
    startup_active = 1; mouse_abort_pending = input_pending = 1;
    assert(!poll_graphics_keys() && !mouse_abort_pending);
    startup_active = 0; mouse_abort_pending = input_pending = 1;
    assert(poll_graphics_keys() && exit_requested); exit_requested = 0;
    bashful_mode = 1; assert(dz_input_push('x')); input_pending = 1;
    assert(poll_graphics_keys() && exit_requested); bashful_mode = exit_requested = 0;
    while (dz_bioskey(1)) dz_bioskey(0);
    menu_active = 1; key(13); assert(!exit_requested); menu_active = 0;
    assert(dz_input_push(27)); input_pending = 1; assert(poll_graphics_keys() && exit_requested);
    palette_buffer = NULL;
}
static void test_callbacks_and_integrity(void)
{
    reset_options(); mouse_available = 0; current_image_header = NULL;
    install_runtime_callbacks();
    assert(timer_hook_installed && keyboard_hook_installed);
    assert(dz_timer_tick_handler == graphics_timer_tick_isr && dz_keyboard_irq_handler == keyboard_pending_isr);
    assert(!dz_mouse_event_handler);
    mouse_position(0x2a, 0, 0); assert(dz_mouse_event_handler);
    input_pending = mouse_abort_pending = 0;
    SDL_Event event = {0}; event.type = SDL_EVENT_MOUSE_MOTION; event.motion.xrel = 2;
    dz_input_event(&event); assert(!input_pending && !mouse_abort_pending);
    event.type = SDL_EVENT_MOUSE_BUTTON_DOWN; event.button.button = SDL_BUTTON_LEFT;
    dz_input_event(&event); assert(input_pending == 1 && mouse_abort_pending == 1);
    input_pending = mouse_abort_pending = 0; event.type = SDL_EVENT_MOUSE_BUTTON_UP;
    dz_input_event(&event); assert(!input_pending && !mouse_abort_pending);
    mouse_position(0x2b, 0, 0); event.type = SDL_EVENT_MOUSE_MOTION; event.motion.xrel = 2;
    dz_input_event(&event); assert(input_pending == 1 && mouse_abort_pending == 1);
    restore_runtime_callbacks(); assert(!dz_timer_tick_handler && !dz_keyboard_irq_handler && !dz_mouse_event_handler);
    assert(!timer_hook_installed && !keyboard_hook_installed);
    dz_options.original_exe = "missing-original.exe";
    computed_crc = UINT32_C(0x12345678); exit_requested = 1;
    uint64_t rng = dz_rand_calls(); check_executable_integrity();
    assert(!exit_requested && computed_crc == UINT32_C(0x12345678) && dz_rand_calls() == rng);
    uint8_t binary[530] = {'M','Z',0,0,2,0};
    binary[513] = 0x78; binary[514] = 0x56; binary[515] = 0x34; binary[516] = 0x12;
    binary[517] = 0xf4; binary[518] = 1; binary[519] = 0xe8; binary[520] = 3;
    binary[521] = 2; binary[522] = 0x88;
    FILE *file = fopen("original.exe", "wb"); assert(file); assert(fwrite(binary, 1, sizeof binary, file) == sizeof binary); fclose(file);
    dz_options.original_exe = "original.exe"; check_executable_integrity();
    assert(resource_base == 513 && expected_crc == UINT32_C(0x12345678));
    assert(serial_r1 == 500 && serial_r2 == 1000 && image_data_offset == 0x8802 && !selfcheck_stream);
    file = fopen("original.exe", "rb"); assert(file);
    /* Independently calculated CRC with polynomial 0x00102100 over binary[0:512]. */
    uint32_t crc = genCRC(file, 513); fclose(file);
    assert(crc == UINT32_C(0xc6735000));
    assert(remove("original.exe") == 0);
    dz_options.original_exe = NULL;
    uint64_t ns = dz_virtual_ns(); muted = 1; beep(440, 100); beep_float(440, 100); mode_ready_chirp(); play_tone_sequence();
    assert(dz_virtual_ns() == ns);
    struct exception exception = {4, NULL, 0, 0, 0}; assert(matherr(&exception) == 1);
}
static uint16_t selected[12], symmetry[12], work_limits[12];
static int used_ctrl, used_alt;
static unsigned selected_count, stop_after;
static void auto_draw(int16_t rebuild)
{
    if (rebuild) {
        assert(selected_count < 12);
        selected[selected_count] = current_image_index;
        work_limits[selected_count] = image_work_limit;
        symmetry[selected_count++] = draw_symmetry;
        if (selected_count == stop_after) exit_requested = 1;
    }
    image_work_count = (int16_t)(image_work_limit + 1u); /* one work step per image */
}
static void alt_draw(int16_t rebuild) { if (rebuild) ++used_alt; auto_draw(rebuild); }
static void ctrl_draw(int16_t rebuild) { if (rebuild) ++used_ctrl; auto_draw(rebuild); }
static void auto_plot(uint16_t draw_flags) { (void)draw_flags; dz_charge(1); }
static void setup_auto(uint16_t seed, unsigned count)
{
    reset_options(); records();
    for (unsigned i = 0; i < 36; ++i) alt_image_records[i].draw = alt_draw;
    for (unsigned i = 0; i < 6; ++i) ctrl_image_records[i].draw = ctrl_draw;
    exit_requested = startup_config_failure = semi_auto_mode = input_pending = mouse_abort_pending = 0;
    current_graphics_key = 0; image_work_limit = 0; screen_extent = 172;
    minimum_images_per_display = 3; maximum_images_per_display = 9;
    palette_cycle_ticks = UINT16_MAX; palette_elapsed_ticks = 0;
    plot_callback = auto_plot; selected_count = boundaries = 0; used_ctrl = used_alt = 0; stop_after = count; dz_srand(seed);
}
static void test_auto(void)
{
    setup_auto(1, 12); auto_loop();
    const uint16_t expected[] = {0,15,30,4,1,32,9,39,26,30,5,24};
    const uint16_t expected_symmetry[] = {1,1,1,3,1,3,1,3,3,1,3,1};
    const uint16_t expected_limits[] = {243,676,451,840,1099,1456,1034,1326,1023,564,872,962};
    assert(selected_count == 12 && !memcmp(selected, expected, sizeof expected));
    assert(!memcmp(symmetry, expected_symmetry, sizeof symmetry));
    assert(!memcmp(work_limits, expected_limits, sizeof work_limits));
    assert(dz_rand_calls() == 43 && boundaries == 2);
    setup_auto(23, 1); auto_loop(); /* random index 36: the DOS Ctrl[-1] alias */
    assert(selected[0] == 36 && used_alt == 1 && !used_ctrl && symmetry[0] == 1);
    setup_auto(2, 1); auto_loop(); /* index 42's symmetry is a nonzero DOS segment */
    assert(selected[0] == 42 && used_ctrl == 1 && !used_alt && symmetry[0] == 3);
    setup_auto(1, 12);
    for (unsigned i = 0; i < 36; ++i) alt_image_records[i].size = 0;
    for (unsigned i = 0; i < 6; ++i) ctrl_image_records[i].size = 0;
    auto_loop(); assert(startup_config_failure && exit_requested && !selected_count);
    dz_dosi_set_limits(0, 1000000, NULL); dz_delay_ms(2); exit_requested = 0;
    dz_dosi_checkpoint(); assert(exit_requested); dz_dosi_set_limits(0, 0, NULL);
}
static unsigned scripted_tick;
static void semi_input_tick(void)
{
    ++scripted_tick;
    uint16_t word = scripted_tick == 1 ? 0x1e01 : scripted_tick == 2 ? BIOS_KEY_F10 : BIOS_KEY_ALT_F9;
    assert(dz_input_push(word)); keyboard_pending_isr();
}
static void test_semi_auto(void)
{
    setup_auto(1, 12); semi_auto_mode = 1; display_cleared = 1; scripted_tick = 0;
    dz_timer_tick_handler = semi_input_tick;
    semi_auto_loop();
    dz_timer_tick_handler = NULL;
    assert(!semi_auto_mode && scripted_tick == 3 && selected_count == 1);
    assert(selected[0] == 37 && used_ctrl == 1 && !used_alt && boundaries == 1);
}
/* Compile the owned native entry into this test with its app call redirected to
 * a fixture. This checks argv filtering without depending on the pending DZLE. */
static int fixture_main(int argc, char **argv);
#define main native_entry
#define dazzle_main fixture_main
#include "../src/main.c"
#undef dazzle_main
#undef main
static int fixture_main(int argc, char **argv)
{
    assert(argc == 5 && !strcmp(argv[1], "/amq") && !strcmp(argv[2], "custom.DZL"));
    assert(!strcmp(argv[3], "-h") && !strcmp(argv[4], "/s=1") && !argv[5]);
    assert(dz_options.deterministic && dz_options.headless && dz_options.seed == 41);
    assert(dz_options.speed == 2.5 && !strcmp(dz_options.original_exe, "original.exe"));
    return 0;
}
static void test_native_entry(void)
{
    char *args[] = {"dazzle", "/amq", "--headless", "--deterministic", "custom.DZL",
        "--seed", "41", "--speed", "2.5", "--original", "original.exe", "--frames", "2",
        "--exit-after", "1", "--trace-rng", "-h", "/s=1", NULL};
    assert(native_entry(18, args) == 0);
}
static void test_exits(const char *self)
{
    const char *options[] = {"/?", "/$", "/Z", "/g", "/d=8192", "/s=3601", "signal"};
    int expected[] = {10, 0, 3, 10, 10, 10, 9};
    for (unsigned i = 0; i < sizeof expected / sizeof *expected; ++i) {
        const char *args[] = {self, "--child", options[i], NULL};
        assert(_spawnv(_P_WAIT, self, args) == expected[i]);
        FILE *file = fopen("child.txt", "rt"); assert(file);
        char contents[8192]; size_t count = fread(contents, 1, sizeof contents - 1, file); contents[count] = 0; fclose(file);
        assert(strstr(contents, i == 1 ? "REGISTRATION and ORDER FORM" : i == 2 ? "manual extraction fixture" : i == 6 ? "sigerr -- program aborted" : "Additional Options"));
        assert(remove("child.txt") == 0);
    }
}
int main(int argc, char **argv)
{
    dz_options = (struct dz_options){1, 1, 1, 1.0, NULL}; assert(dz_initialize());
    records(); reset_options();
    if (argc == 3 && !strcmp(argv[1], "--child")) {
        assert(freopen("child.txt", "wt", stdout));
        if (!strcmp(argv[2], "signal")) { install_runtime_callbacks(); raise(SIGFPE); return 99; }
        int16_t position; parse(argv[2], &position); return 99;
    }
    char self[MAX_PATH]; assert(GetModuleFileNameA(NULL, self, sizeof self));
    test_options(); begin_directory(); test_configuration(); test_input();
    test_callbacks_and_integrity(); test_auto(); test_semi_auto(); test_exits(self); end_directory();
    test_native_entry();
    dz_shutdown(); puts("DOSI tests passed"); return 0;
}
