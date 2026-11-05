/* Real fade code and DOS-order dependencies; independent physical end oracle. */
#define main original_fade_tests
#include "test_dzfd.c"
#undef main
#include "canvas_capture.h"
#include "effects/fades.h"

int main(void)
{
    dz_options=(struct dz_options){1,1,1,1.0,NULL};
    assert(dz_initialize() && SDL_InitSubSystem(SDL_INIT_VIDEO));
    SDL_Surface *s=SDL_CreateSurface(16,16,SDL_PIXELFORMAT_ARGB8888);
    SDL_Renderer *r=SDL_CreateSoftwareRenderer(s); assert(r);
    memcpy(original_bounds,countdown_segment_bounds,sizeof original_bounds);
    assert(dz_native_fades_select("BE,i,k,l,m,r,t,u,x,z"));
    assert(!dz_native_fades_select("?"));
    assert(dz_native_fades_select("all"));
    uint8_t *row_reference[2]={NULL,NULL};
    dz_display.canvas=DZ_CANVAS_SCREEN; dz_display.native_effects=0;
    assert(dz_native_any());
    assert(dz_native_fades_select("none") && !dz_native_any());
    assert(dz_native_fades_select("all"));
    for (int mode=0;mode<4;++mode) for (int policy=0;policy<=3;policy+=3) {
        dz_display.canvas=DZ_CANVAS_SCREEN;
        dz_display.presentation=(enum dz_presentation)policy;
        setup(mode>=2,mode==1 || mode==2,1);
        assert(dz_canvas_configure(r,3840,2160,(enum dz_presentation)policy)==1);
        dz_canvas_take_restart(); dz_canvas_clear(7);
        dz_canvas_fade_begin('e');
        if (mode>=2) {
            struct dz_canvas_view v=dz_canvas_surface(); size_t n=(size_t)v.width*v.height;
            uint8_t *before=malloc(n); assert(before); memcpy(before,v.indices,n);
            const double x[]={-400,400,230,-400}, y[]={-90,-60,100,-90};
            draw_x=draw_y=0; dz_canvas_raster_xor(1);
            for (int pass=0;pass<2;++pass) {
                dz_native_fade_polyline(x,y,4,2,3,5);
                dz_native_fade_shape(x,y,3,1,9);
            }
            assert(!memcmp(before,dz_canvas_surface().indices,n));
            free(before); dz_canvas_raster_xor(0);
        }
        dz_canvas_fade_end();
        if (mode>=2) {
            /* The same physical line in 350- and 700-software-row modes must
             * have exactly the same weight, joins and ends at every pixel. */
            dz_canvas_clear(0); dz_canvas_fade_begin('l');
            draw_x=20; draw_y=mode==3 ? 40 : 20; plot_color=7;
            int end_y=mode==3 ? 80 : 40,done=1;
            dz_native_fade_line(draw_x,draw_y,40,end_y);
            do { done=line_step(40,(int16_t)end_y,(int16_t)done); plot_callback(0); } while (!done);
            dz_canvas_fade_end();
            struct dz_canvas_view v=canvas_capture(); size_t n=(size_t)v.width*v.height;
            unsigned slot=policy==3;
            if (mode==2) { row_reference[slot]=malloc(n); assert(row_reference[slot]); memcpy(row_reference[slot],v.indices,n); }
            else { assert(!memcmp(row_reference[slot],v.indices,n)); free(row_reference[slot]); }
        }
        if (mode<2) {
            /* Ziti is an eraser with a deliberately untouched corner. Seed the
             * whole output and logical receipt, so no pre-existing zero can
             * conceal debris. Compare EVERY pixel after completion and after
             * an interrupted pass followed by the same fade again. */
            for (int interrupted=0;interrupted<2;++interrupted) {
                uint8_t *expected=NULL;
                for (int native=0;native<2;++native) {
                    dz_display.native_fades=native ? UINT32_MAX : 0;
                    dz_canvas_clear(7); set_ega_raster_xor(0);
                    struct dz_native_mapping m=dz_canvas_native_mapping(screen_half_width,screen_half_height);
                    dz_canvas_stroke_ellipse(m.cx-screen_half_width*m.unit_x,m.cy-screen_half_height*m.unit_y,
                        m.cx+screen_half_width*m.unit_x,m.cy+screen_half_height*m.unit_y,m.weight_x,m.weight_y,11);
                    current_graphics_key=exit_requested=input_pending=0;
                    dz_clock_reset(); dz_srand(1); plot_color=3;
                    if (interrupted) { inject_key='n'; inject_tick=2; }
                    current_graphics_key='z'; dispatch_fade_key();
                    if (interrupted) { current_graphics_key='z'; dispatch_fade_key(); }
                    struct dz_canvas_view v=canvas_capture(); size_t n=(size_t)v.width*v.height;
                    if (!native) { expected=malloc(n); assert(expected); memcpy(expected,v.indices,n); }
                    else {
                        size_t differences=0,first=0;
                        for (size_t i=0;i<n;++i) if (expected[i]!=v.indices[i]) { if (!differences) first=i; ++differences; }
                        if (differences) fprintf(stderr,"erase mode=%d policy=%d interrupted=%d differences=%zu first=%zu,%zu expected=%u actual=%u\n",
                            mode,policy,interrupted,differences,first%(size_t)v.width,first/(size_t)v.width,expected[first],v.indices[first]);
                        assert(!differences); free(expected);
                    }
                }
            }
        }
        teardown();
    }
    SDL_DestroyRenderer(r); SDL_DestroySurface(s); dz_shutdown();
    puts("Native fade XOR is self-inverse; completed and resumed erasers equal Faithful at every pixel");
    return 0;
}
