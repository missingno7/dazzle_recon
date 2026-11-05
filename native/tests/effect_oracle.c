/* Real generators and stores, in a small deterministic Auto controller.
 * Each invocation starts a fresh process; Classic supplies the logical oracle. */
#include "dazzle.h"
#include "dzconst.h"
#include "effects/effects.h"
#include "dvdt.h"
#include "canvas_capture.h"
#include "platform/testctl.h"
#include <inttypes.h>
#include <stdio.h>
#include <windows.h>

static const char *prefix;
static int width, height, classic, ordinal, fading, final_observation;
static unsigned effect;
static ImageConfigRow *record;
static SDL_Renderer *renderer;
static int interrupt_fade;
static char deadline_paths[64][4096];
static unsigned deadline_count;
static uint64_t fade_start;
static struct dz_canvas_store_audit stores;
static uint8_t written[DZ_VRAM_PIXELS];
static FILE *trace;
static void (*original_plot)(uint16_t);
extern int16_t yarnball_draw_flags, gumballs_draw_flags;
static void trace_plot(uint16_t symmetry)
{
    fprintf(trace,"%d,%d,%d,%u,%u,%ld,%ld,%d,%d,%d,%d,%d,%d\n",ordinal,
        draw_x,draw_y,plot_color,symmetry,(long)ellipse_step_radius_x,(long)ellipse_step_radius_y,
        ellipse_trace_index,ellipse_trace_limit,ellipse_trace_x,ellipse_trace_y,
        yarnball_draw_flags,gumballs_draw_flags);
    original_plot(symmetry);
}
static void bytes(const char *suffix, const void *data, size_t size)
{
    char path[4096];
    assert(snprintf(path,sizeof path,"%s-%d.%s",prefix,ordinal,suffix) < (int)sizeof path);
    FILE *f = fopen(path,"wb"); assert(f);
    assert(fwrite(data,1,size,f) == size && !fclose(f));
}
static void observe(const char *phase)
{
    if (!classic) assert(dz_canvas_configure(renderer,width,height,dz_display.presentation) == 1);
    if (classic) bytes("idx",dz_canvas_memory(),(size_t)dz_video.width*dz_video.height);
    else bytes("idx",canvas_capture().indices,(size_t)width*height);
    bytes("logical",dz_canvas_memory(),(size_t)dz_video.width*dz_video.height);
    if (!classic && fading && final_observation && dz_display.presentation == DZ_PRESENT_EXTENDED) {
        uint8_t *survives = malloc((size_t)width*height); assert(survives);
        memset(survives,1,(size_t)width*height);
        SDL_FRect clip = dz_canvas_content();
        for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x) {
            if (x >= clip.x && x < clip.x+clip.w) continue;
            uint32_t a = dz_canvas_fold_address(x,y);
            survives[(size_t)y*width+x] = a < DZ_VRAM_PIXELS && (!written[a] || dz_canvas_memory()[a]);
        }
        bytes("survives",survives,(size_t)width*height); free(survives);
    }
    uint8_t rgb[256*3];
    for (unsigned i = 0; i < 256; ++i) {
        if (dz_video.mode == DZ_MODE_EGA_350) {
            unsigned c = dz_video.ega_attr[i&15u];
            rgb[3*i] = (uint8_t)(((c>>2)&1u)*170u+((c>>5)&1u)*85u);
            rgb[3*i+1] = (uint8_t)(((c>>1)&1u)*170u+((c>>4)&1u)*85u);
            rgb[3*i+2] = (uint8_t)((c&1u)*170u+((c>>3)&1u)*85u);
        } else for (int j = 0; j < 3; ++j) rgb[3*i+j] = (uint8_t)((dz_video.dac[i][j]&63u)*255u/63u);
    }
    bytes("pal",rgb,sizeof rgb);
    char path[4096]; snprintf(path,sizeof path,"%s-%d.state",prefix,ordinal);
    FILE *f = fopen(path,"wb"); assert(f);
#define FIELD(name) assert(fwrite((const void *)&name,1,sizeof name,f) == sizeof name)
#include "state_fields.inc"
    FIELD(plasma_random); FIELD(plasma_width); FIELD(plasma_height); FIELD(plasma_color_limit);
