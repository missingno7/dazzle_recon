#include "platform.h"
#include "testctl.h"
#include <string.h>

extern uint16_t exit_requested;

static uint16_t queue[16];
static unsigned head, count;
static SDL_Keymod shift_state;
static int insert_state;
static int mouse_dx, mouse_dy;
static unsigned mouse_buttons;
static int saver_mouse_origin_set;
static int64_t saver_mouse_x, saver_mouse_y;
static int saver_dismissed, saver_focus_seen;
static const char *saver_dismiss_reason;
static uint64_t saver_test_start, saver_test_delay;
static int saver_test_sent, saver_test_hook;

static void screensaver_dismiss(const char *reason, const SDL_Event *event)
{
    if (saver_dismissed) return;
    saver_dismissed = 1;
    saver_dismiss_reason = reason;
    if (saver_test_start) saver_test_delay = (SDL_GetTicksNS() - saver_test_start) / UINT64_C(1000000);
    exit_requested = 1;
    dz_window_activity_dismiss(event);
}

static void screensaver_test_events(void)
{
    if (!saver_test_hook || saver_test_sent || dz_presented_frames() == 0 || dz_window_first_mode() < 0) return;
    const char *activity = getenv("DAZZLE_SCR_TEST_ACTIVITY");
    if (!activity || !*activity) return;
    saver_test_sent = 1;
    saver_test_start = SDL_GetTicksNS();
    SDL_Event event = {0};
    event.type = SDL_EVENT_MOUSE_MOTION;
    SDL_PushEvent(&event); /* The first motion is the activation baseline. */
    if (strcmp(activity, "jitter") == 0) {
        event.motion.xrel = 2;
        SDL_PushEvent(&event);
    } else if (strcmp(activity, "motion") == 0) {
        event.motion.xrel = 24;
        SDL_PushEvent(&event);
    } else if (strcmp(activity, "key") == 0) {
        memset(&event, 0, sizeof event);
        event.type = SDL_EVENT_KEY_DOWN;
        event.key.scancode = SDL_SCANCODE_A;
        SDL_PushEvent(&event);
    }
}

void dz_input_reset(void)
{
    memset(queue, 0, sizeof queue);
    head = count = 0;
    shift_state = SDL_KMOD_NONE;
    insert_state = 0;
    mouse_dx = mouse_dy = 0; mouse_buttons = 0;
    saver_mouse_origin_set = saver_mouse_x = saver_mouse_y = 0;
    saver_dismissed = saver_focus_seen = 0; saver_dismiss_reason = "none";
    saver_test_start = saver_test_delay = 0;
    saver_test_sent = 0;
    const char *test_flag = getenv("DAZZLE_SCR_TEST_HOOKS");
    saver_test_hook = dz_display.screensaver && test_flag && strcmp(test_flag, "1") == 0;
}
int dz_input_push(uint16_t key)
{
    if (!key || count == 16) return 0;
    queue[(head + count) & 15u] = key;
    ++count;
    return 1;
}

