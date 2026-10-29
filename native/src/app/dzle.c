/* DAZZLE 5.0 main module (DZLE), native C17 port. */
#include "dazzle.h"
#include "platform/hybrid.h"
#include "platform/testctl.h"
#include "dzconst.h"
#include "dzconio.h"
#include <ctype.h>
#include <direct.h>
#include <math.h>
#include <time.h>
#include "platform/dzle_support.h"

/* DOS signed shifts round toward minus infinity. Fixed-point products retain
 * their 32-bit low word before shifting, as Borland's long arithmetic did. */
static int32_t asr32(int32_t value, unsigned bits)
{
    int64_t divisor = INT64_C(1) << bits;
    return (int32_t)(value < 0 ? -((-(int64_t)value + divisor - 1) / divisor) : value / divisor);
}

static int32_t mul32(int32_t first, int32_t second)
{
    uint32_t product = (uint32_t)first * (uint32_t)second;
    return product <= INT32_MAX ? (int32_t)product : (int32_t)((int64_t)product - INT64_C(4294967296));
}

static size_t checked_index(int index, size_t length)
{
    if (index < 0 || (size_t)index >= length) {
        fprintf(stderr, "DZLE array index %d outside %zu entries\n", index, length);
        abort();
    }
    return (size_t)index;
}
#define ELEMENT(array, index) ((array)[checked_index((index), sizeof(array)/sizeof((array)[0]))])

/* Large EGA arches overflow the original 700-word cache into the next BSS
 * word (DS:535C), which is the loop's own completion flag. The nonzero
 * direction stops caching there; inclusive playback also reads DS:535E. */
static int16_t *ellipse_direction(int16_t index)
{
    if (index == 700) return &ellipse_trace_done;
    if (index == 701) return &ellipse_fill_x;
    return &ELEMENT(ellipse_direction_steps, index);
}

/* BIOS keyboard services, LED bits, and the normalized clear command. */
#define BIOS_KEY_READ 0
#define BIOS_KEY_PENDING 1
#define BIOS_TYPEMATIC_DELAY_1S 3
#define BIOS_TYPEMATIC_RATE_2CPS 31
#define KEYBOARD_LED_MASK 0x70
#define COMMAND_CLEAR 0x6e
#define OPTION_SKIP_INTEGRITY 'i'

#define TEXT_COLOR_YELLOW 14
#define TEXT_COLOR_BLINK_LIGHT_RED 140
#define TEXT_COLOR_CYAN 3

/* Initialized state at DS:0094; literal pool follows the record tables. */
char *executable_name_pointer="DAZZLE.EXE";
char *help_program_name="DAZZLE";
char *program_revision="5.0";
char *registration_notice_pointer="Comments and inquires about updates and other products are welcome at\n\rour mailing address. $15 ($20US for non-U.S.A. sites) registration payable\n\rto MicroTronics required after 10 days usage, which makes you eligible\n\rfor an update disk and puts you on our PRIVATE mailing list.\n\rAdd $5 extra for a printed manual. DOS & Windows Screen Savers available\n\rfor an additional cost.\n\r";
int16_t direction_x[4]={0,-1,0,1};
int16_t direction_y[4]={1,0,-1,0};
ConeDirections cone_directions[4]={
    {{-1,0,-2},{1,2,0}},
    {{-1,0,-2},{-1,-2,0}},
    {{1,0,2},{-1,-2,0}},
    {{1,0,2},{1,2,0}}
};
/* Four 8 by 8 grid masks, stored in row order. */
uint8_t grid_patterns[256]={
    0,0,0,1,1,0,0,0,
    0,0,0,1,1,0,0,0,
    0,0,0,0,1,1,0,0,
    1,1,0,0,0,1,1,1,
    1,1,1,0,0,0,1,1,
    0,0,1,1,0,0,0,0,
    0,0,0,1,1,0,0,0,
    0,0,0,1,1,0,0,0,
    0,0,0,1,1,0,0,0,
    0,0,0,1,1,0,0,0,
    0,0,1,1,0,0,0,0,
    1,1,1,0,0,0,1,1,
    1,1,0,0,0,1,1,1,
    0,0,0,0,1,1,0,0,
    0,0,0,1,1,0,0,0,
    0,0,0,1,1,0,0,0,
    0,0,0,1,1,0,0,0,
    0,0,0,1,1,0,0,0,
    0,0,0,1,1,0,0,0,
    1,1,0,1,1,0,1,1,
    1,1,0,1,1,0,1,1,
    0,0,0,1,1,0,0,0,
    0,0,0,1,1,0,0,0,
    0,0,0,1,1,0,0,0,
    0,0,0,1,1,0,0,0,
    0,0,0,1,1,0,0,0,
    0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,
    0,0,0,0,0,0,0,0,
    0,0,0,1,1,0,0,0,
    0,0,0,1,1,0,0,0
};
int16_t diamond_x[4]={1,1,-1,-1};
int16_t diamond_y[4]={1,-1,-1,1};
int16_t zigstream_dx[2]={0,1};
int16_t zigstream_dy[2]={1,0};

/* Bresenham line iterator: initialize from draw_x/draw_y to the endpoint,
 * or advance one point. Returns 1 at completion; leaves plotting to the caller.
 * Original 066A:000E, 269 bytes. */
int16_t line_step(int16_t end_x,int16_t end_y,int16_t initialize) {
    if(initialize) {
        dx=(int16_t)(end_x-draw_x); dy=(int16_t)(end_y-draw_y);
        sx=dx>0?1:dx==0?0:-1;
        sy=dy>0?1:dy==0?0:-1;
        dx=(int16_t)(abs(dx)*2); dy=(int16_t)(abs(dy)*2);
        if(dx>=dy) error=(int16_t)(dy-dz_asr16(dx,1)); else error=(int16_t)(dx-dz_asr16(dy,1));
        if(dx==0 && dy==0) return 1; else return 0;
    } else {
        if(dx>=dy) {
            if(error>=0) {error=(int16_t)(error-dx); draw_y = (int16_t)(draw_y+sy);}
            draw_x = (int16_t)(draw_x+sx); error=(int16_t)(error+dy);
            if(draw_x==end_x) return 1; else return 0;
        } else {
            if(error>=0) {error=(int16_t)(error-dy); draw_x = (int16_t)(draw_x+sx);}
            draw_y = (int16_t)(draw_y+sy); error=(int16_t)(error+dx);
            if(draw_y==end_y) return 1;
        }
    }
    return 0;
}

/* Select plot_color from a random or coordinate-based rule, or advance it
 * (once per four calls on EGA). Mask to the palette and reserve indices 0..2.
 * Original 066A:011B, 336 bytes. */
void choose_color(int16_t color_rule) {
    uint32_t color_term,y_term;
    /* Products wrap as signed 16-bit before sign extension to unsigned long. */
    switch(color_rule) {
    default: if(video_adapter!=ADAPTER_EGA) plot_color++; else {ega_color_substep++; if((ega_color_substep&=3)==0) plot_color++;} break;
    case 1: plot_color = (uint16_t)dz_random((uint16_t)color_count); break;
    case 2: plot_color = (uint16_t)(abs(draw_x)); break;
    case 3: plot_color = (uint16_t)(abs(draw_y)); break;
    case 4: plot_color = (uint16_t)(abs(draw_x)+abs(draw_y)); break;
    case 5: color_term=(uint32_t)(int32_t)(int16_t)(draw_x*draw_x); y_term=(uint32_t)(int32_t)(int16_t)(draw_y*draw_y); color_term+=y_term; plot_color = (uint16_t)((color_term>>5)&(uint16_t)(color_count-1)); break;
    case 6: color_term=(uint32_t)(int32_t)(int16_t)(draw_x*draw_x); y_term=(uint32_t)(int32_t)(int16_t)(draw_y*draw_y); color_term-=y_term; plot_color = (uint16_t)((color_term>>5)&(uint16_t)(color_count-1)); break;
    case 7: y_term=(uint32_t)(int32_t)dz_asr16((int16_t)(draw_y*draw_y),5); color_term=(uint32_t)(int32_t)(int16_t)abs(draw_x); plot_color = (uint16_t)(y_term-color_term); break;
    case 8: color_term=(uint32_t)(int32_t)dz_asr16((int16_t)abs((int16_t)(draw_x*draw_y)),5); plot_color = (uint16_t)color_term; break;
    }
    plot_color&=color_count-1;
    if(plot_color<FIRST_CYCLING_COLOR) plot_color = FIRST_CYCLING_COLOR;
}

/* Fill a square at draw_x/draw_y through the symmetry callback; positive
 * side_length preserves the pen position. Cycle the palette between columns.
 * Original 066A:026B, 80 bytes. */
void square(int16_t side_length,int16_t symmetry) {
    int16_t column,row;
    for(column=1;column<=side_length;column++) {
        for(row=1;row<=side_length;row++) { plot_callback((uint16_t)symmetry); draw_y++; }
        draw_x++; draw_y-=side_length;
        if(palette_elapsed_ticks>=palette_cycle_ticks) cycle_palette();
    }
    draw_x-=side_length;
}

/* Cache one left/right span per polygon row on rebuild, then fill and/or
 * outline it with flag-selected colors. Element zero supplies the translation;
 * arrays need a writable closing slot at vertex_count, which is set to zero.
 * Original 066A:02BB, 797 bytes. */
void draw_polygon(int16_t rebuild,int16_t vertex_count,int16_t *vertex_x,int16_t *vertex_y,uint16_t fill_color,uint16_t edge_color,uint16_t flags) {
    int16_t row_right,end_x,end_y,saved_fill_color,color_mode,line_done;
    int16_t index;
    polygon_origin_x=vertex_x[0];
    polygon_origin_y=vertex_y[0];
    /* Temporarily remove the translation and close the vertex list at zero. */
    vertex_x[0]=vertex_y[0]=vertex_x[vertex_count]=vertex_y[vertex_count]=0;
    if(rebuild) {
        polygon_max_y=-screen_half_height;
        polygon_min_y=screen_half_height;
        for(index=0;index<vertex_count;index++) {
            if(vertex_y[index]>polygon_max_y) polygon_max_y=vertex_y[index];
            if(vertex_y[index]<polygon_min_y) polygon_min_y=vertex_y[index];
        }
        polygon_row_limit=polygon_max_y-polygon_min_y;
        for(index=0;index<=polygon_row_limit;index++) {
            ELEMENT(polygon_row_left, index)=screen_half_width;
            ELEMENT(polygon_row_right, index)=-screen_half_width;
        }
        for(index=0;index<vertex_count;index++) {
            draw_x = (int16_t)(vertex_x[index]); draw_y = (int16_t)(vertex_y[index]-polygon_min_y);
            end_x = (int16_t)(vertex_x[index+1]); end_y = (int16_t)(vertex_y[index+1]-polygon_min_y); line_done=1;
            do {
                line_done=line_step(end_x,end_y,line_done);
                if(draw_x<ELEMENT(polygon_row_left, draw_y)) ELEMENT(polygon_row_left, draw_y)=draw_x;
                if(draw_x>ELEMENT(polygon_row_right, draw_y)) ELEMENT(polygon_row_right, draw_y)=draw_x;
            } while(!line_done);
            if(palette_elapsed_ticks>=palette_cycle_ticks) cycle_palette();
        }
    }
    if(flags&SHAPE_FILL) {
        plot_color = (uint16_t)(fill_color);
        saved_fill_color = (int16_t)(fill_color);
        color_mode=flags&POLYGON_FILL_COLOR_MASK;
        draw_y = (int16_t)(polygon_min_y+polygon_origin_y);
        for(index=0;index<=polygon_row_limit;index++) {
            if(color_mode==POLYGON_FILL_COLOR_PER_PIXEL) plot_color = (uint16_t)(saved_fill_color);
            draw_x = (int16_t)(ELEMENT(polygon_row_left, index)+polygon_origin_x);
            row_right=ELEMENT(polygon_row_right, index)+polygon_origin_x;
            while(draw_x<=row_right) {
                (*plot_callback)(draw_symmetry);
                draw_x++;
                if(color_mode==POLYGON_FILL_COLOR_PER_PIXEL) { plot_color++; if(plot_color>=color_count) plot_color = FIRST_CYCLING_COLOR; }
                image_work_count++;
                if(palette_elapsed_ticks>=palette_cycle_ticks) cycle_palette();
            }
            if(color_mode==POLYGON_FILL_COLOR_PER_ROW) { plot_color++; if(plot_color>=color_count) plot_color = FIRST_CYCLING_COLOR; }
            draw_y++;
        }
    }
    if(flags&SHAPE_OUTLINE) {
        plot_color = (uint16_t)(edge_color);
        if(flags&POLYGON_EDGE_COLOR_PER_POINT) color_mode=1;
        else color_mode=0;
        for(index=0;index<vertex_count;index++) {
            draw_x = (int16_t)(vertex_x[index]+polygon_origin_x); draw_y = (int16_t)(vertex_y[index]+polygon_origin_y);
            end_x = (int16_t)(vertex_x[index+1]+polygon_origin_x); end_y = (int16_t)(vertex_y[index+1]+polygon_origin_y); line_done=1;
            do {
                line_done=line_step(end_x,end_y,line_done);
                (*plot_callback)(draw_symmetry);
                if(color_mode) { plot_color++; if(plot_color>=color_count) plot_color = FIRST_CYCLING_COLOR; }
                image_work_count++;
                if(palette_elapsed_ticks>=palette_cycle_ticks) cycle_palette();
            } while(!line_done);
        }
    }
    vertex_x[0]=polygon_origin_x; vertex_y[0]=polygon_origin_y;
}

/* Initialize or advance a midpoint ellipse quadrant in draw_x/draw_y;
 * radius_x == 0 derives an aspect-corrected radius. Returns 1 at completion
 * without plotting; squared radii and decision state persist between calls.
 * Original 066A:05D8, 707 bytes. */
int16_t ellipse_step(int16_t radius_y,int16_t radius_x,int16_t initialize) {
    float aspect_factor;
    if(initialize) {
        ellipse_step_radius_y=radius_y;
        if(radius_x) ellipse_step_radius_x=radius_x;
        else {
            aspect_factor = (float)((double)screen_half_width/screen_half_height*0.75);
            ellipse_step_radius_x=(int32_t)((double)ellipse_step_radius_y*aspect_factor);
            if((double)ellipse_step_radius_y*aspect_factor-(double)ellipse_step_radius_x>0.5) ellipse_step_radius_x++;
        }
        draw_y = (int16_t)(radius_y);
        draw_x = 0;
        ellipse_step_radius_x_sq=ellipse_step_radius_x*ellipse_step_radius_x;
        ellipse_step_radius_y_sq=ellipse_step_radius_y*ellipse_step_radius_y;
        ellipse_step_twice_x_sq=ellipse_step_radius_x_sq*2L;
        ellipse_step_twice_y_sq=ellipse_step_radius_y_sq*2L;
        ellipse_step_gradient_x=0;
        ellipse_step_gradient_y=ellipse_step_twice_x_sq*ellipse_step_radius_y;
        ellipse_step_error=asr32(ellipse_step_radius_x_sq,2)-(ellipse_step_radius_x_sq*ellipse_step_radius_y);
        ellipse_first_region=1;
        if(radius_y==0) return 1; else return 0;
    } else {
        if(ellipse_first_region) {
            ellipse_step_error+=ellipse_step_gradient_x+ellipse_step_radius_y_sq;
            ellipse_step_gradient_x+=ellipse_step_twice_y_sq;
            draw_x++;
            if(ellipse_step_error>0) {
                draw_y--;
                ellipse_step_gradient_y-=ellipse_step_twice_x_sq;
                ellipse_step_error-=ellipse_step_gradient_y;
            }
            /* Switch from X-major to Y-major steps as the tangent steepens. */
            if(ellipse_step_gradient_y<=ellipse_step_gradient_x) {
                ellipse_step_error+=asr32(ellipse_step_gradient_x+ellipse_step_gradient_y,1)+asr32(ellipse_step_radius_y_sq-ellipse_step_radius_x_sq,2);
                ellipse_first_region=0;
            }
            return 0;
        } else {
            ellipse_step_error+=-ellipse_step_gradient_y+ellipse_step_radius_x_sq;
            ellipse_step_gradient_y-=ellipse_step_twice_x_sq;
            draw_y--;
            if(ellipse_step_error<0) {
                ellipse_step_gradient_x+=ellipse_step_twice_y_sq;
                ellipse_step_error+=ellipse_step_gradient_x;
                draw_x++;
            }
            if(draw_y==0) return 1;
        }
    }
    return 0;
}

