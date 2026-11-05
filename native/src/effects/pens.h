#ifndef DAZZLE_NATIVE_PENS_H
#define DAZZLE_NATIVE_PENS_H
/* Sampled fields, round dots, row spans, and moving pens. */
void dz_native_samples(unsigned effect);
int dz_native_ball_field(double x, double y);
void dz_native_rows(unsigned effect, int left, int right, double dl, double dr);
void dz_native_lines(int reset);
void dz_native_field_cell(int formula, int x, int y, unsigned symmetry, unsigned char color);
void dz_native_pen(unsigned effect, double x, double y, int dx, int dy,
                   int step, int count);
void dz_native_curve_pen(unsigned effect, int reset, double x, double y,
                         int dx, int dy, int step, int count);
void dz_native_diagonal(unsigned effect, int rising);
#endif
