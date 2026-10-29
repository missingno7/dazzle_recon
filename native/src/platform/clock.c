#include "platform.h"
#include <math.h>

struct dz_options dz_options = {0, 1, 0, 1.0, NULL};
void (*dz_timer_tick_handler)(void);
void (*dz_keyboard_irq_handler)(void);
void (*dz_mouse_event_handler)(void);
static uint64_t virtual_ns, timer_ticks, wall_start, next_poll, next_frame;
static uint64_t frame_period = DZ_NS_PER_SECOND / 70;
static long double work_remainder;
static int dispatching;

/* 18.2065 Hz = 36413/2000, evaluated with integers, independent of the host clock. */
static uint64_t ticks_at(uint64_t ns)
{
    const uint64_t denominator = UINT64_C(2000000000000);
    return (ns / denominator) * 36413u + (ns % denominator) * 36413u / denominator;
}

void dz_clock_reset(void)
{
    virtual_ns = timer_ticks = next_poll = next_frame = 0;
    dispatching = 0;
    work_remainder = 0;
    frame_period = DZ_NS_PER_SECOND / 70;
    wall_start = 0;
    if (!dz_options.deterministic) {
        wall_start = SDL_GetTicksNS();
        SDL_DisplayID display = SDL_GetPrimaryDisplay();
        const SDL_DisplayMode *mode = SDL_GetCurrentDisplayMode(display);
        if (mode && mode->refresh_rate > 1.0f)
            frame_period = (uint64_t)((double)DZ_NS_PER_SECOND / mode->refresh_rate);
    }
}

static void advance(uint64_t ns)
{
    if (dispatching) return; /* BIOS peeks inside an ISR do not recursively charge time. */
    dispatching = 1;
    const uint64_t target = virtual_ns + ns;
    do {
        uint64_t end = next_poll < target ? next_poll : target;
        if (end < virtual_ns) end = virtual_ns;
        const uint64_t target_ticks = ticks_at(end);
        while (timer_ticks < target_ticks) {
            ++timer_ticks;
            /* Split quotient/remainder so a long-running saver cannot overflow
             * the intermediate product after roughly six days. */
            virtual_ns = (timer_ticks / 36413u) * UINT64_C(2000000000000) +
                ((timer_ticks % 36413u) * UINT64_C(2000000000000) + 36412u) / 36413u;
            if (dz_timer_tick_handler) dz_timer_tick_handler();
        }
        virtual_ns = end;
        if (virtual_ns >= next_poll) {
            dz_input_poll();
            dz_audio_service();
            next_poll = virtual_ns + UINT64_C(1000000);
            if (!dz_options.deterministic) {
                uint64_t elapsed = SDL_GetTicksNS() - wall_start;
                if (virtual_ns > elapsed) SDL_DelayPrecise(virtual_ns - elapsed);
            }
            if (virtual_ns >= next_frame) {
                dz_present();
                next_frame += frame_period;
            }
        }
    } while (virtual_ns < target);
    dispatching = 0;
}

void dz_charge(uint32_t work_units)
{
    if (dispatching) return;
    double speed = dz_options.speed;
    if (!(speed > 0.0) || speed > 1000000.0) speed = 1.0;
    long double ns = work_remainder + (long double)work_units * DZ_NS_PER_SECOND /
        ((long double)DZ_WORK_UNITS_PER_SECOND * speed);
    uint64_t whole = (uint64_t)ns;
    work_remainder = ns - whole;
    advance(whole);
}
void dz_wait_retrace(void)
{
    uint64_t frame = (virtual_ns / DZ_NS_PER_SECOND) * 70u +
        (virtual_ns % DZ_NS_PER_SECOND) * 70u / DZ_NS_PER_SECOND;
    uint64_t next = frame + 1u;
    uint64_t boundary = (next / 70u) * DZ_NS_PER_SECOND +
        ((next % 70u) * DZ_NS_PER_SECOND + 69u) / 70u;
    advance(boundary - virtual_ns);
}
void dz_delay_ms(unsigned ms) { advance((uint64_t)ms * UINT64_C(1000000)); }
void dz_service(void) { dz_charge(1); }
uint64_t dz_virtual_ticks(void) { return timer_ticks; }
uint64_t dz_virtual_ns(void) { return virtual_ns; }
uint32_t dz_bios_time(void) { return (uint32_t)(timer_ticks % UINT64_C(0x1800b0)); }

int dz_initialize(void)
{
    SDL_InitFlags flags = SDL_INIT_EVENTS;
    if (!dz_options.headless) flags |= SDL_INIT_VIDEO;
    if (!SDL_Init(flags)) return 0;
    dz_input_reset();
    dz_clock_reset();
    dz_srand(dz_options.seed);
    dz_video_set_mode(DZ_MODE_TEXT, 1);
    return 1;
}
void dz_shutdown(void)
{
    dz_sound_shutdown();
    dz_video_shutdown();
    dz_timer_tick_handler = dz_keyboard_irq_handler = dz_mouse_event_handler = NULL;
    SDL_Quit();
}