/* Cache quadrant step directions on rebuild, then replay them as filled
 * spans and/or four reflected outlines about center_x/center_y. Flags select
 * coloring; plot_callback supplies the current drawing symmetry.
 * Original 066A:089B, 778 bytes. */
void draw_ellipse(int16_t rebuild,int16_t radius_y,int16_t radius_x,int16_t center_x,int16_t center_y,uint16_t fill_color,uint16_t edge_color,uint16_t flags) {
    if(rebuild) {
        draw_y = (int16_t)(radius_y); ellipse_trace_y=radius_y;
        ellipse_trace_x=draw_x=ellipse_trace_index=0;
        ellipse_trace_done=1;
        do {
            ellipse_trace_done=ellipse_step(radius_y,radius_x,ellipse_trace_done);
            /* Cache horizontal (0), diagonal (1), or vertical (-1) steps. */
            if(draw_y==ellipse_trace_y) (*ellipse_direction(ellipse_trace_index))=0;
            else if(draw_x>ellipse_trace_x) (*ellipse_direction(ellipse_trace_index))=1;
            else (*ellipse_direction(ellipse_trace_index))=-1;
            ellipse_trace_y=draw_y; ellipse_trace_x=draw_x; ellipse_trace_index++;
            if(palette_elapsed_ticks>=palette_cycle_ticks) cycle_palette();
        } while(ellipse_trace_done==0);
        ellipse_trace_limit=ellipse_trace_index;
    }
    if(flags&SHAPE_FILL) {
        plot_color = (uint16_t)(fill_color); ellipse_fill_color = (int16_t)(fill_color);
        ellipse_fill_mode=flags&ELLIPSE_FILL_COLOR_MASK;
        ellipse_trace_index=1; ellipse_trace_x=0; ellipse_trace_y=radius_y;
        while(ellipse_trace_index<=ellipse_trace_limit) {
            while((*ellipse_direction(ellipse_trace_index))==0 && ellipse_trace_index<ellipse_trace_limit) {
                ellipse_trace_x++;
                ellipse_trace_index++;
            }
            ellipse_fill_x=ellipse_trace_x-1;
            if(ellipse_fill_mode==ELLIPSE_FILL_MIRRORED_COLORS) ellipse_fill_min_x=0;
            else ellipse_fill_min_x=-ellipse_fill_x;
            if(ellipse_fill_mode&ELLIPSE_FILL_HORIZONTAL_COLOR_MASK) plot_color = (uint16_t)(ellipse_fill_color);
            do {
                draw_x = (int16_t)(center_x+ellipse_fill_x); draw_y = (int16_t)(center_y+ellipse_trace_y); (*plot_callback)(draw_symmetry);
                draw_x = (int16_t)(center_x+ellipse_fill_x); draw_y = (int16_t)(center_y-ellipse_trace_y); (*plot_callback)(1);
                if(ellipse_fill_mode==ELLIPSE_FILL_MIRRORED_COLORS) {
                    draw_x = (int16_t)(center_x-ellipse_fill_x); draw_y = (int16_t)(center_y-ellipse_trace_y); (*plot_callback)(draw_symmetry);
                    draw_x = (int16_t)(center_x-ellipse_fill_x); draw_y = (int16_t)(center_y+ellipse_trace_y); (*plot_callback)(draw_symmetry);
                }
                if(ellipse_fill_mode&ELLIPSE_FILL_HORIZONTAL_COLOR_MASK) {
                    plot_color++;
                    if(plot_color>=color_count) plot_color = FIRST_CYCLING_COLOR;
                }
                image_work_count++;
                if(palette_elapsed_ticks>=palette_cycle_ticks) cycle_palette();
                ellipse_fill_x--;
            } while(ellipse_fill_x>=ellipse_fill_min_x);
            if(ellipse_fill_mode==ELLIPSE_FILL_COLOR_PER_ROW) {
                plot_color++;
                if(plot_color>=color_count) plot_color = FIRST_CYCLING_COLOR;
            }
            if((*ellipse_direction(ellipse_trace_index))>0) ellipse_trace_x++;
            ellipse_trace_y--;
            ellipse_trace_index++;
        }
    }
    if(flags&SHAPE_OUTLINE) {
        plot_color = (uint16_t)(edge_color);
        ellipse_trace_index=1; ellipse_trace_x=0; ellipse_trace_y=radius_y;
        while(ellipse_trace_index<=ellipse_trace_limit) {
            draw_x = (int16_t)(center_x+ellipse_trace_x); draw_y = (int16_t)(center_y+ellipse_trace_y); (*plot_callback)(draw_symmetry);
            draw_x = (int16_t)(center_x+ellipse_trace_x); draw_y = (int16_t)(center_y-ellipse_trace_y); (*plot_callback)(draw_symmetry);
            draw_x = (int16_t)(center_x-ellipse_trace_x); draw_y = (int16_t)(center_y-ellipse_trace_y); (*plot_callback)(draw_symmetry);
            draw_x = (int16_t)(center_x-ellipse_trace_x); draw_y = (int16_t)(center_y+ellipse_trace_y); (*plot_callback)(draw_symmetry);
            if(flags&ELLIPSE_EDGE_COLOR_PER_POINT) {
                plot_color++;
                if(plot_color>=color_count) plot_color = FIRST_CYCLING_COLOR;
            }
            image_work_count++;
            if((*ellipse_direction(ellipse_trace_index))==0) ellipse_trace_x++;
            else {
                if((*ellipse_direction(ellipse_trace_index))>0) ellipse_trace_x++;
                ellipse_trace_y--;
            }
            ellipse_trace_index++;
            if(palette_elapsed_ticks>=palette_cycle_ticks) cycle_palette();
        }
    }
}

/* Swap two pixel rectangles: flip_flags & 1 reverses the second's rows,
 * and flip_flags >= 2 reverses its columns. Enhanced EGA uses two-unit rows;
 * palette and input work can interrupt the swap between rows.
 * Original 066A:0BA5, 296 bytes. */
void swap_pixel_rectangles(int16_t flip_flags,int16_t first_x,int16_t first_y,int16_t second_x,int16_t second_y,int16_t width,int16_t height) {
    if (dz_hybrid_enabled()) dz_hybrid_retire();
    volatile int16_t row;
    int16_t first_color,second_color,first_pixel_x,first_pixel_y,second_pixel_x,second_pixel_y,row_step,column;
    width--;
    row_step=1;
    if(video_backend == BACKEND_EGA_640X700) {
        first_y=(int16_t)(dz_asr16(first_y,1)*2);
        second_y=(int16_t)(dz_asr16(second_y,1)*2);
        height=(int16_t)(dz_asr16((int16_t)(height-2),1)*2);
        row_step=2;
    } else height--;
    for(row=0;row<=height;row+=row_step) {
        first_pixel_y=first_y+row;
        if(flip_flags&1) second_pixel_y = (int16_t)(second_y+height-row);
        else second_pixel_y = (int16_t)(second_y+row);
        for(column=0;column<=width;column++) {
            first_pixel_x=first_x+column;
            if(flip_flags<2) second_pixel_x = (int16_t)(second_x+column);
            else second_pixel_x = (int16_t)(second_x+width-column);
            first_color=(uint8_t)(*read_pixel_callback)(first_pixel_x,first_pixel_y);
            second_color=(uint8_t)(*read_pixel_callback)(second_pixel_x,second_pixel_y);
            plot_color = (uint16_t)(second_color); draw_x = (int16_t)(first_pixel_x); draw_y = (int16_t)(first_pixel_y); (*plot_callback)(1);
            plot_color = (uint16_t)(first_color); draw_x = (int16_t)(second_pixel_x); draw_y = (int16_t)(second_pixel_y); (*plot_callback)(1);
        }
        if(palette_elapsed_ticks>=palette_cycle_ticks) cycle_palette();
        if(input_pending || exit_requested) if(poll_graphics_keys()) break;
        if(current_graphics_key==COMMAND_CLEAR) break;
    }
}

/* Draw two shrinking families of ellipse outlines centered on the Y and X
 * axes. rebuild resets radii from the screen dimensions; each call reduces
 * the first vertical radius and the second horizontal radius by eight.
 * Original 066A:0CCD, 153 bytes. */
void arches(int16_t rebuild) {
    if(rebuild) {
        arches_first_center_y=arches_first_radius_y=arches_second_radius_y=screen_half_height-1;
        arches_second_center_x=arches_first_radius_x=arches_second_radius_x=screen_half_width-1;
        if(dz_random(2)) { arches_first_radius_y<<=1; arches_second_radius_x<<=1; }
    }
    draw_ellipse(1,arches_first_radius_y,arches_first_radius_x,0,arches_first_center_y,0,FIRST_CYCLING_COLOR,0x21);
    draw_ellipse(1,arches_second_radius_y,arches_second_radius_x,arches_second_center_x,0,0,FIRST_CYCLING_COLOR,0x21);
    arches_first_radius_y-=8; arches_second_radius_x-=8;
}

/* Generate shrinking circular sweeps, extending each computed point into
 * a sixteen-call horizontal stroke. rebuild resets the radius and color;
 * completed sweeps change color, and EGA halves the generated Y coordinate.
 * Original 066A:0D66, 304 bytes. */
void atom(int16_t rebuild) {
    float angle;
    if(rebuild || atom_radius<=1) {
        atom_stroke_index=0xff;
        atom_phase = 0;
        atom_radius=screen_extent/2;
        if(rebuild) plot_color = (uint16_t)(FIRST_CYCLING_COLOR&color_index_mask);
    }
    atom_stroke_index++;
    if((atom_stroke_index=atom_stroke_index&15)==0) {
        atom_phase=(int16_t)(atom_phase+screen_extent/2-atom_radius+45);
        if(atom_phase>18*screen_extent || atom_phase<0) {
            atom_radius-=screen_extent/20;
            atom_phase = 0;
            plot_color++;
            if(plot_color>=color_count) plot_color = FIRST_CYCLING_COLOR;
        }
        angle = (float)(atom_phase*0.0002);
        draw_x = (int16_t)(atom_radius*cos(angle));
        draw_y = (int16_t)(atom_radius*sin(angle));
        if(video_adapter==ADAPTER_EGA) draw_y/=2;
    } else draw_x++;
}

/* Sample nonzero pixels from the shaded-ball bitmap with an expanding
 * cardinal walk. rebuild starts at a random origin; effect_parameter chooses
 * the sampling stride and coordinate divisor. The caller plots each sample.
 * Original 066A:0E96, 416 bytes. */
void balls(int16_t rebuild) {
    int16_t pixel;
    if(rebuild || (balls_sample_x>=ball_bitmap_width && balls_sample_y>=ball_bitmap_height)) {
        balls_step_x=balls_step_y=balls_run_length=0;
        balls_run_progress=-1; balls_direction=5;
        balls_origin_y = (int16_t)(dz_random((uint16_t)(screen_half_height+(ball_bitmap_height>>1)))-ball_bitmap_height);
        balls_origin_x = (int16_t)(dz_random((uint16_t)(screen_half_width+(ball_bitmap_width>>1)))-ball_bitmap_width);
        balls_sample_x = (uint16_t)(ball_bitmap_width>>1); balls_sample_y = (uint16_t)(ball_bitmap_height>>1);
        effect_parameter=dz_random(3)+1;
    }
    do {
        if(balls_run_progress++>=balls_run_length) {
            balls_run_progress=1;
            if(++balls_direction>3) {
                balls_run_length+=2;
                balls_direction=0;
                balls_sample_x = (uint16_t)(balls_sample_x+effect_parameter);
                balls_sample_y = (uint16_t)(balls_sample_y-effect_parameter);
            }
            balls_step_x=ELEMENT(direction_x, balls_direction)*effect_parameter;
            balls_step_y=ELEMENT(direction_y, balls_direction)*effect_parameter;
        }
        balls_sample_x=(uint16_t)(balls_sample_x+balls_step_x);
        balls_sample_y=(uint16_t)(balls_sample_y+balls_step_y);
        /* The DOS fetch precedes the coordinate test. The reference driver
         * provides zero padding for the wrapped 64 KiB offset domain. */
        uint16_t offset=(uint16_t)((uint32_t)balls_sample_x*ball_bitmap_height+balls_sample_y);
        if (!ball_bitmap_pixels) return;
        pixel=offset < (uint32_t)ball_bitmap_width*ball_bitmap_height
            ? ball_bitmap_pixels[offset] : 0;
    } while(pixel==0);
    if(balls_sample_x>=ball_bitmap_width || balls_sample_y>=ball_bitmap_height) return;
    plot_color = (uint16_t)(pixel&color_index_mask);
    draw_x = (int16_t)(balls_origin_x+balls_sample_x/effect_parameter);
    draw_y = (int16_t)(balls_origin_y+balls_sample_y/effect_parameter);
}

/* Generate parallel line segments with a random fixed X/Y span. rebuild
 * chooses the origin and axis of motion; each completed line advances its
 * start and color. The caller plots the shared line iterator's pen.
 * Original 066A:1036, 373 bytes. */
void bars(int16_t rebuild) {
    if(rebuild || draw_x>screen_extent || draw_y>screen_extent) {
        bars_start_x=(uint16_t)dz_random((int16_t)(screen_extent/2)); draw_x=(int16_t)bars_start_x;
        bars_start_y=(uint16_t)dz_random((int16_t)(screen_extent/2)); draw_y=(int16_t)bars_start_y;
        bars_span_x = (uint16_t)(dz_random((int16_t)(screen_extent/6))+screen_extent/6);
        bars_span_y = (uint16_t)(dz_random((int16_t)(screen_extent/6))+screen_extent/6);
        bars_line_done = 1;
        bars_end_x=bars_start_x+bars_span_x; bars_end_y=bars_start_y+bars_span_y;
        effect_parameter=dz_random(2);
    }
    bars_line_done = (uint16_t)line_step((int16_t)bars_end_x,(int16_t)bars_end_y,(int16_t)bars_line_done);
    if(bars_line_done) {
        choose_color(0);
        draw_x = (int16_t)(bars_start_x); draw_y = (int16_t)(bars_start_y);
        if(effect_parameter) bars_start_x++;
        else bars_start_y++;
        bars_end_x=bars_start_x+bars_span_x; bars_end_y=bars_start_y+bars_span_y;
    }
}

/* Cache two seed-line traces and join their points to axis/diagonal targets
 * to generate star layers through the caller's symmetry. rebuild starts
 * nested halving of the geometry; state selects traces and color progression.
 * Original 066A:11AB, 1161 bytes. */
