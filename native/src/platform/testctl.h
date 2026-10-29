/* Whole-program controls; deliberately private to the native platform. */
#ifndef DAZZLE_TESTCTL_H
#define DAZZLE_TESTCTL_H
#include <stdint.h>
void dz_testctl_limits(uint64_t frames, uint64_t ns, const char *screenshot);
int dz_testctl_script(const char *path);
int dz_testctl_snapshot(uint64_t ns, const char *path);
uint64_t dz_testctl_deadline(uint64_t target);
void dz_testctl_poll(void);
void dz_testctl_work(uint32_t units);
void dz_testctl_fade(unsigned key);
int dz_testctl_stopped(void);
int dz_testctl_failed(void);
void dz_testctl_finish(void);
#endif
