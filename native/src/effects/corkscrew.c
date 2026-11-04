/* A growing diagonal axis with quadratic sinusoidal transverse displacement. */
#include "corkscrew.h"
#include "effects.h"
#include "geometry.h"
#include "dazzle.h"
#include <math.h>
static double phase0, phase1, distance0, distance1;
static int valid;
static uint16_t page;
static void point(double t, int swap, double *x, double *y)
{
    double phase = phase0+(phase1-phase0)*t, d = distance0+(distance1-distance0)*t;
    double along = d*(1-cos(phase)/4), across = d*d/screen_extent*sin(phase);
    *x = ((swap ? corkscrew_origin_y : corkscrew_origin_x)+along)*dz_native_gain()+(swap ? across : -across);
    *y = (swap ? corkscrew_origin_x : corkscrew_origin_y)+along+(swap ? -across : across);
}
static void draw(unsigned symmetry, unsigned char color)
{
    double d = fmax(distance0,distance1), dp = fabs(phase1-phase0), dd = fabs(distance1-distance0);
    double error = dz_native_scale()*dz_native_gain()*
        ((d/4+d*d/screen_extent)*dp*dp+(0.5+4*d/screen_extent)*dd*dp+2*dd*dd/screen_extent)/8;
    int n = (int)ceil(sqrt(error/0.125));
    if (n < 1) n = 1;
    for (int swap = 0; swap < ((symmetry & 3u) == 3u ? 2 : 1); ++swap) {
        double ax, ay; point(0,swap,&ax,&ay);
        for (int i = 1; i <= n; ++i) {
            double bx, by; point((double)i/n,swap,&bx,&by);
            dz_native_line(ax,ay,bx,by,symmetry & 1u,color,0); ax = bx; ay = by;
        }
    }
}
void dz_native_corkscrew_prepare(int rebuild)
{
    if (!dz_native_enabled(DZ_EFFECT_CORKSCREW)) { valid = 0; return; }
    phase0 = phase1; distance0 = distance1;
    phase1 = corkscrew_phase; distance1 = corkscrew_distance;
    if (!valid || rebuild || page != page_paragraph) { phase0 = phase1; distance0 = distance1; }
    valid = 1; page = page_paragraph; dz_native_queue(draw);
}