void bigstar(int16_t rebuild) {
    float seed_slope;
    int16_t end_x,end_y;
    if(rebuild || bigstar_restart) {
        bigstar_restart=0;
        bigstar_axis_y = (int16_t)((screen_half_height-1)*1.5);
        bigstar_axis_x = (int16_t)((screen_half_width-1)*1.25);
        bigstar_diagonal_y = (int16_t)(bigstar_axis_y*0.707);
        bigstar_diagonal_x = (int16_t)(bigstar_axis_x*0.707);
        bigstar_inner_radius = (float)((int16_t)((dz_random(3)+3)*bigstar_axis_y)*0.1);
        bigstar_spacing_shift=2;
        bigstar_resize_pending=1;
    }
    if(bigstar_inner_radius>4.0) {
        if(bigstar_resize_pending) {
            bigstar_resize_pending=0;
            bigstar_axis_x = (int16_t)(dz_asr16(bigstar_axis_x,1)); bigstar_axis_y = (int16_t)(dz_asr16(bigstar_axis_y,1)); bigstar_diagonal_x = (int16_t)(dz_asr16(bigstar_diagonal_x,1)); bigstar_diagonal_y = (int16_t)(dz_asr16(bigstar_diagonal_y,1));
            bigstar_inner_radius = (float)(bigstar_inner_radius*0.5);
            bigstar_target_x=(int16_t)(screen_half_width*(double)bigstar_inner_radius/screen_half_height*0.75);
            bigstar_target_y = (int16_t)(bigstar_inner_radius);
            seed_slope = (float)((dz_random(3)+1)*0.25);
            if(--bigstar_spacing_shift<0) bigstar_spacing_shift=0;
            bigstar_trace_step=1<<bigstar_spacing_shift;
            bigstar_trace_index=0;
            bigstar_segment_state=1;
            if(video_adapter!=ADAPTER_EGA) bigstar_color_mode=dz_random(2); else bigstar_color_mode=2;
            bigstar_base_color=dz_random((uint16_t)(color_count-1));
            plot_color = (uint16_t)(bigstar_base_color&color_index_mask);
            /* Rebuild steep and shallow seed traces at the current star scale. */
            end_x = (int16_t)(bigstar_target_x*(double)seed_slope);
            draw_x=draw_y=bigstar_steep_count=0;
            bigstar_line_done=1;
            do {
                bigstar_line_done=line_step(end_x,bigstar_target_y,bigstar_line_done);
                ELEMENT(bigstar_traces, bigstar_steep_count).x1=draw_x;
                ELEMENT(bigstar_traces, bigstar_steep_count).y1=draw_y;
                bigstar_steep_count++;
                if(palette_elapsed_ticks>=palette_cycle_ticks) cycle_palette();
            } while(!bigstar_line_done);
            end_y = (int16_t)(bigstar_target_y*(double)seed_slope);
            draw_x=draw_y=bigstar_shallow_count=0;
            bigstar_line_done=1;
            do {
                bigstar_line_done=line_step(bigstar_target_x,end_y,bigstar_line_done);
                ELEMENT(bigstar_traces, bigstar_shallow_count).x2=draw_x;
                ELEMENT(bigstar_traces, bigstar_shallow_count).y2=draw_y;
                bigstar_shallow_count++;
                if(palette_elapsed_ticks>=palette_cycle_ticks) cycle_palette();
            } while(!bigstar_line_done);
            bigstar_line_done=1;
        }
        if(bigstar_line_done) {
            /* Each completed ray selects the next target and cached seed point. */
            if(bigstar_trace_index>=bigstar_steep_count) bigstar_segment_state|=2;
            if(bigstar_color_mode==1) plot_color = (uint16_t)(bigstar_base_color&color_index_mask);
            switch(bigstar_segment_state) {
            case 1:
                bigstar_target_x=0; bigstar_target_y = (int16_t)(bigstar_axis_y); bigstar_segment_state=2; bigstar_use_steep_trace=1;
                break;
            case 0: case 2:
                bigstar_target_x=bigstar_diagonal_x; bigstar_target_y = (int16_t)(bigstar_diagonal_y); bigstar_segment_state=3;
                break;
            case 3:
                bigstar_target_x=bigstar_diagonal_x; bigstar_target_y = (int16_t)(bigstar_diagonal_y); bigstar_segment_state=4; bigstar_use_steep_trace=0;
                break;
            case 4: case 6:
                bigstar_target_x=bigstar_axis_x; bigstar_target_y = 0; bigstar_segment_state=1;
                break;
            }
            if(bigstar_use_steep_trace) {
                draw_x = (int16_t)(ELEMENT(bigstar_traces, bigstar_trace_index).x1); draw_y = (int16_t)(ELEMENT(bigstar_traces, bigstar_trace_index).y1);
            } else {
                draw_x = (int16_t)(ELEMENT(bigstar_traces, bigstar_trace_index).x2); draw_y = (int16_t)(ELEMENT(bigstar_traces, bigstar_trace_index).y2);
            }
            if(bigstar_trace_index>=bigstar_shallow_count) bigstar_segment_state&=1;
            if(bigstar_segment_state==1) {
                bigstar_trace_index+=bigstar_trace_step;
                if(bigstar_color_mode==0) { plot_color++; if(plot_color>=color_count) plot_color = FIRST_CYCLING_COLOR; }
                if(bigstar_color_mode==2) {
                    ega_color_substep++;
                    if((ega_color_substep&=3)==0) { plot_color++; if(plot_color>=color_count) plot_color = FIRST_CYCLING_COLOR; }
                }
            }
            if(bigstar_trace_index>=bigstar_steep_count && bigstar_trace_index>=bigstar_shallow_count) bigstar_resize_pending=1;
        }
        bigstar_line_done=line_step(bigstar_target_x,bigstar_target_y,bigstar_line_done);
        if(bigstar_color_mode==1) { plot_color++; if(plot_color>=color_count) plot_color = FIRST_CYCLING_COLOR; }
        image_work_count=1;
    } else {
        image_work_count=0x7d00;
        bigstar_restart=1;
    }
}

/* Generate a fan of lines from a fixed origin to a rectangular perimeter.
 * rebuild chooses a new box; each completed line advances the target, and
 * finishing the perimeter requests another box. The caller plots each point.
 * Original 066A:1634, 427 bytes. */
void boxes(int16_t rebuild) {
    if(rebuild || boxes_restart) {
        boxes_target_x=dz_random((int16_t)(screen_extent/3));
        boxes_target_y=dz_random((int16_t)(screen_extent/3));
        boxes_origin_x=boxes_target_x; boxes_origin_y=boxes_target_y;
        boxes_width=dz_random((int16_t)(screen_extent/4))+screen_extent/4;
        boxes_height=dz_random((int16_t)(screen_extent/4))+screen_extent/4;
        boxes_line_done=1; boxes_restart=0; boxes_edge_state=1; boxes_edge_progress=0;
        draw_x = (int16_t)(boxes_origin_x); draw_y = (int16_t)(boxes_origin_y);
        plot_color = (uint16_t)(FIRST_CYCLING_COLOR&color_index_mask);
    }
    boxes_line_done=line_step(boxes_target_x,boxes_target_y,boxes_line_done);
    choose_color(0);
    if(boxes_line_done) {
        plot_color = (uint16_t)(FIRST_CYCLING_COLOR&color_index_mask);
        draw_x = (int16_t)(boxes_origin_x); draw_y = (int16_t)(boxes_origin_y);
        boxes_edge_progress++;
        switch(boxes_edge_state-1) {
        case 0:
            boxes_target_x+=boxes_width;
            boxes_edge_progress=0;
            boxes_edge_state++;
            break;
        case 1:
            boxes_target_y++;
            if(boxes_edge_progress>=boxes_height) { boxes_edge_progress=0; boxes_edge_state++; }
            break;
        case 2:
            boxes_target_x--;
            if(boxes_edge_progress>=boxes_width) { boxes_edge_progress=0; boxes_edge_state++; }
            break;
        case 3:
            boxes_target_y-=boxes_height;
            boxes_edge_progress=0;
            boxes_edge_state++;
            break;
        default:
            boxes_restart=1;
            break;
        }
    }
}

/* Draw a complete stack of growing ellipse outlines with a moving center
 * and cycling color. Random corner and direction variants form a cone;
 * rebuild is unused, and every call chooses a fresh stack.
 * Original 066A:17DF, 467 bytes. */
void cones(int16_t rebuild) {
    int16_t radius_y,radius_x,center_step_x,center_step_y,corner_index,radius_limit;
    int16_t center_x,center_y,direction_variant;
    (void)rebuild;
    plot_color = (uint16_t)(dz_random((uint16_t)(color_count)));
    center_x=dz_random((int16_t)(screen_half_width/2));
    center_y=dz_random((int16_t)(screen_half_height/2));
    corner_index=dz_random(4);
    direction_variant=dz_random(3);
    switch(corner_index) {
    case 0:
        center_x=screen_half_width-center_x;
        break;
    case 1:
        center_x=screen_half_width-center_x;
        /* Fall through: the corner orientation also reverses Y. */
    case 2:
        center_y=screen_half_height-center_y;
        break;
    }
    center_step_x=ELEMENT(cone_directions, corner_index).dx[direction_variant];
    center_step_y=ELEMENT(cone_directions, corner_index).dy[direction_variant];
    if(direction_variant && dz_random(2)) { center_step_x=dz_asr16(center_step_x,1); center_step_y=dz_asr16(center_step_y,1); }
    radius_y=0; radius_x=0;
    radius_limit=screen_half_height/16+dz_random((int16_t)(screen_half_height/8));
    while(radius_y<radius_limit) {
        draw_ellipse(1,radius_y,radius_x,center_x,center_y,0,plot_color,SHAPE_OUTLINE);
        plot_color++;
        if(plot_color>=color_count) plot_color = FIRST_CYCLING_COLOR;
        center_x+=center_step_x; center_y+=center_step_y;
        radius_x++; radius_y++;
    }
}

/* Generate an expanding diagonal corkscrew by combining growing along-axis
 * and sinusoidal cross-axis offsets. rebuild chooses an origin and growth
 * rate; phase advances each call and color advances every sixteen calls.
 * Original 066A:19B2, 432 bytes. */
void corkscrew(int16_t rebuild) {
    if(rebuild || draw_x>screen_extent) {
        corkscrew_phase=0; corkscrew_distance=0;
        corkscrew_growth_step = (float)((dz_random(2)+1)*0.02);
        corkscrew_origin_x=dz_random((int16_t)(screen_extent/2));
        corkscrew_origin_y=dz_random((int16_t)(screen_extent/2));
        plot_color++;
        if(plot_color>=color_count) plot_color = FIRST_CYCLING_COLOR;
    }
    corkscrew_along = (float)(corkscrew_distance*(1-cos(corkscrew_phase)/4));
    corkscrew_across = (float)(((double)corkscrew_distance*corkscrew_distance/screen_extent)*sin(corkscrew_phase));
    draw_x = (int16_t)(corkscrew_origin_x+(int16_t)((double)corkscrew_along-corkscrew_across));
    draw_y = (int16_t)(corkscrew_origin_y+(int16_t)((double)corkscrew_along+corkscrew_across));
    corkscrew_phase=(float)(corkscrew_phase+0.02);
    corkscrew_distance=(float)((double)corkscrew_distance+corkscrew_growth_step);
    corkscrew_color_counter++;
    if((corkscrew_color_counter=corkscrew_color_counter&15)==0) {
        plot_color++;
        if(plot_color>=color_count) plot_color = FIRST_CYCLING_COLOR;
    }
}

/* Generate a sine/cosine curve with independent phase steps and a shared
 * amplitude. rebuild randomizes the steps; each computed point is extended
 * into a sixteen-call horizontal stroke, with Y-dependent color changes.
 * Original 066A:1B62, 352 bytes. */
void curves(int16_t rebuild) {
    float angle_x,angle_y;
    if(rebuild) {
        curves_phase_x=(curves_phase_y=1);
        curves_amplitude = (int16_t)(screen_extent/1.5);
        curves_stroke_index=255;
        curves_phase_step_x=dz_random(8)+2;
        curves_phase_step_y=dz_random(6)+4;
    }
    curves_stroke_index++;
    if((curves_stroke_index=curves_stroke_index&15)==0) {
        angle_x = (float)(curves_phase_x*0.001);
        angle_y = (float)(curves_phase_y*0.001);
        draw_x = (int16_t)((int16_t)(cos(angle_x)*curves_amplitude)+4);
        draw_y = (int16_t)((int16_t)(sin(angle_y)*curves_amplitude)+4);
        curves_phase_x+=curves_phase_step_x;
        curves_phase_y+=curves_phase_step_y;
        if(curves_phase_x>32000) curves_phase_x=0;
        if(curves_phase_y>32000) curves_phase_y=0;
        if((draw_y&2) && ++plot_color>=color_count) plot_color = FIRST_CYCLING_COLOR;
    } else {
        draw_x++;
    }
}

/* Draw a complete rolling-circle curve by joining orbit-plus-pen samples
 * with raster lines. rebuild randomizes the radii and color mode; successive
 * curves vary the pen radius and starting phase until the batch is complete.
 * Original 066A:1CC2, 1107 bytes. */
void cycloids(int16_t rebuild) {
    /* Rebuild or next-curve setup assigns both before the first read. Zero is
     * also defined for an isolated non-rebuild call on invalid external state. */
    int16_t end_x=0,end_y = 0;
    /* Hybrid: explicit joins of the existing quantized endpoints. */
    dz_stroke_id stroke = dz_stroke_begin(DZ_STROKE_CYCLOIDS);
    if(rebuild || cycloids_curve_index>=cycloids_curve_limit) {
        /* X uses a 16.16 aspect scale; angles index the 64-sample trig table. */
        cycloids_aspect_scale=((int32_t)screen_half_width*65536L/screen_half_height)*3L/4L;
        cycloids_outer_radius = (int16_t)(screen_half_height*3/4);
        cycloids_rolling_radius=dz_random((int16_t)(cycloids_outer_radius*5/16))+cycloids_outer_radius/16;
        cycloids_pen_radius=dz_random((int16_t)(cycloids_rolling_radius))+cycloids_rolling_radius/2;
        cycloids_orbit_radius=cycloids_outer_radius-cycloids_rolling_radius;
        cycloids_phase_advance=dz_random(5);
        cycloids_orbit_step=1;
        cycloids_spin_step = (int16_t)((cycloids_outer_radius/cycloids_rolling_radius)*cycloids_orbit_step);
        cycloids_pen_radius_step=cycloids_pen_radius/8;
        if(dz_random(2)) cycloids_pen_radius_step=-cycloids_pen_radius_step;
        cycloids_start_phase = 48;
        cycloids_orbit_phase = (int16_t)(cycloids_start_phase);
        cycloids_spin_phase = 0;
        cycloids_curve_index=0;
        end_x = (int16_t)((cycloids_pen_radius+cycloids_orbit_radius)*(double)ELEMENT(trig_table, cycloids_orbit_phase).cosine);
        end_x = (int16_t)(asr32(mul32(cycloids_aspect_scale,end_x),16));
        end_y = (int16_t)((cycloids_pen_radius+cycloids_orbit_radius)*(double)ELEMENT(trig_table, cycloids_orbit_phase).sine);
        cycloids_curve_limit=dz_random(10)+1;
        plot_color = (uint16_t)(dz_random((uint16_t)(color_count-1))&color_index_mask);
        cycloids_next_curve=0;
        cycloids_color_per_point=dz_random(2);
    }
    if(cycloids_next_curve) {
        cycloids_next_curve=0;
        if(cycloids_color_per_point==0) { plot_color++; if(plot_color>=color_count) plot_color = FIRST_CYCLING_COLOR; }
        cycloids_pen_radius+=cycloids_pen_radius_step;
        if(cycloids_pen_radius<0) cycloids_pen_radius=-cycloids_pen_radius;
        cycloids_start_phase = (int16_t)((cycloids_start_phase+cycloids_phase_advance)%64);
        cycloids_orbit_phase = (int16_t)(cycloids_start_phase);
        cycloids_spin_phase = 0;
        end_x = (int16_t)((cycloids_pen_radius+cycloids_orbit_radius)*(double)ELEMENT(trig_table, cycloids_orbit_phase).cosine);
        end_x = (int16_t)(asr32(mul32(cycloids_aspect_scale,end_x),16));
        end_y = (int16_t)((cycloids_pen_radius+cycloids_orbit_radius)*(double)ELEMENT(trig_table, cycloids_orbit_phase).sine);
        cycloids_curve_index++;
    }
    do {
        cycloids_orbit_phase = (int16_t)((cycloids_orbit_phase+cycloids_orbit_step)%64);
        cycloids_spin_phase = (int16_t)((cycloids_spin_phase+cycloids_spin_step)%64);
        cycloids_pen_phase = (int16_t)((cycloids_orbit_phase-cycloids_spin_phase)%64);
        if(cycloids_pen_phase<0) cycloids_pen_phase+=64;
        cycloids_center_x = (int16_t)(cycloids_orbit_radius*(double)ELEMENT(trig_table, cycloids_orbit_phase).cosine);
        cycloids_center_y = (int16_t)(cycloids_orbit_radius*(double)ELEMENT(trig_table, cycloids_orbit_phase).sine);
        cycloids_pen_x = (int16_t)(cycloids_pen_radius*(double)ELEMENT(trig_table, cycloids_pen_phase).cosine);
        cycloids_pen_y = (int16_t)(cycloids_pen_radius*(double)ELEMENT(trig_table, cycloids_pen_phase).sine);
        draw_x = (int16_t)(end_x); draw_y = (int16_t)(end_y);
        end_x = (int16_t)(asr32(mul32(cycloids_aspect_scale,(int16_t)(cycloids_center_x+cycloids_pen_x)),16));
        end_y = (int16_t)(cycloids_center_y+cycloids_pen_y);
        double stroke_x = draw_x, stroke_y = draw_y;
        cycloids_line_done=1;
        do {
            cycloids_line_done=line_step(end_x,end_y,cycloids_line_done);
            dz_stroke_line(stroke, stroke_x, stroke_y, end_x, end_y, draw_x, draw_y);
            (*plot_callback)(0);
            image_work_count++;
            if(cycloids_color_per_point) { plot_color++; if(plot_color>=color_count) plot_color = FIRST_CYCLING_COLOR; }
            if(palette_elapsed_ticks>=palette_cycle_ticks) cycle_palette();
        } while(!cycloids_line_done);
        /* Close only when both orbit and pen phases return to the start. */
    } while(cycloids_orbit_phase!=cycloids_start_phase || cycloids_pen_phase!=cycloids_start_phase);
    dz_stroke_break(stroke);
    cycloids_next_curve=1;
    draw_x=draw_y = (int16_t)(screen_extent+1);
}

