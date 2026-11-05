#ifndef DAZZLE_EXTENDED_REACH_H
#define DAZZLE_EXTENDED_REACH_H
#include <stdint.h>
/* Presentation-only suffixes of existing runs. No logical stores or services. */
void dz_extended_diagonal(int rising);
void dz_extended_field(int formula);
void dz_extended_zigstream(int rebuild, int variant);
void dz_extended_segment(unsigned effect, int ax, int ay, int bx, int by);
void dz_extended_hexagon(int rebuild);
void dz_extended_triangles(int rebuild);
int dz_extended_native(unsigned effect);
/* A growing walk the screen guard cut short carries on, on the sides only.
 * Each original call moves it one step; the middle never sees it. */
struct dz_extended_circuit {
    unsigned effect; int x, y, step, length, direction, limit, shift, color, phase;
    const int16_t *dx, *dy;
};
void dz_extended_circuit_begin(const struct dz_extended_circuit *circuit);
void dz_extended_circuit_step(void);
#endif
