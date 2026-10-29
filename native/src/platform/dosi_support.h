/* Small native-only services for DOSI and main; shared dzplat.h stays unchanged. */
#ifndef DAZZLE_DOSI_SUPPORT_H
#define DAZZLE_DOSI_SUPPORT_H
#include <stddef.h>
#include <stdint.h>
int dz_dosi_printf(const char *format, ...);
char *dz_dosi_upper(char *text);
char *dz_dosi_lower(char *text);
uint16_t dz_dosi_le16(const uint8_t *bytes);
int dz_dosi_file_exists(const char *path);
int dz_dosi_find_config(const char *name, char *path, size_t capacity);
int dz_dosi_close_configuration(void);
void dz_dosi_register_mouse(uint16_t mask);
void dz_dosi_install_signals(void);
void dz_dosi_restore_signals(void);
void dz_dosi_set_limits(uint64_t frames, uint64_t nanoseconds, const char *screenshot);
void dz_dosi_checkpoint(void);
void dz_dosi_finish(void);
int dz_dosi_output_failed(void);
#endif