/* Cache the edge between angular samples, then draw four reflected ray
 * fans about a random center. rebuild chooses radii, sample range and color
 * mode; subsequent calls alternate radii while advancing around the arc.
 * Original 066A:2115, 1238 bytes. */
void daisies(int16_t rebuild) {
    if(rebuild || daisies_sample_index>=daisies_sample_limit) {
        daisies_aspect_scale=((int32_t)screen_half_width*65536L/screen_half_height)*3L/4L;
        daisies_center_x=dz_random((int16_t)(screen_extent/2))+screen_extent/4;
        daisies_center_y=dz_random((int16_t)(screen_extent/2))+screen_extent/4;
        daisies_radius=dz_random((int16_t)(screen_extent/3))+screen_extent/6;
        daisies_radius_step=daisies_radius>>(dz_random(3)+1);
        daisies_sample_step=1;
        if(dz_random(3)) { daisies_sample_start=0; daisies_sample_limit=9; }
        else {
            daisies_arc_variant=dz_random(2);
            daisies_sample_start=daisies_arc_variant+2;
            daisies_sample_limit=7-daisies_arc_variant;
            if(daisies_arc_variant==0) daisies_sample_step+=dz_random(2);
        }
        daisies_color_per_ray=dz_random(2);
        daisies_base_color=dz_random((uint16_t)(color_count));
        for(daisies_work_index=0;daisies_work_index<=9;daisies_work_index++) {
            if(daisies_work_index>1 && daisies_work_index<9) daisies_trig_index = (int16_t)(daisies_work_index*2-1);
            else if(daisies_work_index<=1) daisies_trig_index = (int16_t)(daisies_work_index);
            else daisies_trig_index = 16;
            ELEMENT(daisies_cosine_table, daisies_work_index)=(int32_t)(ELEMENT(trig_table, daisies_trig_index).cosine*65536.0);
            ELEMENT(daisies_sine_table, daisies_work_index)=(int32_t)(ELEMENT(trig_table, daisies_trig_index).sine*65536.0);
        }
        daisies_sample_index=daisies_sample_start;
    }
    /* Cache the raster edge between angular samples as ray endpoints. */
    draw_x = (int16_t)(asr32(mul32(ELEMENT(daisies_sine_table, daisies_sample_index),daisies_radius),16));
    draw_y = (int16_t)(asr32(mul32(ELEMENT(daisies_cosine_table, daisies_sample_index),daisies_radius),16));
    daisies_sample_index+=daisies_sample_step;
    daisies_edge_end_x = (int16_t)(asr32(mul32(ELEMENT(daisies_sine_table, daisies_sample_index),daisies_radius),16));
    daisies_edge_end_y = (int16_t)(asr32(mul32(ELEMENT(daisies_cosine_table, daisies_sample_index),daisies_radius),16));
    daisies_edge_count=0;
    daisies_line_done=1;
    do {
        daisies_line_done=line_step(daisies_edge_end_x,daisies_edge_end_y,daisies_line_done);
        ELEMENT(daisies_edge_points, daisies_edge_count).x=(int16_t)asr32(mul32(daisies_aspect_scale,draw_x),16);
        ELEMENT(daisies_edge_points, daisies_edge_count).y=draw_y;
        daisies_edge_count++;
        if(palette_elapsed_ticks>=palette_cycle_ticks) cycle_palette();
    } while(!daisies_line_done);
    for(daisies_work_index=0;daisies_work_index<daisies_edge_count;daisies_work_index++) {
        draw_x = (int16_t)(ELEMENT(daisies_edge_points, daisies_work_index).x);
        draw_y = (int16_t)(ELEMENT(daisies_edge_points, daisies_work_index).y);
        daisies_line_done=1;
        if(daisies_color_per_ray) { plot_color++; if(plot_color>=color_count) plot_color = FIRST_CYCLING_COLOR; }
        else plot_color = (uint16_t)(daisies_base_color&color_index_mask);
        do {
            daisies_line_done=line_step(0,0,daisies_line_done);
            daisies_saved_x=draw_x; daisies_saved_y=draw_y;
            draw_x = (int16_t)(daisies_center_x+daisies_saved_x); draw_y = (int16_t)(daisies_center_y+daisies_saved_y); (*plot_callback)(1);
            draw_x = (int16_t)(daisies_center_x-daisies_saved_x); draw_y = (int16_t)(daisies_center_y-daisies_saved_y); (*plot_callback)(1);
            draw_x = (int16_t)(daisies_center_x+daisies_saved_x); draw_y = (int16_t)(daisies_center_y-daisies_saved_y); (*plot_callback)(1);
            draw_x = (int16_t)(daisies_center_x-daisies_saved_x); draw_y = (int16_t)(daisies_center_y+daisies_saved_y); (*plot_callback)(1);
            draw_x = (int16_t)(daisies_saved_x); draw_y = (int16_t)(daisies_saved_y);
            image_work_count++;
            if(daisies_color_per_ray==0) { plot_color++; if(plot_color>=color_count) plot_color = FIRST_CYCLING_COLOR; }
            if(palette_elapsed_ticks>=palette_cycle_ticks) cycle_palette();
        } while(!daisies_line_done);
    }
    daisies_radius_step=-daisies_radius_step;
    daisies_radius+=daisies_radius_step;
    draw_x = (int16_t)(daisies_center_x);
    draw_y = (int16_t)(daisies_center_y);
}

/* Generate descending diagonal points with coordinate wrapping. rebuild starts
 * a new color rule; effect_parameter selects coordinate coloring or a color
 * advance at each new diagonal. The caller plots draw_x/draw_y.
 * Original 066A:25EB, 172 bytes. */
void diag1(int16_t rebuild) {
    --draw_y;
    if (rebuild || draw_x++ > screen_extent - 1) {
        draw_x = 0;
        ++draw_y;
        if (rebuild) {
            effect_parameter = dz_random(9);
            if (effect_parameter == 1)
                --effect_parameter;
            if (dz_random(2) != 0)
                effect_parameter = 0;
        }
        if (effect_parameter == 0)
            choose_color(0);
    }
    if (draw_y < 0)
        draw_y = (int16_t)(screen_extent);
    if (effect_parameter != 0)
        choose_color(effect_parameter);
}

/* Generate rising diagonal points with coordinate wrapping. rebuild selects
 * either per-diagonal color changes or color runs whose period increases
 * from zero through twenty. The caller plots draw_x/draw_y.
 * Original 066A:2697, 185 bytes. */
void diag2(int16_t rebuild) {
    int16_t restart = rebuild;
    if (restart) {
        diag2_color_counter = diag2_color_period = 0;
        effect_parameter = dz_random(2);
    }
    ++draw_y;
    if (effect_parameter != 0) {
        if (draw_x++ >= screen_extent) {
            draw_x = 0;
            --draw_y;
            plot_color = (uint16_t)(4 & color_index_mask);
            if (++diag2_color_period > 20)
                diag2_color_period = 0;
        }
        if (diag2_color_counter++ > diag2_color_period) {
            diag2_color_counter = 0;
            choose_color(0);
        }
    } else if (restart || draw_x++ > screen_extent - 1) {
        draw_x = 0;
        --draw_y;
        choose_color(0);
    }
    if (draw_y > screen_extent)
        draw_y = 0;
}

/* Walk a random midpoint ellipse and draw center-to-boundary rays or
 * crossing diameters through each sample. rebuild resets radii, placement and
 * coloring; ellipse_region retains progress through the two decision regions.
 * Original 066A:2750, 1548 bytes. */
void ellipse(int16_t rebuild) {
    if (rebuild || (uint16_t)ellipse_region >= 2) {
        ellipse_region = 0;
        effect_parameter = dz_random(2);
        ellipse_draw_mode = dz_random(3);
        ellipse_offset_x = 0;
        if (ellipse_draw_mode == 0) {
            ellipse_center_x = 0;
            ellipse_center_y = 0;
        } else {
            ellipse_center_x = dz_random((int16_t)(screen_extent / 2)) + screen_extent / 4;
            ellipse_center_y = dz_random((int16_t)(screen_extent / 2)) + screen_extent / 4;
        }
        /* The int32_t accumulators below retain squared radii and midpoint error. */
        g_long5b42 = dz_random((int16_t)(screen_extent >> 1)) + (screen_extent >> 4);
        ellipse_offset_y = dz_random((int16_t)(screen_extent >> 1)) + (screen_extent >> 4);
        g_long5b46 = ellipse_offset_y;
        g_long5b4a = g_long5b42 * g_long5b42;
        g_long5b4e = g_long5b4a * 2;
        g_long5b52 = g_long5b46 * g_long5b46;
        g_long5b56 = g_long5b52 * 2;
        g_long5b36 = g_long5b52 - g_long5b4a * g_long5b46 + g_long5b4a / 4L;
        g_long5b3a = 0L;
        g_long5b3e = g_long5b4e * g_long5b46;
        plot_color = (uint16_t)(FIRST_CYCLING_COLOR & color_index_mask);
    }
    switch (ellipse_region) {
    case 0:
        if (g_long5b3a < g_long5b3e) {
            if (effect_parameter) {
                if (++plot_color >= color_count)
                    plot_color = FIRST_CYCLING_COLOR;
            } else {
                plot_color = (uint16_t)((ellipse_offset_x + ellipse_offset_y) & color_index_mask);
            }
            if ((uint16_t)ellipse_draw_mode < 2) {
                draw_x = (int16_t)(ellipse_center_x);
                draw_y = (int16_t)(ellipse_center_y);
                ellipse_line_done = 1;
                do {
                    ellipse_line_done = line_step(ellipse_center_x + ellipse_offset_x, ellipse_center_y + ellipse_offset_y, ellipse_line_done);
                    ++image_work_count;
                    plot_callback(1);
                    if (!effect_parameter && ++plot_color >= color_count)
                        plot_color = FIRST_CYCLING_COLOR;
                } while (!ellipse_line_done);
            } else {
                draw_x = (int16_t)(ellipse_center_x - ellipse_offset_x);
                draw_y = (int16_t)(ellipse_center_y - ellipse_offset_y);
                ellipse_line_done = 1;
                do {
                    ellipse_line_done = line_step(ellipse_center_x + ellipse_offset_x, ellipse_center_y + ellipse_offset_y, ellipse_line_done);
                    ++image_work_count;
                    plot_callback(1);
                    if (!effect_parameter && ++plot_color >= color_count)
                        plot_color = FIRST_CYCLING_COLOR;
                } while (!ellipse_line_done);
                draw_x = (int16_t)(ellipse_center_x - ellipse_offset_x);
                draw_y = (int16_t)(ellipse_center_y + ellipse_offset_y);
                do {
                    ellipse_line_done = line_step(ellipse_center_x + ellipse_offset_x, ellipse_center_y - ellipse_offset_y, ellipse_line_done);
                    plot_callback(1);
                    if (!effect_parameter && ++plot_color >= color_count)
                        plot_color = FIRST_CYCLING_COLOR;
                } while (!ellipse_line_done);
            }
            if (g_long5b36 > 0L) {
                --ellipse_offset_y;
                g_long5b3e -= g_long5b4e;
                g_long5b36 -= g_long5b3e;
            }
            ++ellipse_offset_x;
            g_long5b3a += g_long5b56;
            g_long5b36 += g_long5b52 + g_long5b3a;
            break;
        }
        ++ellipse_region;
        g_long5b36 += (3L * (g_long5b4a - g_long5b52) / 2L
                      - (g_long5b3a + g_long5b3e)) / 2L;
        /* Fall through to the second region of the ellipse. */
    case 1:
        if (ellipse_offset_y >= 0) {
            if (effect_parameter) {
                if (++plot_color >= color_count)
                    plot_color = FIRST_CYCLING_COLOR;
            } else {
                plot_color = (uint16_t)((ellipse_offset_x + ellipse_offset_y) & color_index_mask);
            }
            if ((uint16_t)ellipse_draw_mode < 2) {
                draw_x = (int16_t)(ellipse_center_x);
                draw_y = (int16_t)(ellipse_center_y);
                ellipse_line_done = 1;
                do {
                    ellipse_line_done = line_step(ellipse_center_x + ellipse_offset_x, ellipse_center_y + ellipse_offset_y, ellipse_line_done);
                    ++image_work_count;
                    plot_callback(1);
                    if (!effect_parameter && ++plot_color >= color_count)
                        plot_color = FIRST_CYCLING_COLOR;
                } while (!ellipse_line_done);
            } else {
                draw_x = (int16_t)(ellipse_center_x - ellipse_offset_x);
                draw_y = (int16_t)(ellipse_center_y - ellipse_offset_y);
                ellipse_line_done = 1;
                do {
                    ellipse_line_done = line_step(ellipse_center_x + ellipse_offset_x, ellipse_center_y + ellipse_offset_y, ellipse_line_done);
                    ++image_work_count;
                    plot_callback(1);
                    if (!effect_parameter && ++plot_color >= color_count)
                        plot_color = FIRST_CYCLING_COLOR;
                } while (!ellipse_line_done);
                draw_x = (int16_t)(ellipse_center_x - ellipse_offset_x);
                draw_y = (int16_t)(ellipse_center_y + ellipse_offset_y);
                do {
                    ellipse_line_done = line_step(ellipse_center_x + ellipse_offset_x, ellipse_center_y - ellipse_offset_y, ellipse_line_done);
                    plot_callback(1);
                    if (!effect_parameter && ++plot_color >= color_count)
                        plot_color = FIRST_CYCLING_COLOR;
                } while (!ellipse_line_done);
            }
            if (g_long5b36 < 0L) {
                ++ellipse_offset_x;
                g_long5b3a += g_long5b56;
                g_long5b36 += g_long5b3a;
            }
            --ellipse_offset_y;
            g_long5b3e -= g_long5b4e;
            g_long5b36 += g_long5b4a - g_long5b3e;
        } else {
            ++ellipse_region;
        }
    }
}

/* Scan Y within growing X columns and derive plot_color from a shifted
 * quadratic in Y minus shifted X. rebuild or a completed scan randomizes the
 * coefficients; the caller plots each generated coordinate.
 * Original 066A:2D5C, 190 bytes. */
void formula1(int16_t rebuild) {
    if (rebuild || draw_x > screen_extent) {
        draw_x = 0;
        draw_y = (int16_t)(-1);
        formula1_x_shift = dz_random(2) + 1;
        formula1_y_square_factor = dz_random(3) + 1;
        formula1_color_shift = dz_random(4) + 2;
    }
    if (draw_y++ > draw_x) {
        ++draw_x;
        draw_y = 0;
    }
    /* Preserve the original 16-bit products and shifts before palette masking. */
    int16_t y_square=(int16_t)(draw_y*draw_y);
    int16_t quadratic=(int16_t)(y_square*formula1_y_square_factor);
    int16_t x_term=(int16_t)((uint16_t)draw_x*(UINT32_C(1)<<formula1_x_shift));
    plot_color = (uint16_t)((uint16_t)dz_asr16((int16_t)(quadratic-x_term),(unsigned)formula1_color_shift)&color_index_mask);
}

/* Tile four 8-by-8 masks across a 96-unit-wide grid, returning only zero-mask
 * coordinates for the caller to plot. rebuild resets the tile cursor and color
 * variant; enhanced EGA doubles Y spacing.
 * Original 066A:2E1A, 313 bytes. */
void grid(int16_t rebuild) {
    if (rebuild) {
        plot_color = (uint16_t)(dz_random(4) & color_index_mask);
        grid_pattern_index = 0; grid_next_x = grid_next_y = 0;
        effect_parameter = dz_random(3);
    }
    do {
        if ((grid_pattern_index & 0x3f) == 0) {
            if (effect_parameter == 0 && ++plot_color >= color_count)
                plot_color = FIRST_CYCLING_COLOR;
            grid_tile_left = (int16_t)(grid_next_x);
            draw_x = (int16_t)(grid_next_x - 1);
            /* Enhanced EGA uses double-height software coordinates. */
            if (video_backend == BACKEND_EGA_640X700)
                draw_y = (int16_t)(grid_next_y << 1);
            else
                draw_y = (int16_t)(grid_next_y);
            grid_tile_right = grid_tile_left + 7;
            grid_pattern_index = (int16_t)(((grid_next_x + grid_next_y) & 0x18) << 3);
            grid_next_x += 8;
            if (grid_next_x >= 0x60) {
                if (++plot_color >= color_count)
                    plot_color = FIRST_CYCLING_COLOR;
                grid_next_x = 0;
                grid_next_y += 8;
                if (grid_next_y > 0x60)
                    grid_next_y = 0;
            }
        }
        if (++draw_x > grid_tile_right) {
            draw_x = (int16_t)(grid_tile_left);
            ++draw_y;
            if (video_backend == BACKEND_EGA_640X700)
                ++draw_y;
        }
    } while (ELEMENT(grid_patterns, grid_pattern_index++) != 0);
}