uint16_t dz_key_from_sdl(SDL_Scancode key, SDL_Keymod modifiers)
{
    int shift = (modifiers & SDL_KMOD_SHIFT) != 0;
    int ctrl = (modifiers & SDL_KMOD_CTRL) != 0;
    int alt = (modifiers & SDL_KMOD_ALT) != 0;
    unsigned scan = 0, ascii = 0;
    if (key >= SDL_SCANCODE_A && key <= SDL_SCANCODE_Z) {
        static const uint8_t scans[26] = {0x1e,0x30,0x2e,0x20,0x12,0x21,0x22,0x23,0x17,0x24,0x25,0x26,0x32,0x31,0x18,0x19,0x10,0x13,0x1f,0x14,0x16,0x2f,0x11,0x2d,0x15,0x2c};
        unsigned letter = (unsigned)(key - SDL_SCANCODE_A);
        scan = scans[letter];
        ascii = alt ? 0u : ctrl ? letter + 1u : letter +
            ((shift ^ ((modifiers & SDL_KMOD_CAPS) != 0)) ? 'A' : 'a');
    } else if (key >= SDL_SCANCODE_1 && key <= SDL_SCANCODE_0) {
        unsigned index = (unsigned)(key - SDL_SCANCODE_1);
        static const char normal[] = "1234567890", shifted[] = "!@#$%^&*()";
        scan = alt ? 0x78u + index : 2u + index;
        ascii = alt ? 0u : (uint8_t)(shift ? shifted[index] : normal[index]);
        if (ctrl && !alt) {
            if (index == 1) ascii = 0;       /* Ctrl+2 */
            else if (index == 5) ascii = 30; /* Ctrl+6 */
            else return 0;
        }
    } else if (key >= SDL_SCANCODE_F1 && key <= SDL_SCANCODE_F10) {
        scan = (unsigned)(key - SDL_SCANCODE_F1) + (alt ? 0x68u : ctrl ? 0x5eu : shift ? 0x54u : 0x3bu);
    } else {
        switch (key) {
        case SDL_SCANCODE_F11: case SDL_SCANCODE_F12:
            scan = (unsigned)(key - SDL_SCANCODE_F11) + (alt ? 0x8bu : ctrl ? 0x89u : shift ? 0x87u : 0x85u); break;
        case SDL_SCANCODE_ESCAPE: scan = 1; ascii = 27; break;
        case SDL_SCANCODE_RETURN: case SDL_SCANCODE_KP_ENTER: scan = 0x1c; ascii = ctrl ? 10u : 13u; break;
        case SDL_SCANCODE_BACKSPACE: scan = 0x0e; ascii = ctrl ? 127u : 8u; break;
        case SDL_SCANCODE_TAB: scan = alt ? 0xa5 : ctrl ? 0x94 : 0x0f; ascii = (shift || alt || ctrl) ? 0u : 9u; break;
        case SDL_SCANCODE_SPACE: scan = 0x39; ascii = 32; break;
        case SDL_SCANCODE_HOME: case SDL_SCANCODE_UP: case SDL_SCANCODE_PAGEUP:
        case SDL_SCANCODE_LEFT: case SDL_SCANCODE_RIGHT: case SDL_SCANCODE_END:
        case SDL_SCANCODE_DOWN: case SDL_SCANCODE_PAGEDOWN: case SDL_SCANCODE_INSERT: case SDL_SCANCODE_DELETE: {
            static const SDL_Scancode keys[] = {SDL_SCANCODE_HOME,SDL_SCANCODE_UP,SDL_SCANCODE_PAGEUP,SDL_SCANCODE_LEFT,SDL_SCANCODE_RIGHT,SDL_SCANCODE_END,SDL_SCANCODE_DOWN,SDL_SCANCODE_PAGEDOWN,SDL_SCANCODE_INSERT,SDL_SCANCODE_DELETE};
            static const uint8_t plain[] = {0x47,0x48,0x49,0x4b,0x4d,0x4f,0x50,0x51,0x52,0x53};
            static const uint8_t control[] = {0x77,0x8d,0x84,0x73,0x74,0x75,0x91,0x76,0x92,0x93};
            for (unsigned i = 0; i < sizeof keys / sizeof *keys; ++i)
                if (key == keys[i]) scan = alt ? plain[i] + 0x50u : ctrl ? control[i] : plain[i];
            break;
        }
        case SDL_SCANCODE_KP_0: case SDL_SCANCODE_KP_1: case SDL_SCANCODE_KP_2:
        case SDL_SCANCODE_KP_3: case SDL_SCANCODE_KP_4: case SDL_SCANCODE_KP_5:
        case SDL_SCANCODE_KP_6: case SDL_SCANCODE_KP_7: case SDL_SCANCODE_KP_8:
        case SDL_SCANCODE_KP_9: case SDL_SCANCODE_KP_PERIOD: {
            static const SDL_Scancode keys[] = {SDL_SCANCODE_KP_0,SDL_SCANCODE_KP_1,SDL_SCANCODE_KP_2,SDL_SCANCODE_KP_3,SDL_SCANCODE_KP_4,SDL_SCANCODE_KP_5,SDL_SCANCODE_KP_6,SDL_SCANCODE_KP_7,SDL_SCANCODE_KP_8,SDL_SCANCODE_KP_9,SDL_SCANCODE_KP_PERIOD};
            static const uint8_t scans[] = {0x52,0x4f,0x50,0x51,0x4b,0x4c,0x4d,0x47,0x48,0x49,0x53};
            for (unsigned i = 0; i < sizeof keys / sizeof *keys; ++i) if (keys[i] == key) {
                scan = scans[i];
                if (((modifiers & SDL_KMOD_NUM) != 0) ^ shift) ascii = i == 10 ? '.' : '0' + i;
            }
            break;
        }
        default: {
            static const struct { SDL_Scancode key; uint8_t scan; char normal, shifted; } punctuation[] = {
                {SDL_SCANCODE_MINUS,0x0c,'-','_'},{SDL_SCANCODE_EQUALS,0x0d,'=','+'},
                {SDL_SCANCODE_LEFTBRACKET,0x1a,'[','{'},{SDL_SCANCODE_RIGHTBRACKET,0x1b,']','}'},
                {SDL_SCANCODE_BACKSLASH,0x2b,'\\','|'},{SDL_SCANCODE_SEMICOLON,0x27,';',':'},
                {SDL_SCANCODE_APOSTROPHE,0x28,'\'', '"'},{SDL_SCANCODE_GRAVE,0x29,'`','~'},
                {SDL_SCANCODE_COMMA,0x33,',','<'},{SDL_SCANCODE_PERIOD,0x34,'.','>'},
                {SDL_SCANCODE_SLASH,0x35,'/','?'},{SDL_SCANCODE_KP_PLUS,0x4e,'+','+'},
                {SDL_SCANCODE_KP_MINUS,0x4a,'-','-'},{SDL_SCANCODE_KP_MULTIPLY,0x37,'*','*'},
                {SDL_SCANCODE_KP_DIVIDE,0x35,'/','/'}
            };
            for (unsigned i = 0; i < sizeof punctuation / sizeof *punctuation; ++i)
                if (punctuation[i].key == key) {
                    scan = punctuation[i].scan;
                    ascii = (uint8_t)(shift ? punctuation[i].shifted : punctuation[i].normal);
                    if (ctrl) ascii = (ascii == '[') ? 27u : (ascii == '\\') ? 28u :
                        (ascii == ']') ? 29u : (ascii == '_') ? 31u : 0u;
                    if (alt && key == SDL_SCANCODE_MINUS) scan = 0x82;
                    if (alt && key == SDL_SCANCODE_EQUALS) scan = 0x83;
                }
        } break;
        }
        if (alt) ascii = 0;
    }
    return (uint16_t)((scan << 8) | ascii);
}

