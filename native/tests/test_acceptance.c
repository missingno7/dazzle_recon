/* Behavioural input acceptance, including the DOS INT 33h event mask.
 * The poll seam also permits bounded whole-program mouse captures from scratch. */
#include "dazzle.h"
#include "platform/platform.h"
#include "platform/dosi_support.h"
#include "platform/testctl.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static int capture_mouse, sent_mouse;

/* Tests link this definition before the platform library (CMake's test seam).
 * All keyboard/IRQ and mouse event handling still use the real platform code. */
void dz_input_poll(void)
{
    if (capture_mouse && !sent_mouse && dz_virtual_ns() >= UINT64_C(5000000000)) {
        SDL_Event event = {0};
        event.type = capture_mouse == 2 ? SDL_EVENT_MOUSE_BUTTON_DOWN : SDL_EVENT_MOUSE_MOTION;
        event.motion.xrel = 24;
        event.motion.yrel = 12;
        if (capture_mouse == 2) event.button.button = SDL_BUTTON_LEFT;
        sent_mouse = 1;
        dz_input_event(&event);
    }
}

static void test_mouse_mask(void)
{
    startup_active = 0;
    mouse_available = 0;
    bashful_mode = 0;
    exit_requested = input_pending = mouse_abort_pending = 0;
    install_runtime_callbacks();
    assert(dz_mouse_event_handler);
    SDL_Event event = {0};
    event.type = SDL_EVENT_MOUSE_MOTION;
    event.motion.xrel = 24;
    event.motion.yrel = 12;
    dz_input_event(&event);
    assert(!input_pending && !mouse_abort_pending);
    assert(!poll_graphics_keys() && !exit_requested);

    event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    event.button.button = SDL_BUTTON_LEFT;
    dz_input_event(&event);
    assert(input_pending == 1 && mouse_abort_pending == 1);
    assert(poll_graphics_keys() && exit_requested && !mouse_abort_pending);
    restore_runtime_callbacks();

    dz_input_reset();
    bashful_mode = 1;
    exit_requested = input_pending = mouse_abort_pending = 0;
    install_runtime_callbacks();
    event.type = SDL_EVENT_MOUSE_MOTION;
    event.motion.xrel = 24;
    event.motion.yrel = 12;
    dz_input_event(&event);
    assert(input_pending == 1 && mouse_abort_pending == 1);
    assert(poll_graphics_keys() && exit_requested);
    restore_runtime_callbacks();
}

static void test_key_release(void)
{
    dz_input_reset();
    bashful_mode = exit_requested = input_pending = 0;
    dz_keyboard_irq_handler = keyboard_pending_isr;
    SDL_Event event = {0};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.scancode = SDL_SCANCODE_F10;
    dz_input_event(&event);
    assert(input_pending == 1 && dz_bioskey(0) == 0x4400);
    input_pending = 0;
    event.type = SDL_EVENT_KEY_UP;
    dz_input_event(&event);
    /* Semi-Auto's F10 drawing loop stops on this IRQ notice. A release has
     * no BIOS word and must not execute the F10 command a second time. */
    assert(input_pending == 1 && !dz_bioskey(1));
    assert(!poll_graphics_keys() && !input_pending && !exit_requested);
    dz_keyboard_irq_handler = NULL;
}

int main(int argc, char **argv)
{
    dz_options = (struct dz_options){1, 1, 1, 1.0, NULL};
    assert(dz_initialize());
    /* A dummy SDL video driver has no physical mouse; model its presence after
     * initialization so INT 33h registration is exercised without a window. */
    dz_options.headless = 0;
    if (argc == 4 && !strcmp(argv[1], "--capture-mouse")) {
        capture_mouse = !strcmp(argv[2], "click") ? 2 : 1;
        int bashful = !strcmp(argv[2], "bashful");
        dz_options.original_exe = argv[3];
        assert(dz_testctl_snapshot(UINT64_C(4500000000), "t4.5.snap"));
        if (!bashful) assert(dz_testctl_snapshot(UINT64_C(5500000000), "t5.5.snap"));
        if (!bashful) assert(dz_testctl_snapshot(UINT64_C(7000000000), "t7.snap"));
        dz_dosi_set_limits(0, UINT64_C(8000000000), NULL);
        char *args[] = {"dazzle", bashful ? "/B" : "*", NULL};
        int result = dazzle_main(2, args);
        dz_testctl_finish();
        dz_shutdown();
        return dz_testctl_failed() ? 1 : result;
    }
    test_mouse_mask();
    test_key_release();
    dz_shutdown();
    puts("Auto ignores mouse motion, mouse press aborts; /B includes motion");
    return 0;
}