/* Generate expanding vertical-then-diagonal runs for hexagonal patterns
 * through the caller's symmetry. rebuild restarts at the origin; each wider run
 * begins at Y zero and advances the color.
 * Original 066A:2F53, 119 bytes. */
void hexagon(int16_t rebuild) {
    if (rebuild || (draw_x > screen_extent && draw_y > screen_extent)) {
        hexagon_start_x = hexagon_vertical_limit = 1;
        draw_x = draw_y = 0;
        if (++plot_color >= color_count)
            plot_color = FIRST_CYCLING_COLOR;
    }
    if (draw_x < 0) {
        draw_y = 0;
        ++hexagon_start_x;
        draw_x = (int16_t)(hexagon_start_x);
        ++hexagon_vertical_limit;
        choose_color(0);
    } else {
        ++draw_y;
        if (draw_y > hexagon_vertical_limit)
            --draw_x;
    }
}

/* Draw a recursive Hilbert curve at a random depth of three through five,
 * rasterizing segments with square stamps. rebuild starts a new curve and may
 * paint a background; retained state controls orientation and color pacing.
 * Original 066A:2FCA, 715 bytes. */
void hilbert(int16_t rebuild) {
    int16_t line_done, direction, column, solid_background, background_color_rule, step;
    int16_t segment_dx, segment_dy;
    /* Setup reuses the segment deltas as background side length and row counter. */
    if ((uint16_t)palette_elapsed_ticks >= (uint16_t)palette_cycle_ticks)
        cycle_palette();
    if (rebuild || hilbert_restart) {
        hilbert_restart = 0;
        segment_dx = screen_extent >> 1;
        /* step selects depth here, then holds a component during recursion swaps. */
        step = dz_random(3);
        hilbert_depth = step + 3;
        hilbert_stroke_width = 6 >> step;
        hilbert_step_x = (int16_t)(segment_dx / (2 << (hilbert_depth - 1)));
        hilbert_step_y = 0;
        hilbert_previous_x = hilbert_previous_y = hilbert_target_x = hilbert_target_y = 0;
        hilbert_color_per_point = dz_random(2);
        if (rebuild && dz_random(2)) {
            draw_x = draw_y = 0;
            solid_background = dz_random(2);
            if (solid_background) {
                if (++plot_color >= color_count)
                    plot_color = FIRST_CYCLING_COLOR;
            } else {
                background_color_rule = dz_random(3) + 2;
            }
            for (segment_dy = 1; segment_dy <= segment_dx; ++segment_dy) {
                for (column = 1; column <= segment_dx; ++column) {
                    if (!solid_background)
                        choose_color(background_color_rule);
                    plot_callback(1);
                    ++draw_y;
                }
                ++draw_x;
                draw_y -= segment_dx;
                if ((uint16_t)palette_elapsed_ticks >= (uint16_t)palette_cycle_ticks)
                    cycle_palette();
            }
            set_ega_raster_xor(0);
        }
        hilbert(0);
        image_work_count = 32000;
        hilbert_restart = 1;
    } else {
        --hilbert_depth;
        if (hilbert_depth) {
            /* Exchange axes for the first recursive subcurve, then restore them. */
            step = hilbert_step_x;
            hilbert_step_x = (int16_t)(hilbert_step_y);
            hilbert_step_y = step;
            hilbert(0);
            step = hilbert_step_y;
            hilbert_step_y = hilbert_step_x;
            hilbert_step_x = (int16_t)(step);
        }
        for (direction = 1; direction <= 3; ++direction) {
            switch (direction) {
            case 1: segment_dx = hilbert_step_x; segment_dy = hilbert_step_y; break;
            case 2: segment_dx = hilbert_step_y; segment_dy = hilbert_step_x; break;
            case 3:
                segment_dx = -hilbert_step_x;
                segment_dy = -hilbert_step_y;
                hilbert_step_x = (int16_t)(segment_dy);
                hilbert_step_y = segment_dx;
                break;
            }
            hilbert_target_x += segment_dx;
            hilbert_target_y += segment_dy;
            draw_x = (int16_t)(hilbert_previous_x);
            draw_y = (int16_t)(hilbert_previous_y);
            line_done = 1;
            do {
                line_done = line_step(hilbert_target_x, hilbert_target_y, line_done);
                square(hilbert_stroke_width, 1);
                if (hilbert_color_per_point && ++plot_color >= color_count)
                    plot_color = FIRST_CYCLING_COLOR;
            } while (!line_done);
            if (!hilbert_color_per_point && ++plot_color >= color_count)
                plot_color = FIRST_CYCLING_COLOR;
            hilbert_previous_x = hilbert_target_x;
            hilbert_previous_y = hilbert_target_y;
            if (hilbert_depth)
                hilbert(0);
        }
        hilbert_step_x = (int16_t)(-segment_dx);
        hilbert_step_y = -segment_dy;
        ++hilbert_depth;
    }
}

/* Generate bouncing points with unit horizontal speed and randomized
 * vertical speed after a bounce. rebuild resets motion; four calls form each
 * vertical stroke, and hitting the left boundary advances the color.
 * Original 066A:3295, 266 bytes. */
void lines(int16_t rebuild) {
    if (rebuild) {
        lines_stroke_index = 255;
        lines_base_y = 0;
        lines_step_x = lines_step_y = 1;
        plot_color = (uint16_t)((uint16_t)dz_random(4) & color_index_mask);
    }
    ++lines_stroke_index;
    if ((lines_stroke_index = lines_stroke_index & 3) == 0) {
        draw_x = (int16_t)(draw_x + lines_step_x);
        lines_base_y = (int16_t)(lines_base_y + lines_step_y);
        if (draw_x < 0) {
            lines_step_x = 1;
            draw_x = 0;
            if (++plot_color >= color_count)
                plot_color = FIRST_CYCLING_COLOR;
        }
        if (lines_base_y < 0) {
            lines_step_y = dz_random(4) + 1;
            lines_base_y = 0;
        }
        if (draw_x > screen_extent) {
            lines_step_x = -1;
            draw_x = (int16_t)(screen_extent);
        }
        if (lines_base_y > screen_extent) {
            lines_step_y = 0 - (dz_random(4) + 1);
            lines_base_y = screen_extent;
        }
        draw_y = (int16_t)(lines_base_y);
    } else {
        ++draw_y;
    }
}

/* Draw a complete star from two families of raster lines, reflecting and
 * exchanging coordinates to plot eight copies about a random origin.
 * Random spacing, an inner limit and color mode vary the star's appearance.
 * Original 066A:339F, 869 bytes. */
void littleStars(void) {
    int16_t outer_extent, inner_extent, line_spacing, center_x, center_y, min_start_x, base_color, target_x_step;
    int16_t reflected_y, start_x, start_y, end_x, end_y, line_done, saved_component, pass, reflection;
    int16_t color_mode, reflected_x;
    outer_extent = dz_random((int16_t)(3 * screen_extent / 8)) + screen_extent / 8;
    if (dz_random(2))
        inner_extent = 0;
    else
        inner_extent = (int16_t)((dz_random(3) + 1) * (outer_extent / 4));
    line_spacing = (int16_t)(((dz_random(4) + 2) * outer_extent) / 20 & 0xfffe);
    center_x = dz_random((int16_t)(screen_extent));
    center_y = dz_random((int16_t)(screen_extent));
    if (dz_random(2))
        min_start_x = dz_random((int16_t)(line_spacing));
    else
        min_start_x = 0;
    if (video_adapter == ADAPTER_VGA)
        color_mode = dz_random(3);
    else
        color_mode = 3;
    plot_color = (uint16_t)(dz_random((uint16_t)(color_count - 1)) & color_index_mask);
    base_color = (int16_t)plot_color;
    /* Pass zero targets the outer edge; pass one targets the inset diagonal. */
    for (pass = 0; pass <= 1; ++pass) {
        start_y = (start_x = line_spacing) << 1;
        plot_color = (uint16_t)(base_color & color_index_mask);
        if (pass) {
            end_y = end_x = (int16_t)(inner_extent);
            target_x_step = 1;
        } else {
            end_x = 0;
            end_y = (int16_t)(outer_extent);
            target_x_step = 0;
        }
        do {
            draw_x = (int16_t)(start_x);
            draw_y = (int16_t)(start_y);
            if (color_mode == 0 || color_mode == 2)
                plot_color = (uint16_t)(base_color & color_index_mask);
            line_done = 1;
            do {
                line_done = line_step(end_x, end_y, line_done);
                reflected_x = draw_x;
                reflected_y = draw_y;
                /* Four sign reflections, then exchange X/Y for four more. */
                for (reflection = 0; reflection <= 7; ++reflection) {
                    draw_x = (int16_t)(center_x + reflected_x);
                    draw_y = (int16_t)(center_y + reflected_y);
                    if (draw_x < screen_extent && draw_y < screen_extent)
                        plot_callback(1);
                    reflected_y = -reflected_y;
                    if (reflection & 1)
                        reflected_x = -reflected_x;
                    if (reflection == 3) {
                        saved_component = reflected_x;
                        reflected_x = reflected_y;
                        reflected_y = saved_component;
                    }
                    if (color_mode == 2 && ++plot_color >= color_count)
                        plot_color = FIRST_CYCLING_COLOR;
                }
                draw_x = (int16_t)(reflected_y);
                draw_y = (int16_t)(reflected_x);
                if (color_mode == 0 && ++plot_color >= color_count)
                    plot_color = FIRST_CYCLING_COLOR;
                ++image_work_count;
                if ((uint16_t)palette_elapsed_ticks >= (uint16_t)palette_cycle_ticks)
                    cycle_palette();
            } while (!line_done);
            if (color_mode == 1 && ++plot_color >= color_count)
                plot_color = FIRST_CYCLING_COLOR;
            if (color_mode == 3) {
                ++ega_color_substep;
                if ((ega_color_substep &= 3) == 0 && ++plot_color >= color_count)
                    plot_color = FIRST_CYCLING_COLOR;
            }
            end_x -= target_x_step;
            --end_y;
            start_x -= start_y & 1;
            --start_y;
        } while (start_x >= min_start_x);
    }
    draw_x = (int16_t)(center_x);
    draw_y = (int16_t)(center_y);
}

/* Generate a random pen position and choose its color; rebuild selects a new
 * color rule in effect_parameter. The caller plots the resulting point.
 * Original 066A:3704, 129 bytes. */
void lotsadots(int16_t rebuild) {
    if (rebuild)
        effect_parameter = dz_random(9);
    draw_x = (int16_t)(dz_random((int16_t)(screen_extent)));
    draw_y = (int16_t)(dz_random((int16_t)(screen_extent)));
    choose_color(effect_parameter);
}

/* Generate inward polar sweeps from successively smaller starting radii.
 * rebuild restarts the pattern and color; eight calls extend each polar point
 * into a diagonal stroke, with color changes as the radius decreases.
 * Original 066A:3785, 302 bytes. */
void pedals(int16_t rebuild) {
    float angle;
    if (rebuild || pedals_start_radius <= 0) {
        pedals_stroke_index = 255;
        pedals_phase = 0;
        pedals_start_radius = pedals_radius = 3 * (screen_extent / 4);
        if (rebuild)
            plot_color = (uint16_t)(FIRST_CYCLING_COLOR & color_index_mask);
        else if (++plot_color >= color_count)
            plot_color = FIRST_CYCLING_COLOR;
    }
    if (pedals_radius <= 0) {
        pedals_start_radius -= 16;
        pedals_radius = pedals_start_radius;
        pedals_phase = 0;
    }
    ++pedals_stroke_index;
    if ((pedals_stroke_index = pedals_stroke_index & 7) == 0) {
        pedals_phase += 31;
        angle = (float)(pedals_phase * 0.0002);
        draw_x = (int16_t)(pedals_radius * sin(angle));
        draw_y = (int16_t)(pedals_radius * cos(angle));
        if ((--pedals_radius & 7) == 0 && ++plot_color >= color_count)
            plot_color = FIRST_CYCLING_COLOR;
    } else {
        ++draw_x;
        ++draw_y;
    }
}

/* Generate lines with random endpoints about a fixed or diagonally drifting
 * origin. rebuild chooses the variant; a boundary crossing starts another puff.
 * Advance the shared line iterator and color for the caller to plot.
 * Original 066A:38B3, 534 bytes. */
void puffs(int16_t rebuild) {
    if (rebuild || abs(draw_x) >= screen_extent || abs(draw_y) >= screen_extent) {
        draw_x = puffs_origin_x = dz_random((int16_t)(screen_extent));
        draw_y = puffs_origin_y = dz_random((int16_t)(screen_extent));
        puffs_spread = dz_random((int16_t)(screen_extent / 6)) + screen_extent / 6;
        puffs_line_done = 1;
        puffs_end_x = (int16_t)(puffs_origin_x + puffs_spread);
        puffs_end_y = (int16_t)(puffs_origin_y + puffs_spread);
        if (rebuild)
            effect_parameter = dz_random(2);
        puffs_drift_step = (char)(dz_random(2) + 1);
    }
    puffs_line_done = line_step(puffs_end_x, puffs_end_y, puffs_line_done);
    choose_color(0);
    if (puffs_line_done) {
        if (effect_parameter) {
            draw_x = (int16_t)(puffs_origin_x);
            draw_y = (int16_t)(puffs_origin_y);
            puffs_end_x = (int16_t)(puffs_origin_x + dz_random((int16_t)(puffs_spread * 2)) - puffs_spread);
            puffs_end_y = (int16_t)(puffs_origin_y + dz_random((int16_t)(puffs_spread * 2)) - puffs_spread);
        } else {
            puffs_origin_x += puffs_drift_step;
            draw_x = (int16_t)(puffs_origin_x);
            puffs_origin_y += puffs_drift_step;
            draw_y = (int16_t)(puffs_origin_y);
            puffs_end_x = (int16_t)(puffs_origin_x + dz_random((int16_t)(puffs_spread)));
            puffs_end_y = (int16_t)(puffs_origin_y + dz_random((int16_t)(puffs_spread)));
        }
    }
}

/* Generate ribbon bands from horizontal runs that lengthen between rows.
 * rebuild resets the run limits and color variant; boundary crossings wrap to
 * random positions, and row changes shift the pen left.
 * Original 066A:3AC9, 338 bytes. */
void ribbons(int16_t rebuild) {
    if (rebuild) {
        ribbons_run_step = ribbons_run_length = 0;
        ribbons_run_limit = 24;
        effect_parameter = dz_random(2);
    }
    if (ribbons_run_step++ > ribbons_run_length) {
        draw_x -= ribbons_run_length;
        ribbons_run_step = 0;
        if (++ribbons_run_length > ribbons_run_limit) {
            ribbons_run_length = 0;
            if ((ribbons_run_limit += dz_random(4)) > 44)
                ribbons_run_limit = 0;
        }
        if (effect_parameter && ++plot_color >= color_count)
            plot_color = FIRST_CYCLING_COLOR;
        if (++draw_y > screen_extent)
            draw_y = (int16_t)(dz_random((int16_t)(screen_extent / 2)));
    }
    if (++draw_x > screen_extent)
        draw_x = (int16_t)(dz_random((int16_t)(screen_extent / 2)));
    if (dz_random(1000) == 0 && ++plot_color >= color_count)
        plot_color = FIRST_CYCLING_COLOR;
}

/* Sierpinski triangle by the chaos game: move halfway toward a random
 * vertex each call. rebuild chooses triangle placement and either a fixed color
 * or color advances on vertex zero; the caller plots the pen.
 * Original 066A:3C1B, 311 bytes. */
