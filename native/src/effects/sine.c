/* Continuous interpretation of the original independently modulated oscillator.
 * Original parameters/state remain in DZLE; this module has no RNG or clock. */
#include "sine.h"
#include "effects.h"
#include "dazzle.h"
#include "platform/canvas.h"
#include <math.h>

static struct { double ax, ay, phase; } previous, current;
static int valid, pending;
static uint16_t page;
void dz_native_sine_prepare(int rebuild, double ax, double ay, double phase)
{
    if (!dz_native_enabled(DZ_EFFECT_SINE)) {
        valid = pending = 0; return;
    }
    if (rebuild || page != page_paragraph) valid = 0;
    page = page_paragraph;
    previous = current;
    current.ax = ax; current.ay = ay; current.phase = phase;
    if (!valid) previous = current;
    valid = pending = 1;
}
int dz_native_sine_pending(void) { return pending; }
static void point(double t, int swap, int sx, int sy, double *x, double *y)
{
    double a = previous.ax + (current.ax-previous.ax)*t;
    double b = previous.ay + (current.ay-previous.ay)*t;
    double phase = previous.phase + (current.phase-previous.phase)*t;
    double px = a*sin(phase), py = b*cos(phase);
    *x = sx*(swap ? py : px); *y = sy*(swap ? px : py);
}
void dz_native_sine_draw(uint16_t symmetry, uint8_t color)
{
    pending = 0;
    int w, h;
    dz_canvas_native_size(&w, &h);
    if (!w || !h || screen_half_height <= 0) return;
    /* Preserve the original mode's physical pixel aspect, including EGA700's
     * software Y coordinates. Width-derived geometry spreads with output width;
     * vertical size stays at the fitted Original height. One centred figure. */
    double fitted_h = fmin(h,w*0.75);
    double scale_x = (double)w/(2*screen_half_width);
    double scale_y = fitted_h/(2*screen_half_height);
    double width = fmax(1.0,sqrt(fitted_h*fitted_h*4/3/(dz_video.width*dz_video.height)));
    int branches = (symmetry & 1u) ? 4 : 1;
    int swaps = (symmetry & 3u) == 3u ? 2 : 1;
    /* Linear-amplitude parametric interpolation: |f''|/8 bounds chord error.
     * Subdivide until that bound is <= 1/8 physical pixel, independent of length. */
    double dp = fabs(current.phase-previous.phase);
    double amplitudes = fmax(fabs(current.ax),fabs(previous.ax))+fmax(fabs(current.ay),fabs(previous.ay));
    double changes = fabs(current.ax-previous.ax)+fabs(current.ay-previous.ay);
    double error = fmax(scale_x,scale_y)*(amplitudes*dp*dp+2*changes*dp)/8;
    int steps = (int)ceil(sqrt(error/0.125));
    if (steps < 1) steps = 1;
    if (steps > 4096) steps = 4096;
    {
        double cx = w*0.5, cy = h*0.5;
        for (int swap = 0; swap < swaps; ++swap) for (int branch = 0; branch < branches; ++branch) {
            int sx = branch >= 2 ? -1 : 1, sy = branch & 1 ? -1 : 1;
            double ax, ay;
            point(0, swap, sx, sy, &ax, &ay);
            for (int i = 1; i <= steps; ++i) {
                double bx, by;
                point((double)i/steps, swap, sx, sy, &bx, &by);
                dz_canvas_stroke(cx+ax*scale_x, cy+ay*scale_y, cx+bx*scale_x, cy+by*scale_y, width, color);
                ax = bx; ay = by;
            }
        }
    }
}
