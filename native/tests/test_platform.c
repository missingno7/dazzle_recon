/* Platform semantics; RNG values/seed width cross-checked against n-ref's
 * actual Borland RTL capture in native/tests/ref/data/rng.txt. */
#include "dazzle.h"
#include "platform/platform.h"
#include <assert.h>
#include <stdio.h>

static unsigned ticks_seen, irqs_seen;
static void tick(void) { ++ticks_seen; assert(dz_virtual_ticks() == ticks_seen); }
static void irq(void) { ++irqs_seen; }

static void test_rng(void)
{
    static const int16_t first[] = {346,130,10982,1090,11656,7117,17595,6415,22948,31126};
    dz_srand(1);
    for (unsigned i = 0; i < sizeof first / sizeof *first; ++i) assert(dz_rand() == first[i]);
    assert(dz_rand_calls() == 10);
    dz_srand(1); assert(dz_random((uint16_t)256) == 2);
    dz_srand(1); assert(dz_random((int16_t)-32768) == -346);
    dz_srand(1); assert(dz_random((uint16_t)65535) == 691);
    dz_srand(0x10001); assert(dz_rand() == 346); /* actual RTL srand width */
    dz_srand(1); assert(dz_random(INT32_MAX) == 0); /* wrapped product -346 */
    assert(dz_rand_calls() == 1);
}
static void test_clock(void)
{
    dz_clock_reset(); ticks_seen = 0;
    dz_timer_tick_handler = tick;
    dz_delay_ms(54); assert(dz_virtual_ticks() == 0);
    dz_delay_ms(1); assert(dz_virtual_ticks() == 1);
    dz_delay_ms(945); assert(dz_virtual_ticks() == 18 && dz_bios_time() == 18);
    dz_delay_ms(19000); assert(dz_virtual_ticks() == 364 && ticks_seen == 364);
    assert(dz_virtual_ns() == UINT64_C(20000000000));
    dz_timer_tick_handler = NULL;
    dz_clock_reset();
    for (int i = 0; i < 70; ++i) dz_wait_retrace();
    assert(dz_virtual_ns() == DZ_NS_PER_SECOND);
    assert(dz_virtual_ticks() == 18);
    dz_clock_reset(); dz_charge(DZ_WORK_UNITS_PER_SECOND);
    assert(dz_virtual_ns() == DZ_NS_PER_SECOND);
    dz_options.speed = 2.0;
    dz_clock_reset(); dz_charge(DZ_WORK_UNITS_PER_SECOND);
    assert(dz_virtual_ns() == DZ_NS_PER_SECOND / 2);
    dz_options.speed = 1.0;
}
static void test_keys(void)
{
    assert(dz_key_from_sdl(SDL_SCANCODE_ESCAPE, 0) == 0x011b);
    assert(dz_key_from_sdl(SDL_SCANCODE_A, 0) == 0x1e61);
    assert(dz_key_from_sdl(SDL_SCANCODE_A, SDL_KMOD_SHIFT) == 0x1e41);
    assert(dz_key_from_sdl(SDL_SCANCODE_A, SDL_KMOD_CTRL) == 0x1e01);
    assert(dz_key_from_sdl(SDL_SCANCODE_A, SDL_KMOD_ALT) == 0x1e00);
    assert(dz_key_from_sdl(SDL_SCANCODE_1, SDL_KMOD_ALT) == 0x7800);
    assert(dz_key_from_sdl(SDL_SCANCODE_F10, 0) == 0x4400);
    assert(dz_key_from_sdl(SDL_SCANCODE_F10, SDL_KMOD_SHIFT) == 0x5d00);
    assert(dz_key_from_sdl(SDL_SCANCODE_F10, SDL_KMOD_CTRL) == 0x6700);
    assert(dz_key_from_sdl(SDL_SCANCODE_F9, SDL_KMOD_ALT) == 0x7000);
    assert(dz_key_from_sdl(SDL_SCANCODE_F10, SDL_KMOD_ALT) == 0x7100);
    assert(dz_key_from_sdl(SDL_SCANCODE_HOME, 0) == 0x4700);
    assert(dz_key_from_sdl(SDL_SCANCODE_LEFT, SDL_KMOD_CTRL) == 0x7300);
    assert(dz_key_from_sdl(SDL_SCANCODE_KP_8, SDL_KMOD_NUM) == 0x4838);
    dz_input_reset();
    for (uint16_t i = 1; i <= 16; ++i) assert(dz_input_push(i));
    assert(!dz_input_push(17));
    for (uint16_t i = 1; i <= 16; ++i) {
        assert(dz_bioskey(1) == i); assert(dz_bioskey(1) == i);
        assert(dz_bioskey(0) == i);
    }
    assert(!dz_kbhit());
    SDL_Event event = {0};
    dz_keyboard_irq_handler = irq; irqs_seen = 0;
    event.type = SDL_EVENT_KEY_DOWN; event.key.scancode = SDL_SCANCODE_F10;
    event.key.mod = SDL_KMOD_RSHIFT | SDL_KMOD_CTRL;
    dz_input_event(&event);
    assert(dz_bioskey(2) == 5); assert(dz_bioskey(0) == 0x6700);
    event.type = SDL_EVENT_KEY_UP; event.key.mod = SDL_KMOD_NONE;
    dz_input_event(&event); assert(dz_bioskey(2) == 0 && irqs_seen == 2);
    event.type = SDL_EVENT_KEY_DOWN; event.key.scancode = SDL_SCANCODE_INSERT;
    dz_input_event(&event); assert(dz_bioskey(2) == 128 && dz_bioskey(0) == 0x5200);
    assert(dz_input_push(0x1e61));
    event.type = SDL_EVENT_QUIT; dz_input_event(&event);
    assert(dz_bioskey(0) == 0x011b && !dz_kbhit() && irqs_seen == 4);
    dz_keyboard_irq_handler = NULL;
}
static void test_video(void)
{
    static uint32_t rgba[720 * 400];
    dz_video_set_mode(DZ_MODE_VGA_400, 1);
    dz_video.dac[1][0] = 63; dz_video.dac[1][1] = dz_video.dac[1][2] = 0;
    dz_video.dac[2][1] = 63; dz_video.dac[2][0] = dz_video.dac[2][2] = 0;
    dz_video.vram[0] = 1; dz_video.vram[320 * 5] = 2;
    dz_video.display_start_row = 5; dz_video.line_compare = 1;
    dz_video_rgba(rgba);
    assert(rgba[0] == UINT32_C(0xff00ff00));
    assert(rgba[320 * 2] == UINT32_C(0xffff0000));
    dz_video_set_mode(DZ_MODE_EGA_350, 1);
    dz_video.ega_attr[1] = 0x24; dz_video.vram[0] = 1;
    dz_video_rgba(rgba); assert(rgba[0] == UINT32_C(0xffff0000));
    dz_video_set_mode(DZ_MODE_TEXT, 1);
    dz_video.cursor_visible = 0; dz_video.text[0][0][0] = 0xdb;
    dz_video.text[0][0][1] = 0x0f;
    dz_video_rgba(rgba); assert(rgba[0] == UINT32_C(0xffffffff));
    assert(rgba[8] == UINT32_C(0xffffffff)); /* IBM line-graphics range C0..DF extends column 9 */
    assert(rgba[9] == UINT32_C(0xff000000));
    dz_text_attribute(0x1e); dz_text_write("Hi\r\nX");
    assert(dz_video.text[0][0][0] == 'H' && dz_video.text[0][0][1] == 0x1e);
    assert(dz_video.text[1][0][0] == 'X' && dz_video.cursor_x == 1 && dz_video.cursor_y == 1);
}
static void test_primitives(void)
{
    static const Point16 points[] = {{1,1},{2,1},{3,2},{4,2},{5,3}};
    draw_x = draw_y = 0; assert(!line_step(5,3,1));
    for (unsigned i = 0; i < sizeof points / sizeof *points; ++i) {
        assert(line_step(5,3,0) == (i == 4));
        assert(draw_x == points[i].x && draw_y == points[i].y);
    }
    draw_x = INT16_MIN; draw_y = 0;
    assert(!line_step(INT16_MAX,0,1) && dx == 2 && sx == -1);
    video_adapter = 8; color_count = 256;
    draw_x = 300; draw_y = 200; choose_color(8);
    assert(plot_color == 173); /* abs((int16_t)60000)>>5, not abs(60000)>>5 */
}
int main(void)
{
    dz_options = (struct dz_options){1, 1, 1, 1.0, NULL};
    assert(dz_initialize());
    test_rng(); test_clock(); test_keys(); test_video(); test_primitives();
    dz_shutdown();
    puts("Platform: RNG, rational BIOS/retrace time, key buffer and palette scanout passed.");
    return 0;
}