void sierpinski(int16_t rebuild) {
    int16_t vertex_index;
    register int16_t vertex_extent;
    if (rebuild) {
        vertex_extent = (int16_t)(dz_random((int16_t)(screen_extent / 2)) + screen_extent / 3);
        draw_x = draw_y = (int16_t)(vertex_extent);
        ELEMENT(sierpinski_vertices, 1).x = ELEMENT(sierpinski_vertices, 1).y = ELEMENT(sierpinski_vertices, 2).y = vertex_extent;
        if (dz_random(2))
            ELEMENT(sierpinski_vertices, 0).x = ELEMENT(sierpinski_vertices, 0).y = ELEMENT(sierpinski_vertices, 2).x = vertex_extent / 2;
        else
            ELEMENT(sierpinski_vertices, 0).x = ELEMENT(sierpinski_vertices, 0).y = ELEMENT(sierpinski_vertices, 2).x = 0;
        sierpinski_fixed_color = dz_random(2);
        plot_color = (uint16_t)((uint16_t)dz_random(4) & color_index_mask);
    }
    vertex_index = dz_random(3);
    if (vertex_index == 0 && sierpinski_fixed_color == 0 && ++plot_color >= color_count)
        plot_color = FIRST_CYCLING_COLOR;
    /* Divide the displacement to retain the original rounding toward zero. */
    draw_x = (int16_t)(draw_x + (int16_t)(ELEMENT(sierpinski_vertices, vertex_index).x - draw_x) / 2);
    draw_y = (int16_t)(draw_y + (int16_t)(ELEMENT(sierpinski_vertices, vertex_index).y - draw_y) / 2);
}

/* Generate an ellipse whose X/Y amplitudes drift with separate sinusoidal
 * phases. rebuild initializes amplitudes and increments; zero X advances
 * color, and enhanced EGA rounds both coordinates down to even values.
 * Original 066A:3D52, 505 bytes. */
void sine(int16_t rebuild)
{
    static dz_stroke_id stroke;
    if (dz_hybrid_enabled() && (rebuild || !stroke)) stroke = dz_stroke_begin(DZ_STROKE_SINE);
    if (rebuild) {
        sine_phase = mod_x = mod_y = 0;
        if (++plot_color >= color_count)
            plot_color = FIRST_CYCLING_COLOR;
        amplitude_x = screen_extent / 8 + dz_random((int16_t)(screen_extent / 8));
        amplitude_y = screen_extent / 8 + dz_random((int16_t)(screen_extent / 8));
        step_x = (float)(1.0 / (1 + amplitude_x));
        step_y = (float)(1.0 / (1 + amplitude_y));
    }
    double stroke_x = amplitude_x * sin(sine_phase);
    double stroke_y = amplitude_y * cos(sine_phase);
    draw_x = (int16_t)stroke_x;
    draw_y = (int16_t)stroke_y;
    sine_phase = (float)(sine_phase + 0.01);
    mod_x = (float)((double)mod_x + step_x);
    mod_y = (float)((double)mod_y + step_y);
    amplitude_x = (float)(amplitude_x + sin(mod_x));
    amplitude_y = (float)(amplitude_y + cos(mod_y));
    if (draw_x == 0 && ++plot_color >= color_count)
        plot_color = FIRST_CYCLING_COLOR;
    if (video_backend == BACKEND_EGA_640X700) {
        draw_x &= ~1;
        draw_y &= ~1;
    }
    /* Hybrid: keep the fractional sample, applying the original even-row shift. */
    if (dz_hybrid_enabled()) {
        stroke_x += draw_x - (int16_t)stroke_x;
        stroke_y += draw_y - (int16_t)stroke_y;
        dz_stroke_sample(stroke, stroke_x, stroke_y, draw_x, draw_y);
    }
}

/* Generate growing cardinal square circuits, shifting X and color after
 * each circuit. rebuild or a size/boundary limit starts at a random position;
 * effect_parameter selects the shift direction and side-length limit.
 * Original 066A:3F4B, 354 bytes. */
void squares(int16_t rebuild) {
    if (rebuild || squares_side_length >= squares_side_limit || draw_x > screen_extent || draw_y > screen_extent) {
        squares_step_x = squares_step_y = squares_side_step = squares_side_length = squares_direction = 0;
        draw_x = (int16_t)(dz_random((int16_t)(screen_extent)));
        draw_y = (int16_t)(dz_random((int16_t)(screen_extent)));
        effect_parameter = dz_random(2);
        if (targa64_mode)
            effect_parameter = 0;
        squares_side_limit = dz_random((int16_t)(screen_extent / 4)) + screen_extent / 4;
        if (effect_parameter)
            squares_side_limit = dz_asr16(squares_side_limit,1);
    }
    if (squares_side_step++ >= squares_side_length) {
        squares_side_step = 0;
        if (++squares_direction > 3) {
            ++squares_side_length;
            if (effect_parameter)
                --draw_x;
            else
                ++draw_x;
            if (++plot_color >= color_count)
                plot_color = FIRST_CYCLING_COLOR;
            squares_direction = 0;
        }
        squares_step_x = ELEMENT(direction_x, squares_direction);
        squares_step_y = ELEMENT(direction_y, squares_direction);
    }
    draw_x += squares_step_x;
    draw_y += squares_step_y;
}

/* Draw a complete inward square spiral from rotating line segments whose
 * scale decays each turn. Choose a random center and reflection variant;
 * rebuild is unused, and palette/color work continues between segments.
 * Original 066A:40AD, 778 bytes. */
void squirals(int16_t rebuild) {
    int16_t end_x, end_y, center_x, center_y, radius, reflection_mode, line_done;
    float scale = 1.0, decay, angle_step, cosine, sine_value, angle;
    int16_t saved_x, saved_y;

    (void)rebuild;

    angle = (float)(dz_random(2) * 0.7853975);
    angle_step = (float)((dz_random(3) + 1) * 0.03);
    decay = (float)(1.0 / (sin(angle_step) + cos(angle_step)));
    radius = dz_random((int16_t)(3 * screen_extent / 8)) + screen_extent / 8;
    center_x = dz_random((int16_t)(3 * screen_extent / 4));
    center_y = dz_random((int16_t)(3 * screen_extent / 4));
    reflection_mode = dz_random(4);
    do {
        cosine = (float)(cos(angle));
        sine_value = (float)(sin(angle));
        end_x = (int16_t)(radius * (double)scale * ((double)cosine - sine_value));
        end_y = (int16_t)(radius * (double)scale * ((double)cosine + sine_value));
        draw_x = (int16_t)(end_y);
        draw_y = (int16_t)(-end_x);
        line_done = 1;
        do {
            line_done = line_step(end_x, end_y, line_done);
            saved_x = draw_x;
            saved_y = draw_y;
            switch (reflection_mode) {
            case 0:
            case 1:
            case 2:
                draw_x = (int16_t)(center_x + saved_x);
                draw_y = (int16_t)(center_y + saved_y);
                plot_callback(1);
                draw_x = (int16_t)(center_x - saved_x);
                draw_y = (int16_t)(center_y - saved_y);
                plot_callback(1);
                if (reflection_mode == 2)
                    break;
                /* Fall through: modes 0/1 also draw the quarter-turn pair. */
            case 3:
                draw_x = (int16_t)(center_x - saved_y);
                draw_y = (int16_t)(center_y + saved_x);
                plot_callback(1);
                draw_x = (int16_t)(center_x + saved_y);
                draw_y = (int16_t)(center_y - saved_x);
                plot_callback(1);
            }
            draw_x = (int16_t)(saved_x);
            draw_y = (int16_t)(saved_y);
            if ((uint16_t)palette_elapsed_ticks >= (uint16_t)palette_cycle_ticks)
                cycle_palette();
            ++image_work_count;
        } while (!line_done);
        angle = (float)((double)angle + angle_step);
        scale = (float)((double)scale * decay);
        if (++plot_color >= color_count)
            plot_color = FIRST_CYCLING_COLOR;
    } while (abs(end_x) >= 1 || abs(end_y) >= 1);
    draw_x = (int16_t)(center_x);
    draw_y = (int16_t)(center_y);
}

/* Generate an outward cardinal spiral, lengthening its legs every second
 * turn. rebuild or an upper boundary crossing resets to a random position;
 * color advances every eight calls, and the caller plots each point.
 * Original 066A:43B7, 235 bytes. */
void spiral(int16_t rebuild) {
    if (rebuild || draw_x > screen_extent || draw_y > screen_extent) {
        spiral_leg_step = spiral_leg_length = spiral_direction = spiral_color_phase = 0;
        draw_x = (int16_t)(dz_random((int16_t)(screen_extent)));
        draw_y = (int16_t)(dz_random((int16_t)(screen_extent)));
    }
    if (spiral_leg_step++ >= spiral_leg_length) {
        spiral_leg_step = 0;
        ++spiral_direction;
        spiral_direction = spiral_direction & 3;
        if ((spiral_direction & 1) == 0)
            ++spiral_leg_length;
        spiral_step_x = ELEMENT(direction_x, spiral_direction);
        spiral_step_y = ELEMENT(direction_y, spiral_direction);
    }
    draw_x += spiral_step_x;
    draw_y += spiral_step_y;
    ++spiral_color_phase;
    if ((spiral_color_phase = spiral_color_phase & 7) == 0 && ++plot_color >= color_count)
        plot_color = FIRST_CYCLING_COLOR;
}

/* Generate growing diamond circuits, shifting X and color after each
 * circuit. rebuild or a size/boundary limit starts at a random position;
 * effect_parameter selects the horizontal shift direction.
 * Original 066A:44A2, 345 bytes. */
void spiral1(int16_t rebuild) {
    if (rebuild || spiral1_side_length >= spiral1_side_limit || draw_x > screen_extent || draw_y > screen_extent) {
        spiral1_step_x = spiral1_step_y = spiral1_side_step = spiral1_side_length = spiral1_direction = 0;
        draw_x = (int16_t)(dz_random((int16_t)(screen_extent)));
        draw_y = (int16_t)(dz_random((int16_t)(screen_extent)));
        effect_parameter = dz_random(2);
        if (targa64_mode)
            effect_parameter = 1;
        spiral1_side_limit = dz_random((int16_t)(3 * (screen_extent / 4)));
        if (effect_parameter)
            spiral1_side_limit = dz_asr16(spiral1_side_limit,1);
    }
    if (spiral1_side_step++ >= spiral1_side_length) {
        spiral1_side_step = 0;
        if (++spiral1_direction > 3) {
            ++spiral1_side_length;
            if (effect_parameter)
                --draw_x;
            else
                ++draw_x;
            spiral1_direction = 0;
            if (++plot_color >= color_count)
                plot_color = FIRST_CYCLING_COLOR;
        }
        spiral1_step_x = ELEMENT(diamond_x, spiral1_direction);
        spiral1_step_y = ELEMENT(diamond_y, spiral1_direction);
    }
    draw_x += spiral1_step_x;
    draw_y += spiral1_step_y;
}

/* Generate sweeps from the origin with X advancing each call and Y rising
 * at increasingly int32_t intervals. Completed sweeps change color; rebuild or
 * an interval limit restarts the sequence for the caller to plot.
 * Original 066A:45FB, 107 bytes. */
void sweep(int16_t rebuild) {
    if (rebuild || sweep_rise_interval > screen_extent / 2)
        sweep_rise_interval = sweep_rise_counter = draw_x = draw_y = 0;
    if (sweep_rise_counter++ > sweep_rise_interval) {
        ++draw_y;
        sweep_rise_counter = 0;
    }
    if (draw_x++ > screen_extent) {
        if (++plot_color >= color_count)
            plot_color = FIRST_CYCLING_COLOR;
        ++sweep_rise_interval;
        draw_x = draw_y = 0;
    }
}

/* Generate rays from the origin to Y == screen_extent, changing color
 * between rays. rebuild chooses either cyclic X endpoints or a changing signed
 * endpoint step; the shared line iterator supplies each point to the caller.
 * Original 066A:4666, 202 bytes. */
void sweep2(int16_t rebuild) {
    static dz_stroke_id stroke;
    int16_t initialize_line = rebuild;
    if (initialize_line || sweep2_line_done != 0) {
        draw_x = draw_y = 0;
        if (initialize_line) {
            effect_parameter = dz_random(2);
            if (targa64_mode)
                effect_parameter = 0;
            if (effect_parameter) {
                sweep2_end_x = sweep2_end_x_step = 0;
            } else {
                sweep2_end_x = (int16_t)(-1);
            }
        }
        if (effect_parameter) {
            sweep2_end_x = (int16_t)(sweep2_end_x + sweep2_end_x_step);
            if (++sweep2_end_x_step > screen_extent)
                sweep2_end_x_step = -screen_extent;
        } else {
            ++sweep2_end_x;
            sweep2_end_x = (int16_t)(sweep2_end_x % (int16_t)(screen_extent + 2));
        }
        sweep2_end_y = screen_extent;
        if (++plot_color >= color_count)
            plot_color = FIRST_CYCLING_COLOR;
        initialize_line = 1;
    }
    /* Hybrid: a fresh primitive at each ray; only the outer plot commits it. */
    if (dz_hybrid_enabled() && (initialize_line || !stroke)) stroke = dz_stroke_begin(DZ_STROKE_SWEEP2);
    sweep2_line_done = line_step(sweep2_end_x, sweep2_end_y, initialize_line);
    dz_stroke_line(stroke, 0, 0, sweep2_end_x, sweep2_end_y, draw_x, draw_y);
}

/* Generate filled triangular wedges by scanning between widening row
 * bounds. rebuild initializes the variants; successive randomly placed wedges
 * alternate left-edge expansion and per-row color changes.
 * Original 066A:4730, 417 bytes. */
void triangles(int16_t rebuild) {
    if (rebuild) {
        triangles_active = 0;
        effect_parameter = dz_random(3);
        triangles_expand_left = dz_random(3);
        plot_color = (uint16_t)(dz_random(4) & color_index_mask);
    }
    if (triangles_active) {
        if (++draw_x > triangles_row_right) {
            if (effect_parameter && ++plot_color >= color_count)
                plot_color = FIRST_CYCLING_COLOR;
            if (triangles_expand_left)
                --triangles_row_left;
            if (triangles_row_right++ >= triangles_right_limit) {
                triangles_active = 0;
            } else {
                draw_x = (int16_t)(triangles_row_left);
                if (++draw_y > screen_extent) {
                    triangles_active = 0;
                    draw_y = (int16_t)(screen_extent);
                }
            }
        }
    }
    if (!triangles_active) {
        triangles_active = 1;
        effect_parameter = !effect_parameter;
        triangles_expand_left = !triangles_expand_left;
        triangles_row_left = dz_random((int16_t)(screen_extent / 2));
        triangles_row_right = triangles_row_left;
        draw_x = (int16_t)(triangles_row_left);
        triangles_right_limit = (int16_t)(dz_random((int16_t)(screen_extent / 3)) + triangles_row_left + 8);
        if (triangles_right_limit > screen_extent)
            triangles_right_limit = (int16_t)(screen_extent);
        draw_y = (int16_t)(dz_random((int16_t)(screen_extent)));
    }
}

/* Generate colored scanlines from X zero, advancing color each row. size
 * selects a fixed right edge or one that follows Y, making rectangular or
 * widening triangular passes for the caller to plot.
 * Original 066A:48D1, 181 bytes. */
void tunnel(int16_t rebuild) {
    if (rebuild) {
        tunnel_rows_left = 0;
        if (dz_random(2)) {
            tunnel_right_edge_ptr = &tunnel_right_limit;
        } else {
            tunnel_right_edge_ptr = &draw_y;
        }
        plot_color = (uint16_t)(FIRST_CYCLING_COLOR & color_index_mask);
    }
    if (tunnel_rows_left) {
        if (++draw_x > *tunnel_right_edge_ptr) {
            draw_x = (int16_t)(tunnel_row_left);
            ++draw_y;
            --tunnel_rows_left;
            if (draw_y > screen_extent) {
                tunnel_rows_left = 0;
                draw_y = (int16_t)(screen_extent);
            }
            if (++plot_color >= color_count)
                plot_color = FIRST_CYCLING_COLOR;
        }
    }
    if (tunnel_rows_left <= 0) {
        tunnel_right_limit = tunnel_rows_left = screen_extent;
        draw_x = tunnel_row_left = draw_y = 0;
    }
}

/* Generate lines between moving endpoints, returning line_step completion.
 * rebuild resets the walk; endpoint velocities reverse outside the bounds and
 * are randomized after four to 32 segments. The caller plots the pen.
 * Original 066A:4986, 456 bytes. */
