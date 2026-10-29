/* Small host adapters used only by the original DZLE startup sequence. */
#ifndef DAZZLE_DZLE_SUPPORT_H
#define DAZZLE_DZLE_SUPPORT_H
#include "dazzle.h"
void dzle_set_typematic(uint8_t delay, uint8_t rate);
void dzle_setdisk(int drive);
void dzle_install_signal_handlers(void);
#endif
