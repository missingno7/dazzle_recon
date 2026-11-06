/* The scalar/patch primitives must preserve what a cell samples. */
#include "dazzle.h"
#include "dvdt.h"
#include "dzconst.h"
#include "effects/fades.h"
#include "effects/effects.h"
#include "canvas_capture.h"
#include <assert.h>
#include <stdio.h>
#undef assert
#define assert(p) do { if (!(p)) { fprintf(stderr,"assertion %s at %d\n",#p,__LINE__);fflush(stderr);_Exit(1); } } while (0)
static SDL_Renderer *renderer;
static int armed;
static uint32_t stop_tick;
static void timer(void)
{
    graphics_timer_tick_isr();
    if (armed && dz_bios_time()>=stop_tick) { current_graphics_key='n';armed=0; }
}
static void setup(int kind,int policy)
{
    dz_timer_tick_handler=dz_keyboard_irq_handler=dz_mouse_event_handler=NULL;
    free(palette_buffer);palette_buffer=NULL;
    free(linewipe_history);linewipe_history=NULL;
    free(ball_bitmap_pixels);ball_bitmap_pixels=NULL;
    palette_alternate_mode=palette_random_mode=palette_fixed_mode=0;
    targa64_mode=use_bios_palette=split_vga_palette_updates=use_8086_palette_io=0;
    background_fill_word=page_paragraph=palette_cycle_offset=0;palette_step=3;
    display_freeze_active=display_freeze_cycle_requested=0;
    show_mode=show_duration_ticks=display_pan_state=semi_auto_mode=0;
    video_adapter=kind<2 ? ADAPTER_VGA : ADAPTER_EGA;
    bios_resolution_mode=kind==1 || kind==2;
    configure_video_backend(1);initialize_math_tables();
    memset(palette_buffer,0,palette_buffer_bytes+3);palette_build_callback(0);
    dz_video_set_mode(kind<2 ? (kind==1 ? DZ_MODE_VGA_200 : DZ_MODE_VGA_400) : DZ_MODE_EGA_350,1);
    dz_display.presentation=(enum dz_presentation)policy;
    assert(dz_canvas_configure(renderer,3840,2160,dz_display.presentation)==1);
    dz_canvas_take_restart();dz_canvas_select_page(0);dz_canvas_start(0);dz_canvas_split(1023);
    dz_canvas_clear(0);dz_canvas_raster_xor(0);dz_clock_reset();dz_srand(1);
    plot_color=3;draw_delay_count=1;palette_cycle_ticks=UINT16_MAX;muted=1;
    current_graphics_key=previous_graphics_key=input_pending=exit_requested=0;
    auto_fade_pause_ticks=240;display_cleared=palette_elapsed_ticks=countdown_ticks=0;
    dz_timer_tick_handler=timer;
    for (unsigned i=0;i<24;++i) fade_records[i].enabled=1;
}
int main(void)
{
    dz_options=(struct dz_options){1,1,1,1.0,NULL};
    dz_display.canvas=DZ_CANVAS_SCREEN;dz_display.native_effects=UINT64_MAX;
    assert(dz_native_fades_select("all"));
    assert(dz_initialize() && SDL_InitSubSystem(SDL_INIT_VIDEO));
    SDL_Surface *s=SDL_CreateSurface(16,16,SDL_PIXELFORMAT_ARGB8888);
    renderer=SDL_CreateSoftwareRenderer(s);assert(renderer);
    initialize_program_defaults();
    for (int policy=0;policy<=3;policy+=3) for (int kind=0;kind<4;++kind) {
        setup(kind,policy);
        /* Both logical samples are 7. Only the source patch has a fine stripe.
         * A sampled-colour exchange necessarily loses it. Use actual DZLE. */
        dz_canvas_clear(7);SDL_Rect a,b;
        int divisor=kind==3 ? 2 : 1;
        assert(dz_canvas_fade_cell_rect(screen_half_width+20,(screen_half_height+20)/divisor,&a));
        assert(dz_canvas_fade_cell_rect(screen_half_width+60,(screen_half_height+40)/divisor,&b));
        dz_canvas_native_span(a.y+a.h/2,a.x,a.x+a.w,11);
        dz_canvas_native_span(a.y+a.h/2-1,a.x,a.x+a.w,11);
        struct dz_canvas_store_audit audit={0};dz_canvas_store_audit=&audit;
        dz_canvas_fade_begin('q');
        swap_pixel_rectangles(0,20,20,60,40,1,(int16_t)divisor);
        dz_canvas_fade_end();dz_canvas_store_audit=NULL;
        struct dz_canvas_view v=canvas_capture();int stripe=0,ground=0;
        for (int y=b.y;y<b.y+b.h;++y) for (int x=b.x;x<b.x+b.w;++x) {
            unsigned c=v.indices[(size_t)y*v.width+x];stripe+=c==11;ground+=c==7;
        }
        if (!(stripe && ground && !audit.faithful && audit.native)) fprintf(stderr,
            "patch kind=%d policy=%d stripe=%d ground=%d faithful=%llu native=%llu\n",kind,policy,stripe,ground,
            (unsigned long long)audit.faithful,(unsigned long long)audit.native);
        assert(stripe && ground && !audit.faithful && audit.native);
        dz_canvas_clear(0);dz_canvas_fade_begin('m');
        dz_native_fade_mesh(1,9);draw_x=draw_y=20;plot_color=13;plot_callback(0);
        dz_canvas_fade_end();v=canvas_capture();
        assert(v.indices[(size_t)(a.y+a.h/2)*v.width+a.x+a.w/2]==13);
        dz_canvas_clear(0);dz_canvas_fade_begin('f');draw_x=10;draw_y=20;
        dz_native_fade_pen(10.25,20.25,4);plot_color=2;square(4,0);
        dz_native_fade_pen(10.25,20.25,4);plot_color=0;square(4,0);
        dz_canvas_fade_end();v=canvas_capture();
        for (int i=0;i<v.width*v.height;++i) assert(!v.indices[i]);
        if (policy==3) {
            /* Two nested contour sides: the inner one's reflected copy must
             * not appear in the empty field behind the outer real frontier.
             * Test the gap, not just contrast in the edge-adjacent column. */
            dz_canvas_clear(0);dz_canvas_fade_begin('x');
            double outer=screen_half_width+20,inner=screen_half_width-15;
            const double xx[]={outer,outer},yy[]={-30,30};
            dz_native_fade_polyline(xx,yy,2,1,1,11);
            const double ix[]={inner,inner};
            dz_native_fade_polyline(ix,yy,2,1,1,13);
            struct dz_native_mapping m=dz_canvas_native_mapping(screen_half_width,screen_half_height);
            v=canvas_capture();SDL_FRect clip=dz_canvas_content();
            int row=(int)m.cy;
            for (int side=0;side<2;++side) {
                double centre=m.cx+(side ? 1 : -1)*inner*m.unit_x;
                int col=(int)(2*(side ? clip.x+clip.w : clip.x)-centre);
                assert(!v.indices[(size_t)row*v.width+col]);
            }
            dz_canvas_fade_end();
        }
        /* Moving round points erase exactly their own real-position ink,
         * including fractional centres, reflections and either 4:3 edge.
         * Inspect BEFORE finish: deferred cleanup must not hide residue. */
        for (const char *key="so";*key;++key) for (int sample=0;sample<3;++sample) {
            dz_canvas_clear(0);dz_canvas_fade_begin((unsigned)*key);
            draw_x=sample==0 ? 17 : screen_half_width+sample-2;
            draw_y=kind==3 ? 24 : 12;
            double px=draw_x+0.3125,py=draw_y-0.21875;
            dz_native_fade_pen(px,py,0);plot_color=11;plot_callback(3);
            dz_native_fade_pen(px,py,0);plot_color=0;plot_callback(3);
            v=canvas_capture();
            for (int i=0;i<v.width*v.height;++i) assert(!v.indices[i]);
            dz_canvas_fade_end();
        }
        /* The incoming picture is not a round point. Zero invalidates its
         * owned source domain at the store, not only at dispatcher return. */
        dz_canvas_clear(0);draw_x=20;draw_y=kind==3 ? 40 : 20;
        assert(dz_canvas_fade_cell_rect(screen_half_width+draw_x,
            (screen_half_height+draw_y)/divisor,&a));
        dz_canvas_native_span(a.y,a.x,a.x+1,11);
        dz_canvas_fade_begin('s');dz_native_fade_pen(draw_x+0.49,draw_y+0.49,0);
        plot_color=0;plot_callback(0);v=canvas_capture();
        assert(!v.indices[(size_t)a.y*v.width+a.x]);dz_canvas_fade_end();
        {
            uint32_t positions[641],speed[641];
            for (int i=0;i<641;++i) {
                positions[i]=(uint32_t)(-screen_half_height*65536);
                speed[i]=(uint32_t)-51;
            }
            speed[76]=34;dz_canvas_fade_begin('d');plot_color=13;
            dz_native_fade_drip(76,positions,speed);dz_canvas_fade_end();v=canvas_capture();
            /* A negative fractional displacement must not become a full-height
             * polygon, even next to the tiny positive speed from a real seed. */
            for (int y=1;y<v.height;++y) for (int x=0;x<v.width;++x)
                assert(!v.indices[(size_t)y*v.width+x]);
        }
        if (kind<2) {
            struct dz_native_mapping m=dz_canvas_native_mapping(screen_half_width,screen_half_height);
            int stripe_y=(int)(m.cy+12*m.unit_y),stripe_x=(int)(m.cx+15*m.unit_x);
            dz_canvas_native_span(stripe_y,(int)(m.cx+10*m.unit_x),(int)(m.cx+20*m.unit_x),11);
            dz_canvas_fade_begin('g');
            dz_native_fade_plasma(10,10,20,20,240,16,16,240);
            v=canvas_capture();int x=(int)(m.cx+15*m.unit_x),y=(int)(m.cy+15*m.unit_y);
            assert(v.indices[(size_t)y*v.width+x]>=126 && v.indices[(size_t)y*v.width+x]<=130);
            assert(v.indices[(size_t)stripe_y*v.width+stripe_x]==11);
            dz_canvas_fade_end();
        }
        if (kind==3) {
            uint32_t positions[641],speed[641];uint8_t *even=NULL;
            for (int i=0;i<641;++i) { positions[i]=20*65536+16384;speed[i]=32768; }
            for (int odd=0;odd<2;++odd) {
                dz_canvas_clear(0);dz_canvas_fade_begin('d');draw_y=(int16_t)odd;plot_color=13;
                dz_native_fade_drip(screen_half_width,positions,speed);dz_canvas_fade_end();
                v=canvas_capture();size_t n=(size_t)v.width*v.height;
                if (!odd) { even=malloc(n);assert(even);memcpy(even,v.indices,n); }
                else { assert(!memcmp(even,v.indices,n));free(even); }
            }
        }
        /* Stop and resume through the real dispatcher, then erase the scene.
         * The full physical picture (including both Extended edges) must be
         * clean, with no retained callback, sample override or page residue. */
        for (const char *key="acdfgoqsvwyx";*key;++key) {
            if (kind>=2 && strchr("gv",*key)) continue;
            setup(kind,policy);dz_canvas_rect(0,0,dz_video.width,dz_video.height,7);
            stop_tick=dz_bios_time()+2;armed=1;
            current_graphics_key=(uint16_t)*key;dispatch_fade_key();
            assert(!dz_native_fade_plot);armed=0;
            current_graphics_key=(uint16_t)*key;dispatch_fade_key();
            assert(!dz_native_fade_plot);
            dz_canvas_raster_xor(0); /* display boundary's normal entry state */
            dz_canvas_fade_begin('w');fade_wipe(0);dz_canvas_fade_end();
            v=canvas_capture();
            SDL_FRect clip=dz_canvas_content();
            for (int y=(int)clip.y;y<(int)(clip.y+clip.h);++y)
                for (int x=policy==3 ? 0 : (int)clip.x;x<(policy==3 ? v.width : (int)(clip.x+clip.w));++x)
                    if (v.indices[(size_t)y*v.width+x]) {
                        fprintf(stderr,"residue key=%c kind=%d policy=%d pixel=%d,%d index=%u\n",
                            *key,kind,policy,x,y,v.indices[(size_t)y*v.width+x]);fflush(stderr);_Exit(1);
                    }
        }
    }
    for (int policy=0;policy<=3;policy+=3) {
        setup(0,policy);
        assert(dz_canvas_configure(renderer,320,180,dz_display.presentation)==1);
        dz_canvas_take_restart();dz_canvas_clear(0);
        for (int y=0;y<180;++y) for (int x=0;x<320;x+=3)
            dz_canvas_native_span(y,x,x+3,(uint8_t)(3+(x/3+y/7)%13));
        struct dz_canvas_view v=canvas_capture();size_t n=(size_t)v.width*v.height;
        uint8_t *before=malloc(n);assert(before);memcpy(before,v.indices,n);
        struct dz_canvas_store_audit audit={0};dz_canvas_store_audit=&audit;
        current_graphics_key='q';dispatch_fade_key();dz_canvas_store_audit=NULL;
        v=canvas_capture();assert(audit.native && !audit.faithful);
        assert(memcmp(before,v.indices,n));free(before);
    }
    dz_timer_tick_handler=NULL;SDL_DestroyRenderer(renderer);SDL_DestroySurface(s);dz_shutdown();
    puts("Physical patches retain detail; height interpolation and real square erasure pass; interrupted/resumed fades cleanly erase at 4K.");
    return 0;
}