int16_t wandering(int16_t rebuild) {
    if (rebuild) {
        wandering_start_x = wandering_start_y = wandering_end_x = wandering_end_y = wandering_segments_left = 0;
        wandering_line_done = 0;
        if (++plot_color >= color_count)
            plot_color = FIRST_CYCLING_COLOR;
    }
    if (!rebuild && !wandering_line_done)
        return wandering_line_done = line_step(wandering_end_x, wandering_end_y, 0);
    if (wandering_segments_left <= 0) {
        wandering_start_dx = dz_random(9) - 4;
        wandering_end_dx = dz_random(9) - 4;
        wandering_start_dy = dz_random(9) - 4;
        wandering_end_dy = dz_random(9) - 4;
        wandering_segments_left = (int16_t)((dz_random(8) + 1) << 2);
    }
    if (++plot_color >= color_count)
        plot_color = FIRST_CYCLING_COLOR;
    wandering_start_x += wandering_start_dx;
    if (wandering_start_x < 1 || wandering_start_x > screen_extent)
        wandering_start_dx = -wandering_start_dx;
    wandering_end_x += wandering_end_dx;
    if (wandering_end_x < 1 || wandering_end_x > screen_extent)
        wandering_end_dx = -wandering_end_dx;
    wandering_start_y += wandering_start_dy;
    if (wandering_start_y < 1 || wandering_start_y > screen_extent)
        wandering_start_dy = -wandering_start_dy;
    wandering_end_y += wandering_end_dy;
    if (wandering_end_y < 1 || wandering_end_y > screen_extent)
        wandering_end_dy = -wandering_end_dy;
    --wandering_segments_left;
    draw_x = (int16_t)(wandering_start_x);
    draw_y = (int16_t)(wandering_start_y);
    return wandering_line_done = line_step(wandering_end_x, wandering_end_y, 1);
}

/* Generate growing horizontal/vertical stair steps or a bouncing walk
 * with random runs of 12..23 points. rebuild selects the variant; color changes
 * every eight stair steps or at each new random run.
 * Original 066A:4B4E, 495 bytes. */
void zigstream(int16_t rebuild) {
    int16_t walk_variant;
    if (rebuild || draw_x > screen_extent || draw_y > screen_extent) {
        if (rebuild)
            effect_parameter = dz_random(2);
        zigstream_run_counter = zigstream_run_length = zigstream_direction = zigstream_color_phase = 0;
        draw_x = (int16_t)(dz_random((int16_t)(screen_extent)));
        draw_y = (int16_t)(dz_random((int16_t)(screen_extent)));
        if (++plot_color >= color_count)
            plot_color = FIRST_CYCLING_COLOR;
    }
    if ((walk_variant = effect_parameter) == 0) {
        if (zigstream_run_counter++ >= zigstream_run_length) {
            zigstream_run_counter = 0;
            if ((++zigstream_direction & 1) == 0) {
                ++zigstream_run_length;
                zigstream_direction = 0;
            }
            zigstream_step_x = ELEMENT(zigstream_dx, zigstream_direction);
            zigstream_step_y = ELEMENT(zigstream_dy, zigstream_direction);
        }
        ++zigstream_color_phase;
        if ((zigstream_color_phase = zigstream_color_phase & 7) == 0) {
            ++plot_color;
            if (plot_color >= color_count)
                plot_color = FIRST_CYCLING_COLOR;
        }
    } else {
        if (--zigstream_run_counter <= 0) {
            zigstream_step_x = dz_random(2) ? -1 : 1;
            zigstream_step_y = dz_random(3) - 1;
            zigstream_run_counter = dz_random(12) + 12;
            if (++plot_color >= color_count)
                plot_color = FIRST_CYCLING_COLOR;
        }
        if (draw_x <= 0 || draw_x >= screen_extent)
            zigstream_step_x = -zigstream_step_x;
        if (draw_y <= 0 || draw_y >= screen_extent)
            zigstream_step_y = -zigstream_step_y;
    }
    draw_x += zigstream_step_x;
    draw_y += zigstream_step_y;
}

/* The DOS table calls these with a word argument and ignores AX on return. */
static void littleStars_image(int16_t rebuild) { (void)rebuild; littleStars(); }
static void wandering_image(int16_t rebuild) { (void)wandering(rebuild); }

/* Original image/fade record values; native records use normal pointer alignment. */
ImageConfigRow alt_image_records[36]={
    {arches,40,"arches",1,0,'1'},
    {atom,40,"atom",1,0,'2'},
    {balls,40,"balls",1,1,'3'},
    {bars,25,"bars",1,1,'4'},
    {bigstar,5,"bigstar",0,0,'5'},
    {boxes,75,"boxes",1,1,'6'},
    {cones,20,"cones",1,0,'7'},
    {corkscrew,20,"corkscrew",0,0,'8'},
    {curves,80,"curves",1,0,'9'},
    {cycloids,80,"cycloids",0,0,'0'},
    {daisies,60,"daisies",0,0,'q'},
    {diag1,55,"diag1",0,1,'w'},
    {diag2,55,"diag2",0,1,'e'},
    {ellipse,90,"ellipse",0,0,'r'},
    {formula1,38,"formula1",1,0,'t'},
    {grid,30,"grid",0,1,'y'},
    {hexagon,20,"hexagon",0,1,'u'},
    {hilbert,1,"hilbert",0,1,'i'},
    {lines,10,"lines",1,0,'o'},
    {littleStars_image,10,"littleStars",0,0,'p'},
    {lotsadots,3,"lotsadots",1,0,'a'},
    {pedals,30,"pedals",1,1,'s'},
    {puffs,15,"puffs",1,0,'d'},
    {ribbons,18,"ribbons",1,1,'f'},
    {triangles,25,"triangles",1,1,'g'},
    {tunnel,30,"tunnel",1,1,'h'},
    {sierpinski,20,"sierpinski",1,0,'j'},
    {sine,9,"sine",0,0,'k'},
    {spiral,35,"spiral",1,1,'l'},
    {spiral1,40,"spiral1",1,1,'z'},
    {squares,50,"squares",1,1,'x'},
    {squirals,10,"squirals",0,0,'c'},
    {sweep,10,"sweep",1,1,'v'},
    {sweep2,40,"sweep2",1,0,'b'},
    {wandering_image,40,"wandering",1,0,'n'},
    {zigstream,7,"zigstream",0,0,'m'}
};
ImageConfigRow ctrl_image_records[6]={
    {butterflies,50,"butterflies",1,0,'a'},
    {diamonds,60,"diamonds",1,0,'b'},
    {gumballs,30,"gumballs",0,0,'c'},
    {tiles,60,"tiles",1,0,'d'},
    {wedges,60,"wedges",1,0,'e'},
    {yarnball,10,"yarnball",0,0,'f'}
};
FadeConfigRow fade_records[24]={
    {'a',"rAndomize",1},
    {'b',"Blister",1},
    {'c',"Countdown",1},
    {'d',"Drip",1},
    {'e',"Exclusive-or",1},
    {'f',"Firefly",1},
    {'g',"Gas cloud",1},
    {'i',"spIral",1},
    {'j',"Juggle palette",1},
    {'k',"Kleenup",1},
    {'l',"Linewipe",1},
    {'m',"Mesh",1},
    {'o',"firewOrks",1},
    {'p',"Pan/scroll",1},
    {'q',"Quilt",1},
    {'r',"Rectangles",1},
    {'s',"Starburst",1},
    {'t',"Twist",1},
    {'u',"Umbra",1},
    {'v',"moVingbars",1},
    {'w',"Wipe",1},
    {'x',"heXagon",1},
    {'y',"tetrYs",1},
    {'z',"Ziti",1}
};

/* Initialize options/configuration, video, input hooks, image resources and
 * palettes, then run Auto or Semi-Auto until exit. Restore runtime/video
 * state and the startup directory, and conditionally show closing output.
 * Original 066A:4D3D, 1123 bytes. */
int dazzle_main(int argc, char **argv)
{
    int16_t config_load_option, integrity_option_character;
    char *drive_separator;

    strncpy(executable_path, argv[0], sizeof executable_path - 1);
    executable_path[sizeof executable_path - 1] = '\0';
    gettextinfo(&saved_text_info);
    atexit(restore_text_attributes);
    dz_nosound();
    startup_active = 1;
    initialize_program_defaults();
    save_video_state();
    alt_image_count = 36;
    ctrl_image_count = 6;
    fade_count = 24;
    if (_getcwd(startup_directory, 80) == NULL) {
        fprintf(stderr, "\r Startup error getting directory.\n");
        fprintf(stderr, "Please write for assistance. Press any key to continue.\a");
        dz_bioskey(BIOS_KEY_READ);
    }
    config_load_option = parse_command_line((int16_t)argc, argv, &integrity_option_character);
    detect_video_adapter();
    print_startup_information();
    keyboard_led_flags(KEYBOARD_LED_MASK, 1);
    /* Validate the parser's /I location before trusting the integrity bypass. */
    if (insecure_argument_index && tolower((unsigned char)argv[insecure_argument_index][integrity_option_character]) != OPTION_SKIP_INTEGRITY) {
        exit_requested = 1;
        fatal_selfcheck_error(1, "parser corrupted!");
    }
    if (!show_mode && !window_mode)
        check_executable_integrity();
    else
        exit_requested = 0;
    configure_video_backend(1);
    resolve_startup_configuration(config_load_option);
    if (show_mode || bashful_mode || window_mode)
        keep_typematic = 1;
    if (!keep_typematic)
        dzle_set_typematic(BIOS_TYPEMATIC_DELAY_1S, BIOS_TYPEMATIC_RATE_2CPS);
    install_runtime_callbacks();
    atexit(restore_runtime_callbacks);
    dzle_install_signal_handlers();
    if (!show_mode && !exit_requested && !window_mode) {
        countdown_ticks = 40;
        if (!bashful_mode || !no_disk_park)
            beep(880, 40);
        if (!no_disk_park)
            park_disk_heads();
        if (!targa64_mode && (!bashful_mode || !no_disk_park)) {
            textcolor(TEXT_COLOR_YELLOW);
            cprintf(" \311\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\273 \r\n");
            cprintf(" \272 ");
            textcolor(TEXT_COLOR_BLINK_LIGHT_RED);
            cprintf("Program Loading");
            textcolor(TEXT_COLOR_YELLOW);
            cprintf(" \272 \r\n");
            cprintf(" \310\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\274 \r\n\n");
            textcolor(TEXT_COLOR_CYAN);
        }
    }
    prepare_logo_image_resources();
    palette_build_callback(0);
    /* The installed timer interrupt decrements this startup countdown. */
    while (countdown_ticks && !dz_testctl_stopped())
        dz_service();
    background_fill_word = 0;
    mode_setup_callback(1);
    fflush(NULL);
    palette_upload_callback(palette_buffer + palette_cycle_offset);
    dz_srand(dz_options.seed);
    while (dz_bioskey(BIOS_KEY_PENDING))
        dz_bioskey(BIOS_KEY_READ);
    startup_active = input_pending = mouse_abort_pending = 0;
    while (!exit_requested) {
        if (semi_auto_mode)
            semi_auto_loop();
        else
            auto_loop();
    }
    keyboard_led_flags(KEYBOARD_LED_MASK, 1);
    set_ega_raster_xor(0);
    if (!show_mode && !bashful_mode && !startup_config_failure)
        closing_graphic_and_music();
    if (!bashful_mode)
        while (dz_bioskey(BIOS_KEY_PENDING))
            dz_bioskey(BIOS_KEY_READ);
    restore_video_state(1);
    if (!bashful_mode && !show_mode) {
        if (_chdir(startup_directory)) {
            fprintf(stderr, "PATH ERROR ON EXIT\a");
        } else {
            drive_separator = strchr(startup_directory, ':');
            if (drive_separator != NULL)
                dzle_setdisk(tolower((unsigned char)drive_separator[-1]) - 'a');
        }
    }
    if (startup_config_failure) {
        fprintf(stderr, "Program aborting in startup routines due to defective configuration file!\n\a");
        return 14;
    }
    dz_nosound();
    if (!show_mode && !bashful_mode && !window_mode)
        print_closing_text_and_exit(0);
    return 0;
}

/* Uninitialized module state. BC++ 3.0 emits BSS in reverse declaration order.
 * Offsets below are the independently recovered DGROUP addresses. */
