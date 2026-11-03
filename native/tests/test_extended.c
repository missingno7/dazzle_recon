#include "platform/extended.h"
#include "platform/hybrid.h"
#include "dazzle.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void layout(void)
{
    static const int sizes[][2] = {{1920,1080},{1920,1200},{2520,1080},{1440,1080},{1080,1920},{3440,1080}};
    static const float margins[] = {240,160,540,0,0,540};
    for (unsigned i = 0; i < 6; ++i) for (int w = 320; w <= 640; w *= 2) {
        struct dz_extended_layout l = dz_extended_layout(sizes[i][0], sizes[i][1], w);
        SDL_FRect center = dz_present_viewport(sizes[i][0], sizes[i][1], DZ_PRESENT_ORIGINAL);
        SDL_FRect extended = dz_present_viewport(sizes[i][0], sizes[i][1], DZ_PRESENT_EXTENDED);
        assert(!memcmp(&l.center, &center, sizeof center) && !memcmp(&center, &extended, sizeof center));
        assert(fabsf(l.left.w - margins[i]) < 0.001f && l.right.w == l.left.w);
        assert(fabsf(l.columns * center.w / w - margins[i]) < 0.001f);
        assert(l.storage_columns >= l.columns && l.storage_columns < l.columns + 1);
        if (l.columns) {
            assert(l.left.x + l.left.w == center.x && l.right.x == center.x + center.w);
            assert(fabsf(l.left.w / l.columns - center.w / w) < 0.001f);
        }
    }
    assert(dz_extended_layout(0, 0, 320).columns == 0);
}
static uint32_t pixel(SDL_Surface *s, int x, int y)
{
    return ((uint32_t *)((uint8_t *)s->pixels + y * s->pitch))[x];
}
static SDL_Surface *capture(SDL_Renderer *r, struct dz_presenter *p, enum dz_presentation policy)
{
    assert(dz_presenter_draw(p, policy, 1) == 1);
    assert(dz_presenter_draw(p, policy, 0) == 0);
    SDL_Surface *raw = SDL_RenderReadPixels(r, NULL);
    assert(raw);
    SDL_Surface *s = SDL_ConvertSurface(raw, SDL_PIXELFORMAT_ARGB8888);
    SDL_DestroySurface(raw); assert(s); return s;
}
static void palette(void)
{
    /* Every index is nonblack: a black seam or uncovered edge cannot hide in this fixture. */
    for (unsigned i = 0; i < 256; ++i) {
        dz_video.dac[i][0] = (uint8_t)(i % 63 + 1);
        dz_video.dac[i][1] = (uint8_t)(i / 4);
        dz_video.dac[i][2] = (uint8_t)((i * 17) % 64);
    }
    for (unsigned i = 0; i < 16; ++i) dz_video.ega_attr[i] = (uint8_t)(i + 1);
}
static uint32_t sides_hash(void)
{
    uint32_t h = 2166136261u;
    unsigned rows = (DZ_VRAM_PLANE_BYTES * (unsigned)dz_video.pixels_per_byte + (unsigned)dz_video.width - 1) / (unsigned)dz_video.width;
    for (unsigned s = 0; s < 2; ++s) for (unsigned y = 0; y < rows; ++y)
        for (unsigned x = 0; x < (unsigned)dz_video.width * 3 / 8; ++x)
            h = (h ^ dz_extended_index(s, y, x)) * 16777619u;
    return h;
}
static void emit(unsigned image, unsigned count, unsigned page)
{
    for (unsigned i = 0; i < count; ++i) {
        if (image == DZ_EXT_DOTS) dz_extended_dots(1, i % 253 + 3, 1, page);
        else dz_extended_sine(i % 253 + 3, 1, page);
    }
}
static void center_identity(SDL_Renderer *r, struct dz_presenter *p, int w, int h, unsigned image)
{
    SDL_Texture *target = SDL_CreateTexture(r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, w, h);
    assert(target && SDL_SetRenderTarget(r, target));
    assert(dz_presenter_draw(p, DZ_PRESENT_EXTENDED, 1) == 1); /* establish output before sampling */
    dz_extended_image(image, dz_video.width / 2 + 1, dz_video.height / 2, 0, 0);
    emit(image, 3000, 0);
    SDL_Surface *original = capture(r, p, DZ_PRESENT_ORIGINAL);
    SDL_Surface *extended = capture(r, p, DZ_PRESENT_EXTENDED);
    SDL_FRect c = dz_present_viewport(w, h, DZ_PRESENT_ORIGINAL);
    struct dz_extended_layout l = dz_extended_layout(w, h, dz_video.width);
    int active = dz_extended_active();
    unsigned side_pixels = 0;
    /* SDL's nearest rectangle coverage rounds the outside edges to output pixels. */
    int x0 = (int)ceilf(c.x), x1 = (int)floorf(c.x + c.w);
    int y0 = (int)roundf(c.y), y1 = (int)roundf(c.y + c.h);
    int outer0 = l.columns ? (int)roundf(l.left.x) : x0;
    int outer1 = l.columns ? (int)roundf(l.right.x + l.right.w) : x1;
    for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) {
        if (!active || pixel(original,x,y) != UINT32_C(0xff000000) || (x >= x0 && x < x1)) {
            if (pixel(original,x,y) != pixel(extended,x,y))
                fprintf(stderr,"center differs mode=%d path=%d size=%dx%d at=%d,%d bounds=%g,%g,%g,%g values=%08x,%08x\n",
                    dz_video.mode,dz_presenter_path(p),w,h,x,y,c.x,c.y,c.w,c.h,pixel(original,x,y),pixel(extended,x,y));
            assert(pixel(original, x, y) == pixel(extended, x, y));
        }
        else if (y >= y0 && y < y1 && x >= outer0 && x < outer1) {
            if (pixel(extended,x,y) == UINT32_C(0xff000000)) fprintf(stderr,"gap mode=%d path=%d size=%dx%d at=%d,%d center=%g,%g,%g,%g\n",dz_video.mode,dz_presenter_path(p),w,h,x,y,c.x,c.y,c.w,c.h);
            assert(pixel(extended, x, y) != UINT32_C(0xff000000));
            ++side_pixels;
        } else assert(pixel(extended, x, y) == UINT32_C(0xff000000));
    }
    if (active && l.columns > 1) assert(side_pixels > 0);
    SDL_DestroySurface(original); SDL_DestroySurface(extended);
    assert(SDL_SetRenderTarget(r, NULL)); SDL_DestroyTexture(target);
}
static void lifecycle(SDL_Renderer *r, struct dz_presenter *p)
{
    dz_video_set_mode(DZ_MODE_VGA_400, 1); palette();
    SDL_Texture *target = SDL_CreateTexture(r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, 960, 540);
    assert(target && SDL_SetRenderTarget(r, target));
    assert(dz_presenter_draw(p, DZ_PRESENT_EXTENDED, 1) == 1);
    dz_extended_image(DZ_EXT_DOTS, 201, 200, 0, 0);
    struct dz_video *canonical = malloc(sizeof *canonical); assert(canonical); *canonical = dz_video;
    uint32_t rng = dz_random_state(); uint64_t calls = dz_rand_calls(), time = dz_virtual_ns();
    emit(DZ_EXT_DOTS, 12000, 0);
    assert(!memcmp(canonical, &dz_video, sizeof dz_video));
    assert(rng == dz_random_state() && calls == dz_rand_calls() && time == dz_virtual_ns());
    free(canonical);
    uint32_t first = sides_hash();
    SDL_Surface *s = capture(r, p, DZ_PRESENT_EXTENDED); SDL_DestroySurface(s);
    for (int i = 0; i < 3; ++i) assert(dz_presenter_draw(p, DZ_PRESENT_EXTENDED, 1) == 1);
    assert(first == sides_hash()); /* exposes and palette changes cannot advance emitters */
    dz_extended_fade('j'); assert(dz_extended_active() && first == sides_hash());
    memset(dz_video.dac, 0, sizeof dz_video.dac);
    s = capture(r, p, DZ_PRESENT_EXTENDED);
    for (int y = 0; y < s->h; ++y) for (int x = 0; x < s->w; ++x) assert(pixel(s,x,y) == UINT32_C(0xff000000));
    SDL_DestroySurface(s); palette();
    dz_extended_clear(0);
    uint32_t cleared = sides_hash(); assert(cleared != first);
    for (unsigned y = 0; y < 820; ++y) for (unsigned d = 0; d < 120; ++d)
        assert(dz_extended_index(0,y,d) == 0 && dz_extended_index(1,y,d) == 0);
    dz_extended_clear(0x0201);
    for (unsigned d=0;d<120;++d) {
        assert(dz_extended_index(0,0,d) == ((d/4)&1 ? 1 : 2));
        assert(dz_extended_index(1,0,d) == ((d/4)&1 ? 2 : 1));
        assert(dz_extended_index(0,800,d) == 0 && dz_extended_index(1,800,d) == 0);
    }
    dz_extended_clear(0);
    emit(DZ_EXT_DOTS, 1000, 0);
    /* Supported overlay resets private generators but retains both pages' ink. */
    first = sides_hash(); dz_extended_image(DZ_EXT_SINE,201,200,0,0); assert(first == sides_hash());
    for (const char *key = "abcdefgiklmopqrstuvwxyz"; *key; ++key) {
        dz_extended_fade(*key); assert(!dz_extended_active());
        emit(DZ_EXT_SINE,10,0); assert(!dz_extended_active());
        dz_extended_clear(0); assert(!dz_extended_active());
        dz_extended_image(DZ_EXT_SINE,201,200,0,0); assert(dz_extended_active());
    }
    dz_extended_image(9,201,200,0,0); assert(!dz_extended_active());
    dz_extended_image(DZ_EXT_SINE,201,200,0,0); emit(DZ_EXT_SINE,1000,0);
    assert(SDL_SetRenderTarget(r,NULL)); SDL_DestroyTexture(target);
    target = SDL_CreateTexture(r,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_TARGET,1000,540);
    assert(target && SDL_SetRenderTarget(r,target));
    assert(dz_presenter_draw(p,DZ_PRESENT_EXTENDED,1) == 1);
    assert(sides_hash() == cleared && dz_extended_active()); /* resize resets side ink/state */
    dz_video_set_mode(DZ_MODE_EGA_350,1);
    assert(!dz_extended_active());
    dz_extended_image(DZ_EXT_SINE,351,350,1,0); emit(DZ_EXT_SINE,1000,0);
    set_ega_raster_xor(1); assert(!dz_extended_active()); set_ega_raster_xor(0);
    assert(!dz_extended_active()); /* no mid-image revival */
    dz_extended_image(DZ_EXT_SINE,351,350,1,0); assert(dz_extended_active());
    dz_display.quality = DZ_QUALITY_HYBRID; assert(!dz_extended_active());
    SDL_Surface *ha = capture(r,p,DZ_PRESENT_ORIGINAL), *hb = capture(r,p,DZ_PRESENT_EXTENDED);
    for (int y=0;y<ha->h;++y) for(int x=0;x<ha->w;++x) assert(pixel(ha,x,y)==pixel(hb,x,y));
    SDL_DestroySurface(ha); SDL_DestroySurface(hb); dz_hybrid_shutdown();
    dz_display.quality = DZ_QUALITY_CLASSIC;
    dz_video_set_mode(DZ_MODE_TEXT,0);
    SDL_Surface *a = capture(r,p,DZ_PRESENT_ORIGINAL), *b = capture(r,p,DZ_PRESENT_EXTENDED);
    for (int y=0;y<a->h;++y) for(int x=0;x<a->w;++x) assert(pixel(a,x,y)==pixel(b,x,y));
    SDL_DestroySurface(a); SDL_DestroySurface(b);
    assert(SDL_SetRenderTarget(r,NULL)); SDL_DestroyTexture(target);
}
static void deterministic_sides(void)
{
    dz_video_set_mode(DZ_MODE_VGA_400,1);
    uint32_t hashes[2];
    for(unsigned pass=0;pass<2;++pass) {
        dz_extended_shutdown();
        dz_extended_image(DZ_EXT_DOTS,201,200,0,0);
        emit(DZ_EXT_DOTS,5000,0);
        hashes[pass]=sides_hash();
        /* A new selection of the same image has a different private seed. */
        dz_extended_image(DZ_EXT_DOTS,201,200,0,0); dz_extended_clear(0);
        emit(DZ_EXT_DOTS,5000,0);
        assert(sides_hash()!=hashes[pass]);
    }
    assert(hashes[0]==hashes[1]);
}
static void sine_bounds(SDL_Renderer *r, struct dz_presenter *p)
{
    static const int sizes[][2]={{1920,1080},{2560,1080},{1920,1200}};
    for (unsigned size=0;size<3;++size) {
        int w=sizes[size][0], h=sizes[size][1];
        SDL_Texture *target=SDL_CreateTexture(r,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_TARGET,w,h);
        assert(target && SDL_SetRenderTarget(r,target));
        for (unsigned backend=0;backend<4;++backend) {
            enum dz_video_mode mode=backend==0 ? DZ_MODE_VGA_400 : backend==1 ? DZ_MODE_VGA_200 : DZ_MODE_EGA_350;
            dz_video_set_mode(mode,1); palette();
            int hy=backend==3, hh=dz_video.height*(hy ? 2 : 1)/2;
            int span=backend==0 ? 201 : backend==1 ? 161 : hy ? 351 : 321;
            assert(dz_presenter_draw(p,DZ_PRESENT_EXTENDED,1)==1);
            unsigned width=(unsigned)floor(dz_extended_layout(w,h,dz_video.width).columns);
            for (unsigned seed=1;seed<=5;++seed) for (unsigned symmetry=1;symmetry<=3;symmetry+=2) {
                dz_options.seed=seed*12345;
                dz_extended_image(DZ_EXT_SINE,span,hh,hy,0); dz_extended_clear(0);
                if (size==2) {
                    assert(!dz_extended_active());
                    SDL_Surface *a=capture(r,p,DZ_PRESENT_ORIGINAL), *b=capture(r,p,DZ_PRESENT_EXTENDED);
                    for(int y=0;y<h;++y) for(int x=0;x<w;++x) assert(pixel(a,x,y)==pixel(b,x,y));
                    SDL_DestroySurface(a); SDL_DestroySurface(b);
                    continue;
                }
                assert(dz_extended_active());
                struct dz_video *canonical=malloc(sizeof *canonical); assert(canonical); *canonical=dz_video;
                uint32_t rng=dz_random_state(); uint64_t calls=dz_rand_calls(), time=dz_virtual_ns();
                for(unsigned n=0;n<20000;++n) dz_extended_sine(3+n%13,symmetry,0);
                assert(!memcmp(canonical,&dz_video,sizeof dz_video)); free(canonical);
                assert(rng==dz_random_state() && calls==dz_rand_calls() && time==dz_virtual_ns());
                for(unsigned side=0;side<2;++side) {
                    unsigned count=0, minx=width, maxx=0, miny=(unsigned)dz_video.height, maxy=0;
                    unsigned margin=(unsigned)dz_video.width/80;
                    for(unsigned y=0;y<(unsigned)dz_video.height;++y)
                        for(unsigned x=0;x<(unsigned)dz_video.width*3/8;++x) if(dz_extended_index(side,y,x)) {
                            assert(x>=margin && x<width-margin);
                            assert(y>=margin && y<(unsigned)dz_video.height-margin);
                            if(x<minx) minx=x;
                            if(x>maxx) maxx=x;
                            if(y<miny) miny=y;
                            if(y>maxy) maxy=y;
                            ++count;
                        }
                    assert(count>40 && minx+maxx==2*((width-1)/2));
                    assert(miny+maxy==(unsigned)(hy ? hh : hh*2));
                }
                if(seed==1) {
                    uint32_t first=sides_hash();
                    dz_extended_clear(0);
                    for(unsigned n=0;n<20000;++n) {
                        if(n==10000) assert(dz_presenter_draw(p,DZ_PRESENT_EXTENDED,1)==1);
                        dz_extended_sine(3+n%13,symmetry,0);
                    }
                    assert(first==sides_hash()); /* reset/replay and an extra expose are deterministic */
                }
            }
        }
        assert(SDL_SetRenderTarget(r,NULL)); SDL_DestroyTexture(target);
    }
    dz_options.seed=123;
    /* A narrow resize retires this image; widening cannot restore a partial motif. */
    dz_video_set_mode(DZ_MODE_VGA_400,1);
    for(unsigned step=0;step<3;++step) {
        int h=step==1 ? 1200 : 1080;
        SDL_Texture *target=SDL_CreateTexture(r,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_TARGET,1920,h);
        assert(target && SDL_SetRenderTarget(r,target));
        assert(dz_presenter_draw(p,DZ_PRESENT_EXTENDED,1)==1);
        if(!step) dz_extended_image(DZ_EXT_SINE,201,200,0,0);
        assert(dz_extended_active()==(step==0));
        if(step==2) { dz_extended_image(DZ_EXT_SINE,201,200,0,0); assert(dz_extended_active()); }
        assert(SDL_SetRenderTarget(r,NULL)); SDL_DestroyTexture(target);
    }
}
static void pages(SDL_Renderer *r, struct dz_presenter *p)
{
    dz_video_set_mode(DZ_MODE_VGA_400,1); palette();
    SDL_Texture *target = SDL_CreateTexture(r,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_TARGET,960,540);
    assert(target && SDL_SetRenderTarget(r,target));
    assert(dz_presenter_draw(p,DZ_PRESENT_EXTENDED,1) == 1);
    dz_extended_image(DZ_EXT_DOTS,201,200,0,0);
    emit(DZ_EXT_DOTS,10000,0x1f40); /* page 400; first page remains index zero */
    for(unsigned y=0;y<400;++y) for(unsigned d=0;d<120;++d) assert(dz_extended_index(1,y,d)==0);
    dz_video.display_start_row=400; dz_video.line_compare=199;
    SDL_Surface *s=capture(r,p,DZ_PRESENT_EXTENDED);
    SDL_FRect c=dz_present_viewport(960,540,DZ_PRESENT_ORIGINAL);
    for(int y=0;y<540;++y) for(int x=(int)(c.x+c.w);x<960;++x) {
        /* Software/GPU nearest conventions differ at fractional source edges.
         * Compare samples whose entire half-pixel interval lies in one cell. */
        if ((int)(y*400/540.0) != (int)((y+0.5)*400/540.0) ||
            (int)((x-c.x-c.w)*320/c.w) != (int)((x+0.5-c.x-c.w)*320/c.w)) continue;
        unsigned sy=(unsigned)(y*400/540), d=(unsigned)((x-c.x-c.w)*320/c.w);
        unsigned row=sy>199 ? sy-200 : 400+sy;
        uint8_t index=dz_extended_index(1,row,d);
        uint32_t expected=UINT32_C(0xff000000) | (uint32_t)(dz_video.dac[index][0]*255/63)<<16 |
            (uint32_t)(dz_video.dac[index][1]*255/63)<<8 | (uint32_t)(dz_video.dac[index][2]*255/63);
        assert(pixel(s,x,y)==expected);
    }
    SDL_DestroySurface(s);
    /* Aperture wrap is per address, including the partial final row. */
    for(unsigned d=64;d<120;++d) assert(dz_extended_index(1,819,d)==dz_extended_index(1,0,d-64));
    assert(SDL_SetRenderTarget(r,NULL)); SDL_DestroyTexture(target);
}
int main(int argc, char **argv)
{
    layout(); dz_options.deterministic=1; dz_options.seed=123;
    dz_display.presentation=DZ_PRESENT_EXTENDED;
    assert(SDL_Init(SDL_INIT_VIDEO));
    const char *backend=argc==3 && !strcmp(argv[1],"--backend") ? argv[2] : "software";
    SDL_Surface *surface=NULL;
    SDL_Window *window=NULL;
    SDL_Renderer *r;
    if (!strcmp(backend,"software")) {
        surface=SDL_CreateSurface(16,16,SDL_PIXELFORMAT_ARGB8888);
        r=SDL_CreateSoftwareRenderer(surface);
    } else {
        window=SDL_CreateWindow("Extended verification",64,64,SDL_WINDOW_HIDDEN);
        r=!window ? NULL : !strcmp(backend,"gpu") ? SDL_CreateGPURenderer(NULL,window) :
            SDL_CreateRenderer(window,!strcmp(backend,"default") ? NULL : backend);
    }
    if (!r) { fprintf(stderr,"UNAVAILABLE %s: %s\n",backend,SDL_GetError()); SDL_DestroyWindow(window); SDL_Quit(); return 77; }
    printf("backend=%s\n",SDL_GetRendererName(r)); fflush(stdout);
    static const int sizes[][2]={{1920,1080},{1920,1200},{2560,1080},{640,480},{480,640},{701,479},{641,301},{3440,1080}};
    for(int path=DZ_RENDERER_INDEXED;path<=DZ_RENDERER_RGBA;++path) {
        struct dz_presenter *p=dz_presenter_create(r,(enum dz_renderer)path); assert(p);
        for(int m=DZ_MODE_VGA_400;m<=DZ_MODE_EGA_350;++m) {
            dz_video_set_mode((enum dz_video_mode)m,1); palette();
            for(unsigned a=0;a<sizeof dz_video.vram;++a) dz_video.vram[a]=(uint8_t)(a*17+a/319);
            for(unsigned i=0;i<sizeof sizes/sizeof *sizes;++i)
                center_identity(r,p,sizes[i][0],sizes[i][1],i&1 ? DZ_EXT_SINE : DZ_EXT_DOTS);
        }
        lifecycle(r,p); pages(r,p); sine_bounds(r,p); dz_presenter_destroy(p);
    }
    deterministic_sides();
    dz_extended_shutdown(); SDL_DestroyRenderer(r); SDL_DestroySurface(surface); SDL_DestroyWindow(window); SDL_Quit();
    puts("Extended: layout, center identity, indexed/RGBA, palette, lifecycle and page scanout passed");
    return 0;
}
