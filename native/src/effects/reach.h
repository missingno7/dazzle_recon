#ifndef DAZZLE_EXTENDED_REACH_H
#define DAZZLE_EXTENDED_REACH_H
/* Presentation-only suffixes of existing runs. No logical stores or services. */
void dz_extended_diagonal(int rising);
void dz_extended_field(int formula);
void dz_extended_zigstream(int rebuild, int variant);
void dz_extended_segment(unsigned effect, int ax, int ay, int bx, int by);
void dz_extended_hexagon(int rebuild);
void dz_extended_triangles(int rebuild);
int dz_extended_native(unsigned effect);
#endif
