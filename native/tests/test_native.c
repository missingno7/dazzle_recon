#include "platform/canvas.h"
#include "effects/sine.h"
#include "effects/geometry.h"
#include "dazzle.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "canvas_capture.h"

static SDL_Renderer *renderer;
static void oscillator(int w, int h, enum dz_presentation policy, int aa)
{
    dz_display.native_effects = UINT64_MAX; dz_display.native_aa = aa;
    dz_video_set_mode(DZ_MODE_VGA_400, 1);
    assert(dz_canvas_configure(renderer, w, h, policy) == 1);
    screen_half_height = 200; screen_half_width = 160;
    dz_canvas_select_page(0);
    dz_canvas_clear(0);
    for (int i = 0; i <= 360; ++i) {
        double phase = i*6.283185307179586/360;
        dz_native_sine_prepare(i == 0, 30*sin(phase), 30*cos(phase));
        assert(dz_native_sine_pending()); dz_native_sine_draw(0, 17);
    }
    struct dz_canvas_view v = canvas_capture();
    assert(v.width == w && v.height == h);
    assert((v.coverage != NULL) == aa);
    /* The policy transforms geometry and its stroke footprint together. */
    SDL_FRect r = dz_present_viewport(w,h,policy);
    double scale_x = r.w/320, scale_y = r.h/400;
    int minx = w, miny = h, maxx = -1, maxy = -1, total = 0;
    for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) {
        if (v.indices[y*w+x] != 17 || (aa && v.coverage[y*w+x] < 128)) continue;
        if (x < minx) minx = x;
        if (x > maxx) maxx = x;
        if (y < miny) miny = y;
        if (y > maxy) maxy = y;
        ++total;
    }
    assert(total > 30);
    double expected = scale_x/scale_y, actual = (double)(maxx-minx)/(maxy-miny);
    assert(fabs(actual/expected-1) < 0.08);
    /* A single centred curve: every point has nearby ink, with empty corners. */
    for (int i = 0; i < 360; ++i) {
        int x = (int)(w*0.5+0.5*scale_x+dz_native_untruncated(30*sin(i*6.283185307179586/360))*scale_x);
        int y = (int)(h*0.5+0.5*scale_y+dz_native_untruncated(30*cos(i*6.283185307179586/360))*scale_y), found = 0;
        if (x < 0 || y < 0 || x >= w || y >= h) continue;
        for (int dy = -1; dy <= 1; ++dy) for (int dx = -1; dx <= 1; ++dx)
            if (x+dx >= 0 && x+dx < w && y+dy >= 0 && y+dy < h)
                found |= v.indices[(y+dy)*w+x+dx] == 17;
        assert(found);
    }
    assert(!v.indices[0] && !v.indices[w-1]);
    dz_canvas_stroke(w/2.0, 0, w/2.0, h, 12, 51);
    dz_canvas_write(200u*320+160, 0);
    assert(dz_canvas_read(200u*320+160) == 0);
    dz_canvas_clear(4); v = canvas_capture();
    for (int i = 0; i < w*h; ++i) {
        assert(v.indices[i] == 4);
        if (aa) assert(v.coverage[i] == 255);
    }
}
struct extent { int x0,y0,x1,y1; };
static struct extent ink_extent(void)
{
    struct dz_canvas_view v = canvas_capture();
    struct extent e = {v.width,v.height,-1,-1};
    for (int y = 0; y < v.height; ++y) for (int x = 0; x < v.width; ++x) if (v.indices[y*v.width+x]) {
        if (x < e.x0) e.x0 = x;
        if (x > e.x1) e.x1 = x;
        if (y < e.y0) e.y0 = y;
        if (y > e.y1) e.y1 = y;
    }
    assert(e.x1 > e.x0 && e.y1 > e.y0);
    return e;
}
static void figure_aspect(void)
{
    /* Actual original sine state evolution and kernels, including EGA parity.
     * Compare Native against a Faithful raster, not an assumed square circle. */
    const int sizes[][2] = {{640,480},{1440,1080},{1920,1080},{2560,1080},{3840,2160}};
    for (int mode = 0; mode < 4; ++mode) for (unsigned size = 0; size < sizeof sizes/sizeof sizes[0]; ++size) {
        struct extent faithful = {0}, native = {0};
        int w = sizes[size][0], h = sizes[size][1];
        for (int enabled = 0; enabled < 2; ++enabled) {
            video_adapter = mode < 2 ? 8 : 4;
            bios_resolution_mode = mode == 1 || mode == 2;
            configure_video_backend(0);
            dz_display.native_effects = enabled ? UINT64_MAX : 0; dz_display.native_aa = 0;
            dz_video_set_mode(mode < 2 ? (mode == 1 ? DZ_MODE_VGA_200 : DZ_MODE_VGA_400) : DZ_MODE_EGA_350,1);
            assert(dz_canvas_configure(renderer,w,h,DZ_PRESENT_ORIGINAL) == 1);
            dz_canvas_select_page(0); dz_canvas_clear(0);
            dz_srand(1); plot_color = 3; draw_delay_count = 1; draw_symmetry = 1;
            for (int i = 0; i < 5000; ++i) { sine(i == 0); plot_callback(draw_symmetry); }
            if (enabled) native = ink_extent(); else faithful = ink_extent();
        }
        double fw = faithful.x1-faithful.x0+1, fh = faithful.y1-faithful.y0+1;
        double nw = native.x1-native.x0+1, nh = native.y1-native.y0+1;
        /* Continuous coordinates can differ from truncated/even DOS coordinates
         * by two original units plus the different circular stroke footprint. */
        double tolerance_x = 3.0*w/(2*screen_half_width)+2;
        double tolerance_y = 3.0*h/(2*screen_half_height)+2;
        assert(fabs(nh-fh) <= tolerance_y);
        assert(fabs(nw-fw) <= tolerance_x);
        assert(fabs((nw/nh)/(fw/fh)-1) < 0.035);
        printf("sine aspect mode=%d %dx%d Faithful=%.0fx%.0f Native=%.0fx%.0f\n",mode,w,h,fw,fh,nw,nh);
    }
}
static void pages_and_resize(void)
{
    dz_display.native_aa = 1;
    dz_video_set_mode(DZ_MODE_VGA_400, 1);
    assert(dz_canvas_configure(renderer, 641, 479, DZ_PRESENT_ORIGINAL) == 1);
    dz_canvas_clear(0); dz_canvas_select_page(0);
    dz_canvas_stroke(0, 0, 641, 479, 5, 17);
    dz_canvas_select_page(2000);
    dz_canvas_stroke(0, 0, 641, 479, 5, 31);
    dz_canvas_start(400);
    struct dz_canvas_view v = canvas_capture();
    assert(v.indices[200*641+268] == 31);
    dz_canvas_start(819); /* rounded equivalent of the partial aperture wrap */
    v = canvas_capture();
    assert(v.indices[0] == 17);
    dz_canvas_start(200); dz_canvas_split(199);
    v = canvas_capture();
    /* Rounded 200/400*479 = 240; split restarts at output row 240. */
    assert(v.indices[240*641] == 17);
    dz_canvas_start(0); dz_canvas_split(400);
    dz_canvas_select_page(0);
    dz_canvas_clear_page(0, 16000, 0);
    v = canvas_capture();
    for (int i = 0; i < 641*479; ++i) assert(!v.indices[i]);
    dz_canvas_start(400); v = canvas_capture();
    assert(v.indices[200*641+268] == 31);
    dz_canvas_start(0);
    assert(dz_canvas_configure(renderer, 701, 501, DZ_PRESENT_STRETCH) == 1);
    assert(dz_canvas_take_restart() && !dz_canvas_take_restart());
    v = canvas_capture();
    for (int i = 0; i < 701*501; ++i) assert(!v.indices[i]);
}
static void stroke_reference(void)
{
    dz_display.native_aa = 0;
    dz_video_set_mode(DZ_MODE_VGA_400,1);
    assert(dz_canvas_configure(renderer,173,131,DZ_PRESENT_ORIGINAL) == 1);
    dz_canvas_select_page(0);
    const double lines[][5] = {{3.2,7.8,160.4,90.6,1},{80,4,80,126,7},
        {160.4,90.6,3.2,7.8,4.25},{20.1,80.8,20.1,80.8,9},{-20,-40,250,180,2.5}};
    for (unsigned n = 0; n < sizeof lines/sizeof lines[0]; ++n) {
        const double *l = lines[n];
        dz_canvas_clear(0); dz_canvas_stroke(l[0],l[1],l[2],l[3],l[4],17);
        struct dz_canvas_view v = canvas_capture();
        double dx = l[2]-l[0],dy = l[3]-l[1],length = dx*dx+dy*dy;
        for (int y = 0; y < 131; ++y) for (int x = 0; x < 173; ++x) {
            double t = length ? ((x+0.5-l[0])*dx+(y+0.5-l[1])*dy)/length : 0;
            t = fmax(0,fmin(1,t));
            double ex = x+0.5-l[0]-t*dx,ey = y+0.5-l[1]-t*dy;
            int expected = ex*ex+ey*ey <= l[4]*l[4]/4;
            assert((v.indices[y*173+x] == 17) == expected);
        }
    }
}
static void aa_present(void)
{
    SDL_Texture *target = SDL_CreateTexture(renderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_TARGET,641,479);
    assert(target && SDL_SetRenderTarget(renderer,target));
    struct dz_presenter *indexed = dz_presenter_create(renderer,DZ_RENDERER_INDEXED);
    struct dz_presenter *rgba = dz_presenter_create(renderer,DZ_RENDERER_RGBA);
    assert(indexed && rgba);
    dz_display.native_aa = 1;
    dz_video_set_mode(DZ_MODE_VGA_400,1);
    assert(dz_canvas_configure(renderer,641,479,DZ_PRESENT_ORIGINAL) == 1);
    dz_canvas_select_page(0); dz_canvas_clear(4);
    dz_canvas_stroke(10.2,20.8,630.6,450.3,3.75,17);
    for (int frame = 0; frame < 2; ++frame) {
        dz_video.dac[4][0] = frame ? 63 : 7; dz_video.dac[4][1] = 13;
        dz_video.dac[17][0] = frame ? 2 : 49; dz_video.dac[17][1] = 57;
        struct dz_canvas_view v = canvas_capture();
        uint64_t revision = v.revision;
        int partial = 0;
        for (int path = 0; path < 2; ++path) {
            struct dz_presenter *p = path ? rgba : indexed;
            assert(dz_presenter_draw(p,DZ_PRESENT_ORIGINAL,1) == 1);
            SDL_Surface *raw = SDL_RenderReadPixels(renderer,NULL);
            SDL_Surface *pixels = SDL_ConvertSurface(raw,SDL_PIXELFORMAT_ARGB8888);
            assert(pixels);
            for (int y = 0; y < 479; ++y) for (int x = 0; x < 641; ++x) {
                int i = y*641+x;
                unsigned a = v.coverage[i], b = 255-a;
                uint32_t expected = UINT32_C(0xff000000);
                for (int ch = 0; ch < 3; ++ch) {
                    unsigned c = (dz_video.dac[v.indices[i]][ch]&63u)*255/63;
                    unsigned u = (dz_video.dac[v.under[i]][ch]&63u)*255/63;
                    expected |= ((c*a+u*b+127)/255) << (16-8*ch);
                }
                SDL_FRect clip = dz_canvas_content();
                if (x < clip.x || x >= clip.x+clip.w || y < clip.y || y >= clip.y+clip.h)
                    expected = UINT32_C(0xff000000);
                uint32_t got = ((uint32_t *)((uint8_t *)pixels->pixels+(size_t)y*pixels->pitch))[x];
                assert(got == expected);
                partial += a > 0 && a < 255;
            }
            assert(partial && canvas_capture().revision == revision);
            assert(dz_presenter_draw(p,DZ_PRESENT_ORIGINAL,0) == 0);
            SDL_DestroySurface(raw); SDL_DestroySurface(pixels);
        }
    }
    dz_presenter_destroy(indexed); dz_presenter_destroy(rgba);
    SDL_SetRenderTarget(renderer,NULL); SDL_DestroyTexture(target);
}
int main(int argc, char **argv)
{
    dz_options.headless = dz_options.deterministic = 1;
    assert(dz_initialize() && SDL_InitSubSystem(SDL_INIT_VIDEO));
    SDL_Window *window = NULL;
    SDL_Surface *s = NULL;
    if (argc == 3 && !strcmp(argv[1],"--backend")) {
        window = SDL_CreateWindow("native test",64,64,SDL_WINDOW_HIDDEN); assert(window);
        renderer = !strcmp(argv[2],"gpu") ? SDL_CreateGPURenderer(NULL,window) : SDL_CreateRenderer(window,argv[2]);
    } else {
        s = SDL_CreateSurface(16,16,SDL_PIXELFORMAT_ARGB8888);
        renderer = SDL_CreateSoftwareRenderer(s);
    }
    assert(renderer);
    dz_display.canvas = DZ_CANVAS_SCREEN;
    const int sizes[][2] = {{480,320},{641,479},{1280,720},{720,1280},{1920,1080},{3840,2160}};
    for (unsigned i = 0; i < sizeof sizes/sizeof sizes[0]; ++i)
        for (int policy = 0; policy < 4; ++policy)
            for (int aa = 0; aa < 2; ++aa) oscillator(sizes[i][0],sizes[i][1],policy,aa);
    figure_aspect();
    pages_and_resize();
    stroke_reference();
    aa_present();
    SDL_DestroyRenderer(renderer); SDL_DestroySurface(s); SDL_DestroyWindow(window); dz_shutdown();
    puts("Native physical aspect, connected strokes, overwrite, pages, pan/split, clear and resize passed");
    return 0;
}