uint16_t image_data_offset; /* DS:602F */
uint16_t serial_r2; /* DS:602D */
uint16_t serial_r1; /* DS:602B */
uint32_t expected_crc; /* DS:6027 */
int16_t effect_parameter; /* DS:6025 */
uint32_t computed_crc; /* DS:6021 */
struct text_info saved_text_info; /* DS:6016 */
FILE *selfcheck_stream; /* DS:6002 */
FILE *configuration_file_stream; /* DS:5FFE */
char *default_config_name; /* DS:5FFA */
TrigPair trig_table[64]; /* DS:5DF2 */
int16_t draw_x; /* DS:5DF0 */
int16_t draw_y; /* DS:5DEE */
uint16_t palette_cycle_offset; /* DS:5DEC */
uint16_t original_ctrlbreak_setting; /* DS:5DEA */
uint16_t palette_buffer_bytes; /* DS:5DE8 */
uint16_t display_pan_state; /* DS:5DE6 */
uint16_t bios_resolution_mode; /* DS:5DE4 */
uint16_t palette_fixed_mode; /* DS:5DE2 */
uint16_t palette_alternate_mode; /* DS:5DE0 */
uint16_t hide_ega_border; /* DS:5DDE */
uint16_t option_y_requested; /* DS:5DDC */
uint16_t palette_random_mode; /* DS:5DDA */
uint16_t split_vga_palette_updates; /* DS:5DD8 */
uint16_t unused_palette_state; /* DS:5DD6 */
uint16_t use_8086_palette_io; /* DS:5DD4 */
uint16_t keep_typematic; /* DS:5DD2 */
uint16_t palette_four_ega_colors; /* DS:5DD0 */
uint16_t show_mode; /* DS:5DCE */
uint16_t no_disk_park; /* DS:5DCC */
uint16_t bashful_mode; /* DS:5DCA */
uint16_t insecure_argument_index; /* DS:5DC8 */
uint16_t draw_delay_count; /* DS:5DC6 */
uint16_t targa64_mode; /* DS:5DC4 */
uint16_t option_plus_requested; /* DS:5DC2 */
uint16_t color_index_mask; /* DS:5DC0 */
int16_t muted; /* DS:5DBE */
uint16_t window_mode; /* DS:5DBC */
uint16_t palette_entries; /* DS:5DBA */
uint16_t palette_cycle_wrap_bytes; /* DS:5DB8 */
uint16_t color_count; /* DS:5DB6 */
uint16_t use_bios_palette; /* DS:5DB4 */
int16_t screen_half_height; /* DS:5DB2 */
int16_t screen_half_width; /* DS:5DB0 */
int16_t screen_extent; /* DS:5DAE */
int16_t rectangles_angle_index; /* DS:5DAC */
int16_t ega_color_substep; /* DS:5DAA */
uint16_t auto_fade_pause_ticks; /* DS:5DA8 */
uint16_t palette_cycle_ticks; /* DS:5DA6 */
uint16_t palette_step; /* DS:5DA4 */
uint16_t default_palette_cycle_ticks; /* DS:5DA2 */
uint16_t semi_auto_mode; /* DS:5DA0 */
uint16_t semi_auto_continuous; /* DS:5D9E */
uint16_t alt_image_count; /* DS:5D9C */
uint16_t ctrl_image_count; /* DS:5D9A */
uint16_t fade_count; /* DS:5D98 */
uint8_t *palette_buffer; /* DS:5D94 */
uint16_t current_graphics_key; /* DS:5D92 */
uint16_t previous_graphics_key; /* DS:5D90 */
uint16_t startup_active; /* DS:5D8E */
int16_t ega_border_color; /* DS:5D8C */
uint16_t background_fill_word; /* DS:5D8A */
uint16_t maximum_image_size; /* DS:5D88 */
uint16_t show_duration_ticks; /* DS:5D86 */
uint16_t startup_config_failure; /* DS:5D84 */
uint16_t freeze_led_state; /* DS:5D82 */
uint16_t exit_requested; /* DS:5D80 */
uint16_t input_pending; /* DS:5D7E */
uint16_t mouse_abort_pending; /* DS:5D7C */
uint16_t palette_elapsed_ticks; /* DS:5D7A */
uint16_t countdown_ticks; /* DS:5D78 */
uint16_t unused_input_state; /* DS:5D76 */
uint16_t timer_hook_installed; /* DS:5D74 */
uint16_t keyboard_hook_installed; /* DS:5D72 */
uint16_t skip_ctrlbreak_control; /* DS:5D70 */
uint16_t palette_base_blue; /* DS:5D6E */
uint16_t palette_base_green; /* DS:5D6C */
int32_t resource_base; /* DS:5D68 */
uint8_t unused_path_state; /* DS:5D67 */
char executable_path[80]; /* DS:5D17 */
char startup_directory[80]; /* DS:5CC7 */
uint16_t minimum_images_per_display; /* DS:5CC5 */
uint16_t maximum_images_per_display; /* DS:5CC3 */
uint16_t current_image_index; /* DS:5CC1 */
uint16_t diagonal_symmetry_selected; /* DS:5CBF */
uint16_t image_work_limit; /* DS:5CBD */
int16_t image_work_count; /* DS:5CBB */
uint16_t mouse_callback_installed; /* DS:5CB9 */
uint16_t option_bang_requested; /* DS:5CB7 */
uint16_t mouse_available; /* DS:5CB5 */
uint16_t image_xor_mode; /* DS:5CB3 */
int16_t display_cleared; /* DS:5CB1 */
uint16_t draw_symmetry; /* DS:5CAF */
char configuration_filename[80]; /* DS:5C5F */
uint16_t *current_image_header; /* DS:5C5B */
void *linewipe_history; /* DS:5C57 */
uint8_t *ball_bitmap_pixels; /* DS:5C53 */
uint16_t ball_bitmap_width; /* DS:5C51 */
uint16_t ball_bitmap_height; /* DS:5C4F */
ImageDrawRoutine current_image_draw; /* DS:5C4B */
void (*palette_build_callback)(int16_t); /* DS:5C47 */
void (*mode_setup_callback)(uint16_t); /* DS:5C43 */
void (*plot_callback)(uint16_t); /* DS:5C3F */
uint16_t (*read_pixel_callback)(int16_t,int16_t); /* DS:5C3B */
void (*palette_upload_callback)(uint8_t *); /* DS:5C37 */
void (*display_start_callback)(uint16_t, uint16_t); /* DS:5C33 */
int16_t zigstream_step_y; /* DS:5C31 */
int16_t zigstream_step_x; /* DS:5C2F */
int16_t zigstream_direction; /* DS:5C2D */
int16_t zigstream_color_phase; /* DS:5C2B */
int16_t zigstream_run_length; /* DS:5C29 */
int16_t zigstream_run_counter; /* DS:5C27 */
int16_t wandering_line_done; /* DS:5C25 */
int16_t wandering_end_dy; /* DS:5C23 */
int16_t wandering_start_dy; /* DS:5C21 */
int16_t wandering_end_dx; /* DS:5C1F */
int16_t wandering_start_dx; /* DS:5C1D */
int16_t wandering_segments_left; /* DS:5C1B */
int16_t wandering_end_y; /* DS:5C19 */
int16_t wandering_end_x; /* DS:5C17 */
int16_t wandering_start_y; /* DS:5C15 */
int16_t wandering_start_x; /* DS:5C13 */
int16_t *tunnel_right_edge_ptr; /* DS:5C0F */
int16_t tunnel_right_limit; /* DS:5C0D */
int16_t tunnel_rows_left; /* DS:5C0B */
int16_t tunnel_row_left; /* DS:5C09 */
int16_t triangles_expand_left; /* DS:5C07 */
int16_t triangles_right_limit; /* DS:5C05 */
int16_t triangles_active; /* DS:5C03 */
int16_t triangles_row_left; /* DS:5C01 */
int16_t triangles_row_right; /* DS:5BFF */
int16_t sweep2_line_done; /* DS:5BFD */
int16_t sweep2_end_x_step; /* DS:5BFB */
int16_t sweep2_end_x; /* DS:5BF9 */
int16_t sweep2_end_y; /* DS:5BF7 */
int16_t sweep_rise_interval; /* DS:5BF5 */
int16_t sweep_rise_counter; /* DS:5BF3 */
int16_t spiral1_side_limit; /* DS:5BF1 */
int16_t spiral1_step_y; /* DS:5BEF */
int16_t spiral1_step_x; /* DS:5BED */
int16_t spiral1_direction; /* DS:5BEB */
int16_t spiral1_side_length; /* DS:5BE9 */
int16_t spiral1_side_step; /* DS:5BE7 */
int16_t spiral_step_y; /* DS:5BE5 */
int16_t spiral_step_x; /* DS:5BE3 */
int16_t spiral_direction; /* DS:5BE1 */
int16_t spiral_color_phase; /* DS:5BDF */
int16_t spiral_leg_length; /* DS:5BDD */
int16_t spiral_leg_step; /* DS:5BDB */
int16_t squares_side_limit; /* DS:5BD9 */
int16_t squares_step_y; /* DS:5BD7 */
int16_t squares_step_x; /* DS:5BD5 */
int16_t squares_direction; /* DS:5BD3 */
int16_t squares_side_length; /* DS:5BD1 */
int16_t squares_side_step; /* DS:5BCF */
float step_y; /* DS:5BCB */
float step_x; /* DS:5BC7 */
float mod_y; /* DS:5BC3 */
float mod_x; /* DS:5BBF */
float sine_phase; /* DS:5BBB */
float amplitude_y; /* DS:5BB7 */
float amplitude_x; /* DS:5BB3 */
int16_t sierpinski_fixed_color; /* DS:5BB1 */
Point16 sierpinski_vertices[3]; /* DS:5BA5 */
int16_t ribbons_run_limit; /* DS:5BA3 */
int16_t ribbons_run_length; /* DS:5BA1 */
int16_t ribbons_run_step; /* DS:5B9F */
char puffs_drift_step; /* DS:5B9E */
int16_t puffs_line_done; /* DS:5B9C */
int16_t puffs_spread; /* DS:5B9A */
int16_t puffs_end_y; /* DS:5B98 */
int16_t puffs_end_x; /* DS:5B96 */
int16_t puffs_origin_y; /* DS:5B94 */
int16_t puffs_origin_x; /* DS:5B92 */
int16_t pedals_stroke_index; /* DS:5B90 */
int16_t pedals_phase; /* DS:5B8E */
int16_t pedals_start_radius; /* DS:5B8C */
int16_t pedals_radius; /* DS:5B8A */
int16_t lines_base_y; /* DS:5B88 */
int16_t lines_step_y; /* DS:5B86 */
int16_t lines_step_x; /* DS:5B84 */
int16_t lines_stroke_index; /* DS:5B82 */
int16_t hilbert_restart; /* DS:5B80 */
int16_t hilbert_stroke_width; /* DS:5B7E */
int16_t hilbert_color_per_point; /* DS:5B7C */
int16_t hilbert_step_y; /* DS:5B7A */
int16_t hilbert_step_x; /* DS:5B78 */
int16_t hilbert_depth; /* DS:5B76 */
int16_t hilbert_target_y; /* DS:5B74 */
int16_t hilbert_target_x; /* DS:5B72 */
int16_t hilbert_previous_y; /* DS:5B70 */
int16_t hilbert_previous_x; /* DS:5B6E */
int16_t hexagon_start_x; /* DS:5B6C */
int16_t hexagon_vertical_limit; /* DS:5B6A */
uint16_t grid_next_y; /* DS:5B68 */
uint16_t grid_next_x; /* DS:5B66 */
int16_t grid_pattern_index; /* DS:5B64 */
int16_t grid_tile_left; /* DS:5B62 */
int16_t grid_tile_right; /* DS:5B60 */
int16_t formula1_color_shift; /* DS:5B5E */
int16_t formula1_y_square_factor; /* DS:5B5C */
int16_t formula1_x_shift; /* DS:5B5A */
int32_t g_long5b56; /* DS:5B56; ellipse twice the squared Y radius. */
int32_t g_long5b52; /* DS:5B52; ellipse squared Y radius. */
int32_t g_long5b4e; /* DS:5B4E; ellipse twice the squared X radius. */
int32_t g_long5b4a; /* DS:5B4A; ellipse squared X radius. */
int32_t g_long5b46; /* DS:5B46; ellipse Y radius. */
int32_t g_long5b42; /* DS:5B42; ellipse X radius. */
int32_t g_long5b3e; /* DS:5B3E; ellipse Y gradient: twice X radius squared times Y offset. */
int32_t g_long5b3a; /* DS:5B3A; ellipse X gradient: twice Y radius squared times X offset. */
int32_t g_long5b36; /* DS:5B36; ellipse midpoint decision error. */
int16_t ellipse_center_y; /* DS:5B34 */
int16_t ellipse_center_x; /* DS:5B32 */
int16_t ellipse_offset_y; /* DS:5B30 */
int16_t ellipse_offset_x; /* DS:5B2E */
int16_t ellipse_draw_mode; /* DS:5B2C */
int16_t ellipse_region; /* DS:5B2A */
int16_t ellipse_line_done; /* DS:5B28 */
int16_t diag2_color_period; /* DS:5B26 */
int16_t diag2_color_counter; /* DS:5B24 */
int32_t daisies_sine_table[10]; /* DS:5AFC */
int32_t daisies_cosine_table[10]; /* DS:5AD4 */
int32_t daisies_aspect_scale; /* DS:5AD0 */
int16_t daisies_sample_limit; /* DS:5ACE */
int16_t daisies_sample_start; /* DS:5ACC */
Point16 daisies_edge_points[66]; /* DS:59C4 */
int16_t daisies_base_color; /* DS:59C2 */
int16_t daisies_color_per_ray; /* DS:59C0 */
int16_t daisies_sample_step; /* DS:59BE */
int16_t daisies_radius_step; /* DS:59BC */
int16_t daisies_radius; /* DS:59BA */
int16_t daisies_arc_variant; /* DS:59B8 */
int16_t daisies_trig_index; /* DS:59B6 */
int16_t daisies_work_index; /* DS:59B4 */
int16_t daisies_edge_count; /* DS:59B2 */
int16_t daisies_line_done; /* DS:59B0 */
int16_t daisies_sample_index; /* DS:59AE */
int16_t daisies_center_y; /* DS:59AC */
int16_t daisies_center_x; /* DS:59AA */
int16_t daisies_edge_end_y; /* DS:59A8 */
int16_t daisies_edge_end_x; /* DS:59A6 */
int16_t daisies_saved_y; /* DS:59A4 */
int16_t daisies_saved_x; /* DS:59A2 */
int32_t cycloids_aspect_scale; /* DS:599E */
int16_t cycloids_next_curve; /* DS:599C */
int16_t cycloids_curve_index; /* DS:599A */
int16_t cycloids_line_done; /* DS:5998 */
int16_t cycloids_pen_y; /* DS:5996 */
int16_t cycloids_pen_x; /* DS:5994 */
int16_t cycloids_center_y; /* DS:5992 */
int16_t cycloids_center_x; /* DS:5990 */
int16_t cycloids_spin_step; /* DS:598E */
int16_t cycloids_orbit_step; /* DS:598C */
int16_t cycloids_phase_advance; /* DS:598A */
int16_t cycloids_pen_radius_step; /* DS:5988 */
int16_t cycloids_color_per_point; /* DS:5986 */
int16_t cycloids_curve_limit; /* DS:5984 */
int16_t cycloids_orbit_radius; /* DS:5982 */
int16_t cycloids_pen_radius; /* DS:5980 */
int16_t cycloids_rolling_radius; /* DS:597E */
int16_t cycloids_outer_radius; /* DS:597C */
int16_t cycloids_pen_phase; /* DS:597A */
int16_t cycloids_spin_phase; /* DS:5978 */
int16_t cycloids_orbit_phase; /* DS:5976 */
int16_t cycloids_start_phase; /* DS:5974 */
int16_t curves_amplitude; /* DS:5972 */
int16_t curves_phase_step_y; /* DS:5970 */
int16_t curves_phase_step_x; /* DS:596E */
int16_t curves_phase_y; /* DS:596C */
int16_t curves_phase_x; /* DS:596A */
int16_t curves_stroke_index; /* DS:5968 */
int16_t corkscrew_color_counter; /* DS:5966 */
int16_t corkscrew_origin_y; /* DS:5964 */
int16_t corkscrew_origin_x; /* DS:5962 */
float corkscrew_phase; /* DS:595E */
float corkscrew_growth_step; /* DS:595A */
float corkscrew_distance; /* DS:5956 */
float corkscrew_across; /* DS:5952 */
float corkscrew_along; /* DS:594E */
int16_t boxes_restart; /* DS:594C */
uint16_t boxes_edge_progress; /* DS:594A */
int16_t boxes_edge_state; /* DS:5948 */
int16_t boxes_line_done; /* DS:5946 */
int16_t boxes_target_y; /* DS:5944 */
int16_t boxes_target_x; /* DS:5942 */
int16_t boxes_height; /* DS:5940 */
int16_t boxes_width; /* DS:593E */
int16_t boxes_origin_y; /* DS:593C */
int16_t boxes_origin_x; /* DS:593A */
float bigstar_inner_radius; /* DS:5936 */
StarTracePoint bigstar_traces[175]; /* DS:53BE */
int16_t bigstar_base_color; /* DS:53BC */
int16_t bigstar_use_steep_trace; /* DS:53BA */
int16_t bigstar_trace_index; /* DS:53B8 */
int16_t bigstar_shallow_count; /* DS:53B6 */
int16_t bigstar_steep_count; /* DS:53B4 */
int16_t bigstar_color_mode; /* DS:53B2 */
int16_t bigstar_spacing_shift; /* DS:53B0 */
int16_t bigstar_trace_step; /* DS:53AE */
int16_t bigstar_resize_pending; /* DS:53AC */
int16_t bigstar_segment_state; /* DS:53AA */
int16_t bigstar_restart; /* DS:53A8 */
int16_t bigstar_line_done; /* DS:53A6 */
int16_t bigstar_target_y; /* DS:53A4 */
int16_t bigstar_target_x; /* DS:53A2 */
int16_t bigstar_diagonal_y; /* DS:53A0 */
int16_t bigstar_diagonal_x; /* DS:539E */
int16_t bigstar_axis_y; /* DS:539C */
int16_t bigstar_axis_x; /* DS:539A */
uint16_t bars_line_done; /* DS:5398 */
uint16_t bars_span_y; /* DS:5396 */
uint16_t bars_span_x; /* DS:5394 */
uint16_t bars_end_y; /* DS:5392 */
uint16_t bars_end_x; /* DS:5390 */
uint16_t bars_start_y; /* DS:538E */
uint16_t bars_start_x; /* DS:538C */
uint16_t balls_direction; /* DS:538A */
int16_t balls_step_y; /* DS:5388 */
int16_t balls_step_x; /* DS:5386 */
int16_t balls_run_length; /* DS:5384 */
int16_t balls_run_progress; /* DS:5382 */
int16_t balls_origin_y; /* DS:5380 */
int16_t balls_origin_x; /* DS:537E */
uint16_t balls_sample_x; /* DS:537C */
uint16_t balls_sample_y; /* DS:537A */
int16_t atom_stroke_index; /* DS:5378 */
int16_t atom_phase; /* DS:5376 */
int16_t atom_radius; /* DS:5374 */
int16_t arches_second_radius_x; /* DS:5372 */
int16_t arches_second_radius_y; /* DS:5370 */
int16_t arches_second_center_x; /* DS:536E */
int16_t arches_first_radius_x; /* DS:536C */
int16_t arches_first_radius_y; /* DS:536A */
int16_t arches_first_center_y; /* DS:5368 */
int16_t ellipse_trace_limit; /* DS:5366 */
int16_t ellipse_fill_color; /* DS:5364 */
int16_t ellipse_fill_mode; /* DS:5362 */
int16_t ellipse_fill_min_x; /* DS:5360 */
int16_t ellipse_fill_x; /* DS:535E */
int16_t ellipse_trace_done; /* DS:535C */
int16_t ellipse_direction_steps[700]; /* DS:4DE4 */
int16_t ellipse_trace_y; /* DS:4DE2 */
int16_t ellipse_trace_x; /* DS:4DE0 */
int16_t ellipse_trace_index; /* DS:4DDE */
int16_t ellipse_first_region; /* DS:4DDC */
int32_t ellipse_step_gradient_y; /* DS:4DD8 */
int32_t ellipse_step_gradient_x; /* DS:4DD4 */
int32_t ellipse_step_error; /* DS:4DD0 */
int32_t ellipse_step_twice_y_sq; /* DS:4DCC */
int32_t ellipse_step_twice_x_sq; /* DS:4DC8 */
int32_t ellipse_step_radius_y_sq; /* DS:4DC4 */
int32_t ellipse_step_radius_x_sq; /* DS:4DC0 */
int32_t ellipse_step_radius_y; /* DS:4DBC */
int32_t ellipse_step_radius_x; /* DS:4DB8 */
int16_t polygon_row_limit; /* DS:4DB6 */
int16_t polygon_origin_y; /* DS:4DB4 */
int16_t polygon_origin_x; /* DS:4DB2 */
int16_t polygon_min_y; /* DS:4DB0 */
int16_t polygon_max_y; /* DS:4DAE */
int16_t polygon_row_right[350]; /* DS:4AF2 */
int16_t polygon_row_left[350]; /* DS:4836 */
int16_t error; /* DS:4834 */
int16_t sy; /* DS:4832 */
int16_t sx; /* DS:4830 */
int16_t dy; /* DS:482E */
int16_t dx; /* DS:482C */