void dz_input_event(const SDL_Event *event)
{
    if (dz_display.screensaver) {
        if (event->type == SDL_EVENT_KEY_DOWN) {
            screensaver_dismiss("key", event);
        } else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            screensaver_dismiss("button", event);
        } else if (event->type == SDL_EVENT_MOUSE_MOTION) {
            if (!saver_mouse_origin_set) {
                saver_mouse_origin_set = 1;
                saver_mouse_x = saver_mouse_y = 0;
            } else {
                saver_mouse_x += (int)event->motion.xrel;
                saver_mouse_y += (int)event->motion.yrel;
                if (saver_mouse_x > 3 || saver_mouse_x < -3 || saver_mouse_y > 3 || saver_mouse_y < -3 ||
                    saver_mouse_x * saver_mouse_x + saver_mouse_y * saver_mouse_y > 9)
                    screensaver_dismiss("motion", event);
            }
        } else if (event->type == SDL_EVENT_WINDOW_FOCUS_GAINED) {
            saver_focus_seen = 1;
        } else if (event->type == SDL_EVENT_WINDOW_FOCUS_LOST && saver_focus_seen &&
                   dz_window_first_mode() >= 0 && !saver_test_hook && !dz_window_saver_focused()) {
            screensaver_dismiss("focus", event);
        } else if (event->type == SDL_EVENT_QUIT || event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            screensaver_dismiss("close", event);
        }
        return;
    }
    if (dz_display.preview_parent) {
        if (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            exit_requested = 1;
        }
        return;
    }
    if (event->type == SDL_EVENT_QUIT || event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
        head = count = 0;
        dz_input_push(0x011b); /* close follows the same path as Escape */
        if (dz_keyboard_irq_handler) dz_keyboard_irq_handler();
    } else if (event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_KEY_UP) {
        shift_state = event->key.mod;
        if (dz_keyboard_irq_handler) dz_keyboard_irq_handler();
        if (event->type == SDL_EVENT_KEY_DOWN) {
            if (event->key.scancode == SDL_SCANCODE_INSERT && !event->key.repeat)
                insert_state = !insert_state;
            if ((shift_state & SDL_KMOD_ALT) && event->key.scancode == SDL_SCANCODE_RETURN) {
                if (!event->key.repeat) dz_toggle_fullscreen();
            } else dz_input_push(dz_key_from_sdl(event->key.scancode, shift_state));
        }
    } else if (event->type == SDL_EVENT_MOUSE_MOTION) {
        mouse_dx += (int)event->motion.xrel; mouse_dy += (int)event->motion.yrel;
        if (dz_mouse_event_handler) dz_mouse_event_handler();
    } else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN || event->type == SDL_EVENT_MOUSE_BUTTON_UP) {
        unsigned mask = event->button.button == SDL_BUTTON_LEFT ? 1u : event->button.button == SDL_BUTTON_RIGHT ? 2u : 4u;
        if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) mouse_buttons |= mask;
        else mouse_buttons &= ~mask;
        if (dz_mouse_event_handler) dz_mouse_event_handler();
    }
}
void dz_input_poll(void)
{
    if (dz_options.headless && (dz_display.screensaver || dz_display.preview_parent) &&
        dz_video.mode != DZ_MODE_TEXT)
        dz_options.headless = 0;
    if (dz_display.preview_parent && !dz_preview_parent_alive()) {
        exit_requested = 1;
        return;
    }
    if (dz_display.screensaver && dz_window_saver_focused()) saver_focus_seen = 1;
    screensaver_test_events();
    if (saver_test_sent && !saver_dismissed && getenv("DAZZLE_SCR_TEST_ACTIVITY") &&
        strcmp(getenv("DAZZLE_SCR_TEST_ACTIVITY"), "jitter") == 0 &&
        SDL_GetTicksNS() - saver_test_start >= UINT64_C(300000000)) {
        SDL_Event key = {0};
        key.type = SDL_EVENT_KEY_DOWN;
        key.key.scancode = SDL_SCANCODE_A;
        SDL_PushEvent(&key);
    }
    SDL_Event event;
    while (SDL_PollEvent(&event)) dz_input_event(&event);
}
int dz_input_saver_dismissed(void) { return saver_dismissed; }
uint64_t dz_input_saver_test_delay_ms(void) { return saver_test_delay; }
const char *dz_input_saver_dismiss_reason(void) { return saver_dismiss_reason; }
uint16_t dz_bioskey(int command)
{
    dz_charge(DZ_POLL_UNITS);
    if (command == 2) {
        return (uint16_t)(((shift_state & SDL_KMOD_RSHIFT) ? 1u : 0u) |
            ((shift_state & SDL_KMOD_LSHIFT) ? 2u : 0u) | ((shift_state & SDL_KMOD_CTRL) ? 4u : 0u) |
            ((shift_state & SDL_KMOD_ALT) ? 8u : 0u) | ((shift_state & SDL_KMOD_SCROLL) ? 16u : 0u) |
            ((shift_state & SDL_KMOD_NUM) ? 32u : 0u) | ((shift_state & SDL_KMOD_CAPS) ? 64u : 0u) |
            (insert_state ? 128u : 0u));
    }
    if (command == 1) return count ? queue[head] : 0;
    if (command != 0) return 0;
    while (!count && !dz_testctl_stopped()) dz_service();
    if (!count) return 0x011b;
    uint16_t result = queue[head];
    head = (head + 1u) & 15u; --count;
    return result;
}
int dz_kbhit(void) { return dz_bioskey(1) != 0; }
int dz_mouse_present(void) { return !dz_options.headless; }
void dz_mouse_counters(int *dx, int *dy, unsigned *buttons)
{
    dz_charge(DZ_POLL_UNITS);
    if (dx) *dx = mouse_dx;
    if (dy) *dy = mouse_dy;
    if (buttons) *buttons = mouse_buttons;
    mouse_dx = mouse_dy = 0;
}
