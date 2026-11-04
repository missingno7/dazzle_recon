/* Rolling pen: evaluate both real angles through each original table interval. */
#include "cycloids.h"
#include "effects.h"
#include "geometry.h"
#include "dazzle.h"
#include <math.h>
static int enabled, count, step;
static double orbit, spin, delta, pen, radius, aspect;
static void point(double t, double *x, double *y)
{
    double a = orbit+delta*t, b = a-spin-delta*cycloids_spin_step*t;
    *x = aspect*(radius*cos(a)+pen*cos(b));
    *y = radius*sin(a)+pen*sin(b);
}
static void draw(unsigned symmetry, unsigned char color)
{
    double a = dz_native_fraction(step-1,count), b = dz_native_fraction(step,count);
    /* Sum of |r|*angle_step^2 bounds second derivative over this interval. */
    double speed = fabs(1.0-cycloids_spin_step);
    double error = dz_native_scale()*fmax(1,aspect)*(fabs(radius)+fabs(pen)*speed*speed)*delta*delta*(b-a)*(b-a)/8;
    int n = (int)ceil(sqrt(error/0.125));
    if (n < 1) n = 1;
    double ax, ay; point(a,&ax,&ay);
    for (int i = 1; i <= n; ++i) {
        double bx, by; point(a+(b-a)*i/n,&bx,&by);
        dz_native_line(ax,ay,bx,by,symmetry,color,0); ax = bx; ay = by;
    }
    ++step;
}
void dz_native_cycloids_begin(int ax, int ay, int bx, int by)
{
    enabled = dz_native_enabled(DZ_EFFECT_CYCLOIDS);
    if (!enabled) return;
    count = dz_native_count(ax,ay,bx,by); step = 0;
    delta = 6.283185307179586/64;
    orbit = (cycloids_orbit_phase-1)*delta;
    spin = (cycloids_spin_phase-cycloids_spin_step)*delta;
    radius = cycloids_orbit_radius; pen = cycloids_pen_radius;
    aspect = (double)screen_half_width/screen_half_height*0.75;
}
void dz_native_cycloids_step(void) { if (enabled) dz_native_queue(draw); }
