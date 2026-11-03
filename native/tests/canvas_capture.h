/* Test-only CPU scanout oracle. Production never gathers visible rows. */
#include "platform/canvas.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static struct dz_canvas_view canvas_capture(void)
{
    static uint8_t *copy;
    static size_t bytes;
    struct dz_canvas_view v = dz_canvas_surface();
    int w, h;
    dz_canvas_native_size(&w,&h);
    size_t n = (size_t)w*h, needed = n*(v.coverage ? 3u : 1u);
    if (needed > bytes) { free(copy); copy = malloc(needed); assert(copy); bytes = needed; }
    unsigned capacity = DZ_VRAM_PLANE_BYTES*(unsigned)dz_video.pixels_per_byte;
    int ring = (int)lround((double)capacity/dz_video.width*h/dz_video.height);
    if (ring < h) ring = h;
    int start = (int)(llround((double)dz_video.display_start_row*h/dz_video.height) % ring);
    int split = dz_video.line_compare >= (unsigned)dz_video.height ? h :
        (int)llround((double)(dz_video.line_compare+1)*h/dz_video.height);
    for (int y = 0; y < h; ++y) {
        int sy = (y >= split ? y-split : y+start)%ring;
        memcpy(copy+(size_t)y*w,v.indices+(size_t)sy*w,(size_t)w);
        if (v.coverage) {
            memcpy(copy+n+(size_t)y*w,v.coverage+(size_t)sy*w,(size_t)w);
            memcpy(copy+2*n+(size_t)y*w,v.under+(size_t)sy*w,(size_t)w);
        }
    }
    v.indices = copy; v.height = h;
    if (v.coverage) { v.coverage = copy+n; v.under = copy+2*n; }
    return v;
}
static inline void canvas_assert_pixels(SDL_Surface *pixels)
{
    struct dz_canvas_view v = canvas_capture();
    SDL_FRect clip = dz_canvas_content();
    uint32_t palette[256];
    for (unsigned i = 0; i < 256; ++i) {
        unsigned r,g,b;
        if (dz_video.mode == DZ_MODE_EGA_350) {
            unsigned c = dz_video.ega_attr[i&15u];
            r = ((c>>2)&1u)*170u+((c>>5)&1u)*85u;
            g = ((c>>1)&1u)*170u+((c>>4)&1u)*85u;
            b = (c&1u)*170u+((c>>3)&1u)*85u;
        } else {
            r = (dz_video.dac[i][0]&63u)*255u/63u;
            g = (dz_video.dac[i][1]&63u)*255u/63u;
            b = (dz_video.dac[i][2]&63u)*255u/63u;
        }
        palette[i] = UINT32_C(0xff000000) | r<<16 | g<<8 | b;
    }
    SDL_Surface *argb = SDL_ConvertSurface(pixels,SDL_PIXELFORMAT_ARGB8888);
    assert(argb && argb->w == v.width && argb->h == v.height && !v.coverage);
    for (int y = 0; y < v.height; ++y) for (int x = 0; x < v.width; ++x) {
        uint32_t expected = palette[v.indices[(size_t)y*v.width+x]];
        if (!dz_canvas_has_native() && (x < clip.x || x >= clip.x+clip.w || y < clip.y || y >= clip.y+clip.h))
            expected = UINT32_C(0xff000000);
        uint32_t actual = ((uint32_t *)((uint8_t *)argb->pixels+(size_t)y*argb->pitch))[x];
        assert(actual == expected);
    }
    SDL_DestroySurface(argb);
}
