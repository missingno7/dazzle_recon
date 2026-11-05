/* Follow the real along/across combinations the original truncates before
 * adding its integer origin. */
#include "corkscrew.h"
#include "effects.h"
#include "geometry.h"
#include "dazzle.h"
static struct dz_native_path path;
static void draw(unsigned symmetry, unsigned char color)
{ dz_native_path_draw(&path,symmetry,color); }
void dz_native_corkscrew_prepare(int rebuild, double x, double y)
{
    if (!dz_native_enabled(DZ_EFFECT_CORKSCREW)) { path.valid = 0; return; }
    dz_native_path_prepare(&path,corkscrew_origin_x+dz_native_untruncated(x),
                           corkscrew_origin_y+dz_native_untruncated(y),rebuild);
    dz_native_queue(draw);
}
