/* Follow the oscillator's real values, which the original truncates to cells.
 * Samples are dense enough that chords need no subdivision. */
#include "sine.h"
#include "effects.h"
#include "geometry.h"
#include <stddef.h>
static struct dz_native_path path;
static int pending;
void dz_native_sine_prepare(int rebuild, double x, double y)
{
    if (!dz_native_enabled(DZ_EFFECT_SINE)) { path.valid = pending = 0; return; }
    dz_native_path_prepare(&path,dz_native_untruncated(x),dz_native_untruncated(y),rebuild); pending = 1;
    dz_native_queue(dz_native_sine_draw);
}
int dz_native_sine_pending(void) { return pending; }
void dz_native_sine_draw(unsigned symmetry, unsigned char color)
{ dz_native_plot = NULL; pending = 0; dz_native_path_draw(&path,symmetry,color); }
