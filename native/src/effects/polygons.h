#ifndef DAZZLE_NATIVE_POLYGONS_H
#define DAZZLE_NATIVE_POLYGONS_H
#include <stdint.h>

int dz_native_polygon_begin(unsigned effect, int vertex_count,
                            const int16_t *vertex_x, const int16_t *vertex_y,
                            uint16_t fill_color, uint16_t edge_color, uint16_t flags);
void dz_native_polygon_end(int active);

#endif