#undef FIELD
    assert(fwrite(&dz_video,1,sizeof dz_video,f) == sizeof dz_video);
    assert(fwrite(palette_buffer,1,palette_buffer_bytes,f) == palette_buffer_bytes);
    assert(!fclose(f));
    SDL_FRect r = dz_present_viewport(width,height,dz_display.presentation);
    SDL_FRect clip = dz_canvas_content();
    printf("%d %s %d %d %.17g %.17g %.17g %.17g %llu %llu %u %d %.0f %.0f %.0f %.0f\n",ordinal,phase,
        dz_video.width,dz_video.height,r.x,r.y,r.w,r.h,(unsigned long long)dz_virtual_ns(),
        (unsigned long long)dz_rand_calls(),dz_random_state(),dz_native_instance(effect),clip.x,clip.y,clip.w,clip.h);
    ++ordinal;
    if (dz_canvas_store_audit) printf("SIDES reach=%llu fold=%llu min=%d max=%d\n",
        (unsigned long long)stores.reach,(unsigned long long)stores.fold,stores.min_x,stores.max_x);
}
static void moment(void)
{
    observe("fade");
    if (interrupt_fade) { current_graphics_key = 'n'; interrupt_fade = 0; }
}
static void checkpoint(void)
{
    assert(deadline_count < 64);
    char *path = deadline_paths[deadline_count++];
    snprintf(path,4096,"%s-%d.receipt",prefix,ordinal);
    assert(dz_testctl_state(dz_virtual_ns(),path));
    dz_testctl_poll();
}
static void transition(unsigned key)
{
    current_graphics_key = (uint16_t)key;
    if (key == 'p') {
        display_pan_state = 2;
        saved_pan_draw_delay = draw_delay_count;
        display_boundary_fade_and_pan();
    } else dispatch_fade_key();
}
static void draw(int rebuild)
{ record->draw((int16_t)rebuild); plot_callback(draw_symmetry); }
static void auto_image(void)
{
    ULONG64 before = 0, after = 0;
    int timing = getenv("DAZZLE_RASTER_CPU") != NULL;
    if (timing) assert(QueryThreadCycleTime(GetCurrentThread(),&before));
    image_work_count = 0;
    unsigned budget = (uint16_t)(record->size*screen_extent);
    image_work_limit = (uint16_t)(dz_random((uint16_t)budget)+(budget>>2));
    draw(1);
    do { draw(0); } while ((uint16_t)image_work_count++ <= image_work_limit);
    if (timing) {
        assert(QueryThreadCycleTime(GetCurrentThread(),&after));
        printf("RASTER cycles=%llu\n",(unsigned long long)(after-before));
    }
}
int main(int argc, char **argv)
{
    assert(argc == 10 || argc == 13);
    fading = argc == 13;
    effect = (unsigned)atoi(argv[1]); int kind = atoi(argv[2]);
    classic = !strcmp(argv[3],"classic");
    dz_display.canvas = classic ? DZ_CANVAS_CLASSIC : DZ_CANVAS_SCREEN;
    dz_display.native_effects = !strcmp(argv[3],"native") ? UINT64_MAX : 0;
    dz_display.presentation = (enum dz_presentation)atoi(argv[4]);
    width = atoi(argv[5]); height = atoi(argv[6]);
    unsigned seed = (unsigned)atoi(argv[7]); prefix = argv[8];
    int full = atoi(argv[9]);
    assert(effect < DZ_EFFECT_COUNT);
    record = effect < 36 ? alt_image_records+effect : ctrl_image_records+effect-36;
    dz_options.headless = dz_options.deterministic = 1; dz_options.seed = seed;
    assert(dz_initialize() && SDL_InitSubSystem(SDL_INIT_VIDEO));
    SDL_Surface *s = SDL_CreateSurface(16,16,SDL_PIXELFORMAT_ARGB8888);
    renderer = SDL_CreateSoftwareRenderer(s); assert(renderer);
    dz_srand((uint16_t)seed);
    video_adapter = kind < 2 ? ADAPTER_VGA : ADAPTER_EGA;
    bios_resolution_mode = kind == 1 || kind == 2;
    initialize_program_defaults(); configure_video_backend(1); initialize_math_tables();
    input_pending = exit_requested = current_graphics_key = previous_graphics_key = 0;
    memset(palette_buffer,0,palette_buffer_bytes+3u);
    palette_build_callback(0);
    dz_timer_tick_handler = dz_keyboard_irq_handler = dz_mouse_event_handler = NULL;
    dz_video_set_mode(kind < 2 ? (kind == 1 ? DZ_MODE_VGA_200 : DZ_MODE_VGA_400) : DZ_MODE_EGA_350,1);
    palette_upload_callback(palette_buffer+palette_cycle_offset);
    if (!classic) assert(dz_canvas_configure(renderer,width,height,dz_display.presentation) == 1);
    dz_canvas_take_restart(); dz_canvas_select_page(0); dz_canvas_clear(0);
    current_image_draw = record->draw; current_image_index = (uint16_t)effect;
    plot_color = 3; draw_delay_count = 1; draw_symmetry = record->enabled ? 3 : 1;
    palette_cycle_ticks = UINT16_MAX; muted = 1;
    semi_auto_mode = input_pending = exit_requested = 0;
    draw_x = draw_y = image_work_count = corkscrew_color_counter = 0;
    dz_clock_reset(); dz_srand((uint16_t)seed);
    if (argc == 13) {
        /* Real keyed dispatcher and timer, fresh source picture; observations
         * use the existing exact-deadline seam, never extra charged work. */
        for (unsigned i = 0; i < 24; ++i) fade_records[i].enabled = 1;
        for (int i = 0; i < 4; ++i) auto_image();
        if (*argv[10] == 'p') {
            dz_canvas_select_page(video_adapter == ADAPTER_EGA ? 1750 : 2000);
            auto_image();
        }
        dz_native_plot = NULL;
        dz_timer_tick_handler = graphics_timer_tick_isr;
        dz_keyboard_irq_handler = keyboard_pending_isr;
        auto_fade_pause_ticks = 240;
        display_cleared = palette_elapsed_ticks = countdown_ticks = 0;
        dz_testctl_state_observer = moment;
        fade_start = dz_virtual_ns();
        checkpoint();
        dz_canvas_fade_begin((unsigned char)*argv[10]);
        checkpoint(); /* entry must preserve every pixel, including Native ink */
        stores = (struct dz_canvas_store_audit){.min_x=INT_MAX,.max_x=INT_MIN,.written=written};
        dz_canvas_store_audit = &stores;
        char *cursor = argv[12], *end;
        while (*cursor) {
            uint64_t offset = strtoull(cursor,&end,10);
            assert(end != cursor && deadline_count < 64);
            char *path = deadline_paths[deadline_count++];
            snprintf(path,4096,"%s-%u.receipt",prefix,deadline_count-1);
            assert(dz_testctl_state(fade_start+offset,path));
            cursor = *end == ',' ? end+1 : end;
        }
        interrupt_fade = !strcmp(argv[11],"interrupt");
        ULONG64 before, after;
        assert(QueryThreadCycleTime(GetCurrentThread(),&before));
        transition((unsigned char)*argv[10]);
        assert(QueryThreadCycleTime(GetCurrentThread(),&after));
        printf("FADE key=%c duration=%llu cycles=%llu cleared=%d\n",*argv[10],
            (unsigned long long)(dz_virtual_ns()-fade_start),(unsigned long long)(after-before),display_cleared);
        final_observation = 1;
        checkpoint();
        if (strcmp(argv[11],"normal")) { transition((unsigned char)*argv[10]); checkpoint(); }
        goto finish;
    }
    /* Optional Classic-only source trace. Callback identity has EGA semantics,
     * so this diagnostic is explicitly restricted to the VGA reference. */
    if (getenv("DAZZLE_PLOT_TRACE")) {
        assert(classic && kind < 2);
        trace = fopen(getenv("DAZZLE_PLOT_TRACE"),"w"); assert(trace);
        fputs("phase,x,y,color,symmetry,rx,ry,index,limit,tx,ty,yarn_flags,ball_flags\n",trace);
        original_plot = plot_callback; plot_callback = trace_plot;
    }
    image_work_limit = 1; draw(1); observe("first");
    /* Same-owner unplotted rebuild followed by an actual rebuild. */
    image_work_count = 0; record->draw(1); draw(1); observe("restart");
    dz_canvas_clear(0); auto_image(); observe("auto");
    if (full) {
        for (int i = 0; i < 3; ++i) auto_image();
        observe("long");
        unsigned page = video_adapter == ADAPTER_EGA ? 1750u : 2000u;
        dz_canvas_select_page((uint16_t)page); auto_image();
        dz_canvas_start((unsigned)dz_video.height); observe("pan");
        dz_canvas_split((unsigned)dz_video.height/2-1); observe("split");
    }
finish:
    if (trace) { plot_callback = original_plot; assert(!fclose(trace)); }
    free(palette_buffer); palette_buffer = NULL;
    free(linewipe_history); linewipe_history = NULL;
    free(ball_bitmap_pixels); ball_bitmap_pixels = NULL;
    SDL_DestroyRenderer(renderer); SDL_DestroySurface(s); dz_shutdown();
    return 0;
}
