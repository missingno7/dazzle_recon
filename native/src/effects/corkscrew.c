/* The original truncates the two along/across combinations before adding
 * its origin. Connect those produced positions, without a different swap. */
#include "corkscrew.h"
#include "effects.h"
#include "geometry.h"
#include "dazzle.h"
static struct dz_native_path path;
static void draw(unsigned symmetry, unsigned char color)
{ dz_native_path_draw(&path,symmetry,color); }
void dz_native_corkscrew_prepare(int rebuild)
{
    if (!dz_native_enabled(DZ_EFFECT_CORKSCREW)) { path.valid = 0; return; }
    dz_native_path_prepare(&path,draw_x,draw_y,rebuild);
    dz_native_queue(draw);
}
