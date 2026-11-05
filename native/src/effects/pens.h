#ifndef DAZZLE_NATIVE_PENS_H
#define DAZZLE_NATIVE_PENS_H
/* Raster fields, isolated cells, straight pens, and real-curve pen sweeps. */
void dz_native_cells(unsigned effect);
/* A real DVDT raster store, including EGA700 parity and double software Y. */
void dz_native_plot_cell(int x, int y, unsigned symmetry, unsigned char color);
void dz_native_pen(unsigned effect, double x, double y, int dx, int dy,
                   int step, int count);
void dz_native_curve_pen(unsigned effect, int reset, double x, double y,
                         int dx, int dy, int step, int count);
void dz_native_diagonal(unsigned effect, int rising);
#endif
