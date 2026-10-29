#include "dzle_support.h"
#include <direct.h>
#include <signal.h>

/* The native input queue uses host repeat settings; no BIOS typematic hardware. */
void dzle_set_typematic(uint8_t delay, uint8_t rate) { (void)delay; (void)rate; }
void dzle_setdisk(int drive) { (void)_chdrive(drive + 1); }

static void diagnostic(int number) { signal_diagnostic((int16_t)number, 0, NULL); }
void dzle_install_signal_handlers(void)
{
    signal(SIGABRT, diagnostic);
    signal(SIGFPE, diagnostic);
    signal(SIGILL, diagnostic);
    signal(SIGSEGV, diagnostic);
}
