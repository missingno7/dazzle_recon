/* DAZZLE 5.0 main module (DZLE), BC++ 3.00 -ml -O1. */
#include <stdlib.h>
#include <dos.h>
#include <math.h>
#include <stdio.h>
#include <conio.h>
#include <bios.h>
#include <dir.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <signal.h>

#include "dazzle.h"

/* Initialized state at DS:0094; literal pool follows the record tables. */
unsigned _stklen=0x3000;
char far *executable_name_pointer="DAZZLE.EXE";
char far *help_program_name="DAZZLE";
char far *program_revision="5.0";
char far *registration_notice_pointer="Comments and inquires about updates and other products are welcome at\n\rour mailing address. $15 ($20US for non-U.S.A. sites) registration payable\n\rto MicroTronics required after 10 days usage, which makes you eligible\n\rfor an update disk and puts you on our PRIVATE mailing list.\n\rAdd $5 extra for a printed manual. DOS & Windows Screen Savers available\n\rfor an additional cost.\n\r";
int direction_x[4]={0,-1,0,1};
int direction_y[4]={1,0,-1,0};
ConeDirections cone_directions[4]={
    {{-1,0,-2},{1,2,0}},
    {{-1,0,-2},{-1,-2,0}},
    {{1,0,2},{-1,-2,0}},
    {{1,0,2},{1,2,0}}
};
/* Four 8 by 8 grid masks, stored in row order. */
unsigned char grid_patterns[256]={
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
int diamond_x[4]={1,1,-1,-1};
int diamond_y[4]={1,-1,-1,1};
int zigstream_dx[2]={0,1};
int zigstream_dy[2]={1,0};

/* objcmp: {"function":"near_cs","address":"066a:000b","size":3,"segments":{"_DATA":"29bf:0094","_BSS":"29bf:482c"},"symbols":{"_butterflies":"0b84:0003","_diamonds":"0b84:02ef","_gumballs":"0b84:0726","_tiles":"0b84:0a22","_wedges":"0b84:0e1b","_yarnball":"0b84:113f","_executable_name_pointer":"DS:0096","_fade_records":"DS:0474","_background_fill_word":"DS:5d8a","_g_5d92":"DS:5d92","_palette_buffer":"DS:5d94","_image_source_bytes":"DS:7456","_resource_base":"DS:5d68","_selfcheck_stream":"DS:6002","_mode_setup_callback":"DS:5c43","_saved_text_info":"DS:6016","__streams":"DS:43f4","_saved_keyboard_vector":"DS:5ff2","_saved_timer_vector":"DS:5ff6","_palette_base_green":"DS:5d6c"}} */
unsigned near_cs(void) { return _CS; }

/* 066a:000e; RELOCATED; profile dazzle (BC++ 3.0 -ml -O1).
 * Complete extent: 269 bytes. Bindings recovered from the original image.
 */
/* objcmp: {"function":"line_step","address":"066a:000e","size":269,"symbols":{"_draw_x":"DS:5df0","_draw_y":"DS:5dee","_dx":"DS:482c","_dy":"DS:482e","_sx":"DS:4830","_sy":"DS:4832","_error":"DS:4834"},"segments":{"_BSS":"29bf:482c"}} */
int line_step(int endx,int endy,int initialize) {
    if(initialize) {
        dx=endx-draw_x; dy=endy-draw_y;
        sx=dx>0?1:dx==0?0:-1;
        sy=dy>0?1:dy==0?0:-1;
        dx=abs(dx)<<1; dy=abs(dy)<<1;
        if(dx>=dy) error=dy-(dx>>1); else error=dx-(dy>>1);
        if(dx==0 && dy==0) return 1; else return 0;
    } else {
        if(dx>=dy) {
            if(error>=0) {error-=dx; draw_y+=sy;}
            draw_x+=sx; error+=dy;
            if(draw_x==endx) return 1; else return 0;
        } else {
            if(error>=0) {error-=dy; draw_x+=sx;}
            draw_y+=sy; error+=dx;
            if(draw_y==endy) return 1;
        }
    }
    return 0;
}

/* 066a:011b; RELOCATED; profile dazzle (BC++ 3.0 -ml -O1).
 * Complete extent: 336 bytes. Bindings recovered from the original image.
 */
/* objcmp: {"function":"choose_color","address":"066a:011b","size":336,"symbols":{"_draw_x":"DS:5df0","_draw_y":"DS:5dee","_video_adapter":"DS:7484","_ega_color_substep":"DS:5daa","_color_count":"DS:5db6","_plot_color":"DS:7488","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXURSH@":"0000:13d6"},"segments":{"_BSS":"29bf:482c"}} */
void choose_color(int kind) {
    unsigned long a,b;
    switch(kind) {
    default: if(video_adapter!=4) plot_color++; else {ega_color_substep++; if((ega_color_substep&=3)==0) plot_color++;} break;
    case 1: plot_color=random(color_count); break;
    case 2: plot_color=abs(draw_x); break;
    case 3: plot_color=abs(draw_y); break;
    case 4: plot_color=abs(draw_x)+abs(draw_y); break;
    case 5: a=draw_x*draw_x; b=draw_y*draw_y; a+=b; plot_color=(a>>5)&(color_count-1); break;
    case 6: a=draw_x*draw_x; b=draw_y*draw_y; a-=b; plot_color=(a>>5)&(color_count-1); break;
    case 7: b=(draw_y*draw_y)>>5; plot_color=b-abs(draw_x); break;
    case 8: a=abs(draw_x*draw_y)>>5; plot_color=a; break;
    }
    plot_color&=color_count-1;
    if(plot_color<3) plot_color=3;
}

/* 066a:026b; RELOCATED; profile dazzle (BC++ 3.0 -ml -O1).
 * Complete extent: 80 bytes. Bindings recovered from the original image.
 */
/* objcmp: {"function":"square","address":"066a:026b","size":80,"symbols":{"_draw_x":"DS:5df0","_draw_y":"DS:5dee","_plot_callback":"DS:5c3f","_palette_elapsed_ticks":"DS:5d7a","_palette_cycle_ticks":"DS:5da6","_cycle_palette":"11c1:2515"},"segments":{"_BSS":"29bf:482c"}} */
void square(int n,int symmetry) {
    int i,j;
    for(i=1;i<=n;i++) {
        for(j=1;j<=n;j++) { plot_callback(symmetry); draw_y++; }
        draw_x++; draw_y-=n;
        if(palette_elapsed_ticks>=palette_cycle_ticks) cycle_palette();
    }
    draw_x-=n;
}

/* objcmp: {"function":"draw_polygon","address":"066a:02bb","size":797,"symbols":{"_line_step":"066a:000e","_draw_x":"DS:5df0","_draw_y":"DS:5dee","_screen_half_width":"DS:5db0","_screen_half_height":"DS:5db2","_plot_color":"DS:7488","_color_count":"DS:5db6","_draw_symmetry":"DS:5caf","_draw_delay_count":"DS:5dc6","_g_4dae":"DS:4dae","_g_4db0":"DS:4db0","_g_4db2":"DS:4db2","_g_4db4":"DS:4db4","_g_4db6":"DS:4db6","_polygon_row_left":"DS:4836","_polygon_row_right":"DS:4af2","_plot_callback":"DS:5c3f","_cycle_palette":"11c1:2515","_palette_elapsed_ticks":"DS:5d7a","_palette_cycle_ticks":"DS:5da6","_read_pixel_callback":"DS:5c3b","_draw_ellipse":"066a:089b","_ellipse_step":"066a:05d8","_rand":"0000:1694","F_LDIV@":"0000:12e7","F_LXMUL@":"0000:11e3","_g_5dae":"DS:5dae","_g_6025":"DS:6025","_g_5942":"DS:5942","_g_5944":"DS:5944","_g_5946":"DS:5946","_g_5948":"DS:5948","_g_594a":"DS:594a","_g_594c":"DS:594c","_g_5968":"DS:5968","_g_596a":"DS:596a","_g_596c":"DS:596c","_g_596e":"DS:596e","_g_5970":"DS:5970","_g_5972":"DS:5972","_g_597c":"DS:597c","_g_597e":"DS:597e","_g_5980":"DS:5980","_g_5984":"DS:5984","_g_599a":"DS:599a","_g_5382":"DS:5382","_g_5384":"DS:5384","_g_5386":"DS:5386","_g_5388":"DS:5388","_g_538a":"DS:538a","_g_538c":"DS:538c","_g_538e":"DS:538e","_g_5390":"DS:5390","_g_5392":"DS:5392","_g_5394":"DS:5394","_g_5396":"DS:5396","_g_5398":"DS:5398","_g_5cbb":"DS:5cbb","_input_pending":"DS:5d7e","_exit_requested":"DS:5d80","_g_5d92":"DS:5d92","_poll_graphics_keys":"11c1:18e7","_ega_plot_symmetric_640x700":"1899:012d","_g_4db8":"DS:4db8","_g_4dba":"DS:4dba","_g_4dbc":"DS:4dbc","_g_4dbe":"DS:4dbe","_g_4dc0":"DS:4dc0","_g_4dc2":"DS:4dc2","_g_4dc4":"DS:4dc4","_g_4dc6":"DS:4dc6","_g_4dc8":"DS:4dc8","_g_4dca":"DS:4dca","_g_4dcc":"DS:4dcc","_g_4dce":"DS:4dce","_ellipse_direction_steps":"DS:4de4","_g_4dde":"DS:4dde","_g_4de0":"DS:4de0","_g_4de2":"DS:4de2","_g_535c":"DS:535c","_g_535e":"DS:535e","_g_5360":"DS:5360","_g_5362":"DS:5362","_g_5364":"DS:5364","_g_5366":"DS:5366","F_FTOL@":"0000:0fbe","F_LXRSH@":"0000:13b6","_g_06af":"DS:06af","_g_06b3":"DS:06b3","_g_5374":"DS:5374","_g_5376":"DS:5376","_g_5378":"DS:5378","_g_537a":"DS:537a","_g_537c":"DS:537c","_g_537e":"DS:537e","_g_5380":"DS:5380","_g_5c4f":"DS:5c4f","_g_5c51":"DS:5c51","_g_5c53":"DS:5c53","_color_index_mask":"DS:5dc0","_cos":"0000:0f6e","_sin":"0000:0e75","_direction_x":"DS:00a6","_direction_y":"DS:00ae","_g_593a":"DS:593a","_g_593c":"DS:593c","_g_593e":"DS:593e","_g_5940":"DS:5940","F_LXLSH@":"0000:1395","_g_00b6":"DS:00b6","_g_00bc":"DS:00bc","_g_5956":"DS:5956","_g_5958":"DS:5958","_g_595e":"DS:595e","_g_5960":"DS:5960","_g_5962":"DS:5962","_g_5964":"DS:5964","_g_5966":"DS:5966","_g_539a":"DS:539a","_g_539c":"DS:539c","_g_539e":"DS:539e","_g_53a0":"DS:53a0","_g_53a2":"DS:53a2","_g_53a4":"DS:53a4","_g_53a6":"DS:53a6","_g_53a8":"DS:53a8","_g_53aa":"DS:53aa","_g_53ac":"DS:53ac","_g_53ae":"DS:53ae","_g_53b0":"DS:53b0","_g_53b2":"DS:53b2","_g_53b4":"DS:53b4","_g_53b6":"DS:53b6","_g_53b8":"DS:53b8","_g_53ba":"DS:53ba","_g_53bc":"DS:53bc","_g_53be":"DS:53be","_g_53c2":"DS:53c2","_g_5974":"DS:5974","_g_5976":"DS:5976","_g_5978":"DS:5978","_g_597a":"DS:597a","_g_5982":"DS:5982","_g_5986":"DS:5986","_g_5988":"DS:5988","_g_598a":"DS:598a","_g_598c":"DS:598c","_g_598e":"DS:598e","_g_5990":"DS:5990","_g_5992":"DS:5992","_g_5994":"DS:5994","_g_5996":"DS:5996","_g_599e":"DS:599e","_g_59a0":"DS:59a0","_g_599c":"DS:599c","_g_59a2":"DS:59a2","_g_59a4":"DS:59a4","_g_59a6":"DS:59a6","_g_59a8":"DS:59a8","_g_59aa":"DS:59aa","_g_59ac":"DS:59ac","_g_59ae":"DS:59ae","_g_59b0":"DS:59b0","_g_59b2":"DS:59b2","_g_59b4":"DS:59b4","_g_59b6":"DS:59b6","_g_59b8":"DS:59b8","_g_59ba":"DS:59ba","_g_59bc":"DS:59bc","_g_59be":"DS:59be","_g_59c0":"DS:59c0","_g_59c2":"DS:59c2","_g_59c4":"DS:59c4","_g_5acc":"DS:5acc","_g_5ace":"DS:5ace","_g_5ad0":"DS:5ad0","_g_5ad2":"DS:5ad2","_g_5ad4":"DS:5ad4","_g_5afc":"DS:5afc","_g_4dd0":"DS:4dd0","_g_4dd4":"DS:4dd4","_g_4dd8":"DS:4dd8","_ellipse_first_region":"DS:4ddc"},"absolute_symbols":{"FIDRQQ":"0x5c32","FIWRQQ":"0xa23d"},"segments":{"_BSS":"29bf:482c"}} */
void draw_polygon(int rebuild,int n,int far *px,int far *py,unsigned fill_color,unsigned edge_color,unsigned flags) {
    int row_end,end_x,end_y,saved_fill,color_mode,done;
    int i;
    g_4db2=px[0];
    g_4db4=py[0];
    px[0]=py[0]=px[n]=py[n]=0;
    if(rebuild) {
        g_4dae=-screen_half_height;
        g_4db0=screen_half_height;
        for(i=0;i<n;i++) {
            if(py[i]>g_4dae) g_4dae=py[i];
            if(py[i]<g_4db0) g_4db0=py[i];
        }
        g_4db6=g_4dae-g_4db0;
        for(i=0;i<=g_4db6;i++) {
            polygon_row_left[i]=screen_half_width;
            polygon_row_right[i]=-screen_half_width;
        }
        for(i=0;i<n;i++) {
            draw_x=px[i]; draw_y=py[i]-g_4db0;
            end_x=px[i+1]; end_y=py[i+1]-g_4db0; done=1;
            do {
                done=line_step(end_x,end_y,done);
                if(draw_x<polygon_row_left[draw_y]) polygon_row_left[draw_y]=draw_x;
                if(draw_x>polygon_row_right[draw_y]) polygon_row_right[draw_y]=draw_x;
            } while(!done);
            if(palette_elapsed_ticks>=palette_cycle_ticks) cycle_palette();
        }
    }
    if(flags&2) {
        plot_color=fill_color;
        saved_fill=fill_color;
        color_mode=flags&12;
        draw_y=g_4db0+g_4db4;
        for(i=0;i<=g_4db6;i++) {
            if(color_mode==4) plot_color=saved_fill;
            draw_x=polygon_row_left[i]+g_4db2;
            row_end=polygon_row_right[i]+g_4db2;
            while(draw_x<=row_end) {
                (*plot_callback)(draw_symmetry);
                draw_x++;
                if(color_mode==4) { plot_color++; if(plot_color>=color_count) plot_color=3; }
                g_5cbb++;
                if(palette_elapsed_ticks>=palette_cycle_ticks) cycle_palette();
            }
            if(color_mode==8) { plot_color++; if(plot_color>=color_count) plot_color=3; }
            draw_y++;
        }
    }
    if(flags&1) {
        plot_color=edge_color;
        if(flags&16) color_mode=1;
        else color_mode=0;
        for(i=0;i<n;i++) {
            draw_x=px[i]+g_4db2; draw_y=py[i]+g_4db4;
            end_x=px[i+1]+g_4db2; end_y=py[i+1]+g_4db4; done=1;
            do {
                done=line_step(end_x,end_y,done);
                (*plot_callback)(draw_symmetry);
                if(color_mode) { plot_color++; if(plot_color>=color_count) plot_color=3; }
                g_5cbb++;
                if(palette_elapsed_ticks>=palette_cycle_ticks) cycle_palette();
            } while(!done);
        }
    }
    px[0]=g_4db2; py[0]=g_4db4;
}

/* objcmp: {"function":"ellipse_step","address":"066a:05d8","size":707,"absolute_symbols":{"FIDRQQ":"0x5c32"},"symbols":{"_g_06af":"DS:06af","_g_06b3":"DS:06b3"},"segments":{"_BSS":"29bf:482c"}} */
int ellipse_step(int radius_y,int radius_x,int initialize) {
    float aspect_factor;
    if(initialize) {
        g_4dbc=radius_y;
        if(radius_x) g_4db8=radius_x;
        else {
            aspect_factor=(double)screen_half_width/screen_half_height*0.75;
            g_4db8=g_4dbc*aspect_factor;
            if(g_4dbc*aspect_factor-(double)g_4db8>0.5) g_4db8++;
        }
        draw_y=radius_y;
        draw_x=0;
        g_4dc0=g_4db8*g_4db8;
        g_4dc4=g_4dbc*g_4dbc;
        g_4dc8=g_4dc0*2L;
        g_4dcc=g_4dc4*2L;
        g_4dd4=0;
        g_4dd8=g_4dc8*g_4dbc;
        g_4dd0=(g_4dc0>>2)-(g_4dc0*g_4dbc);
        ellipse_first_region=1;
        if(radius_y==0) return 1; else return 0;
    } else {
        if(ellipse_first_region) {
            g_4dd0+=g_4dd4+g_4dc4;
            g_4dd4+=g_4dcc;
            draw_x++;
            if(g_4dd0>0) {
                draw_y--;
                g_4dd8-=g_4dc8;
                g_4dd0-=g_4dd8;
            }
            if(g_4dd8<=g_4dd4) {
                g_4dd0+=((g_4dd4+g_4dd8)>>1)+((g_4dc4-g_4dc0)>>2);
                ellipse_first_region=0;
            }
            return 0;
        } else {
            g_4dd0+=-g_4dd8+g_4dc0;
            g_4dd8-=g_4dc8;
            draw_y--;
            if(g_4dd0<0) {
                g_4dd4+=g_4dcc;
                g_4dd0+=g_4dd4;
                draw_x++;
            }
            if(draw_y==0) return 1;
        }
    }
    return 0;
}

/* objcmp: {"function":"draw_ellipse","address":"066a:089b","size":778,"segments":{"_BSS":"29bf:482c"}} */
void draw_ellipse(int rebuild,int ry,int rx,int cx,int cy,unsigned fill,unsigned edge,unsigned flags) {
    if(rebuild) {
        draw_y=ry; g_4de2=ry;
        g_4de0=draw_x=g_4dde=0;
        g_535c=1;
        do {
            g_535c=ellipse_step(ry,rx,g_535c);
            if(draw_y==g_4de2) ellipse_direction_steps[g_4dde]=0;
            else if(draw_x>g_4de0) ellipse_direction_steps[g_4dde]=1;
            else ellipse_direction_steps[g_4dde]=-1;
            g_4de2=draw_y; g_4de0=draw_x; g_4dde++;
            if(palette_elapsed_ticks>=palette_cycle_ticks) cycle_palette();
        } while(g_535c==0);
        g_5366=g_4dde;
    }
    if(flags&2) {
        plot_color=fill; g_5364=fill;
        g_5362=flags&0x1c;
        g_4dde=1; g_4de0=0; g_4de2=ry;
        while(g_4dde<=g_5366) {
            while(ellipse_direction_steps[g_4dde]==0 && g_4dde<g_5366) {
                g_4de0++;
                g_4dde++;
            }
            g_535e=g_4de0-1;
            if(g_5362==8) g_5360=0;
            else g_5360=-g_535e;
            if(g_5362&0x0c) plot_color=g_5364;
            do {
                draw_x=cx+g_535e; draw_y=cy+g_4de2; (*plot_callback)(draw_symmetry);
                draw_x=cx+g_535e; draw_y=cy-g_4de2; (*plot_callback)(1);
                if(g_5362==8) {
                    draw_x=cx-g_535e; draw_y=cy-g_4de2; (*plot_callback)(draw_symmetry);
                    draw_x=cx-g_535e; draw_y=cy+g_4de2; (*plot_callback)(draw_symmetry);
                }
                if(g_5362&0x0c) {
                    plot_color++;
                    if(plot_color>=color_count) plot_color=3;
                }
                g_5cbb++;
                if(palette_elapsed_ticks>=palette_cycle_ticks) cycle_palette();
                g_535e--;
            } while(g_535e>=g_5360);
            if(g_5362==0x10) {
                plot_color++;
                if(plot_color>=color_count) plot_color=3;
            }
            if(ellipse_direction_steps[g_4dde]>0) g_4de0++;
            g_4de2--;
            g_4dde++;
        }
    }
    if(flags&1) {
        plot_color=edge;
        g_4dde=1; g_4de0=0; g_4de2=ry;
        while(g_4dde<=g_5366) {
            draw_x=cx+g_4de0; draw_y=cy+g_4de2; (*plot_callback)(draw_symmetry);
            draw_x=cx+g_4de0; draw_y=cy-g_4de2; (*plot_callback)(draw_symmetry);
            draw_x=cx-g_4de0; draw_y=cy-g_4de2; (*plot_callback)(draw_symmetry);
            draw_x=cx-g_4de0; draw_y=cy+g_4de2; (*plot_callback)(draw_symmetry);
            if(flags&0x20) {
                plot_color++;
                if(plot_color>=color_count) plot_color=3;
            }
            g_5cbb++;
            if(ellipse_direction_steps[g_4dde]==0) g_4de0++;
            else {
                if(ellipse_direction_steps[g_4dde]>0) g_4de0++;
                g_4de2--;
            }
            g_4dde++;
            if(palette_elapsed_ticks>=palette_cycle_ticks) cycle_palette();
        }
    }
}

/* objcmp: {"function":"swap_pixel_rectangles","address":"066a:0ba5","size":296,"segments":{"_BSS":"29bf:482c"}} */
void swap_pixel_rectangles(int flip,int x1,int y1,int x2,int y2,int width,int height) {
    volatile int row;
    int color1,color2,x1col,y1row,x2col,y2row,step,column;
    width--;
    step=1;
    if(FP_SEG(plot_callback)==FP_SEG(ega_plot_symmetric_640x700) && FP_OFF(plot_callback)==FP_OFF(ega_plot_symmetric_640x700)) {
        y1>>=1; y2>>=1; y1<<=1; y2<<=1;
        height-=2; height>>=1; height<<=1;
        step=2;
    } else height--;
    for(row=0;row<=height;row+=step) {
        y1row=y1+row;
        if(flip&1) y2row=y2+height-row;
        else y2row=y2+row;
        for(column=0;column<=width;column++) {
            x1col=x1+column;
            if(flip<2) x2col=x2+column;
            else x2col=x2+width-column;
            color1=(unsigned char)(*read_pixel_callback)(x1col,y1row);
            color2=(unsigned char)(*read_pixel_callback)(x2col,y2row);
            plot_color=color2; draw_x=x1col; draw_y=y1row; (*plot_callback)(1);
            plot_color=color1; draw_x=x2col; draw_y=y2row; (*plot_callback)(1);
        }
        if(palette_elapsed_ticks>=palette_cycle_ticks) cycle_palette();
        if(input_pending || exit_requested) if(poll_graphics_keys()) break;
        if(g_5d92==0x6e) break;
    }
}

/* objcmp: {"function":"arches","address":"066a:0ccd","size":153,"symbols":{"_screen_half_width":"DS:5db0","_screen_half_height":"DS:5db2","_g_5368":"DS:5368","_g_536a":"DS:536a","_g_536c":"DS:536c","_g_536e":"DS:536e","_g_5370":"DS:5370","_g_5372":"DS:5372","_rand":"0000:1694","F_LDIV@":"0000:12e7","_draw_ellipse":"066a:089b"},"segments":{"_BSS":"29bf:482c"}} */
void arches(int rebuild) {
    if(rebuild) {
        g_5368=g_536a=g_5370=screen_half_height-1;
        g_536e=g_536c=g_5372=screen_half_width-1;
        if(random(2)) { g_536a<<=1; g_5372<<=1; }
    }
    draw_ellipse(1,g_536a,g_536c,0,g_5368,0,3,0x21);
    draw_ellipse(1,g_5370,g_5372,g_536e,0,0,3,0x21);
    g_536a-=8; g_5372-=8;
}

/* objcmp: {"symbols":{"_pedals_phase_scale":"DS:06b7"},"function":"atom","address":"066a:0d66","size":304,"segments":{"_BSS":"29bf:482c"}} */
void atom(int rebuild) {
    float angle;
    if(rebuild || g_5374<=1) {
        g_5378=0xff;
        g_5376=0;
        g_5374=g_5dae/2;
        if(rebuild) plot_color=3&color_index_mask;
    }
    g_5378++;
    if((g_5378=g_5378&15)==0) {
        g_5376+=g_5dae/2-g_5374+45;
        if(g_5376>18*g_5dae || g_5376<0) {
            g_5374-=g_5dae/20;
            g_5376=0;
            plot_color++;
            if(plot_color>=color_count) plot_color=3;
        }
        angle=g_5376*0.0002;
        draw_x=g_5374*cos(angle);
        draw_y=g_5374*sin(angle);
        if(video_adapter==4) draw_y/=2;
    } else draw_x++;
}

/* objcmp: {"function":"balls","address":"066a:0e96","size":416,"segments":{"_BSS":"29bf:482c"}} */
void balls(int rebuild) {
    int pixel;
    if(rebuild || (g_537c>=g_5c51 && g_537a>=g_5c4f)) {
        g_5386=g_5388=g_5384=0;
            g_5382=-1; g_538a=5;
            g_5380=random(screen_half_height+(g_5c4f>>1))-g_5c4f;
            g_537e=random(screen_half_width+(g_5c51>>1))-g_5c51;
            g_537c=g_5c51>>1; g_537a=g_5c4f>>1;
            g_6025=random(3)+1;
    }
    do {
        if(g_5382++>=g_5384) {
            g_5382=1;
            if(++g_538a>3) {
                g_5384+=2;
                g_538a=0;
                g_537c=g_537c+g_6025;
                g_537a=g_537a-g_6025;
            }
            g_5386=direction_x[g_538a]*g_6025;
            g_5388=direction_y[g_538a]*g_6025;
        }
        g_537c+=g_5386; g_537a+=g_5388;
        pixel=g_5c53[g_537c*g_5c4f+g_537a];
    } while(pixel==0);
    if(g_537c>=g_5c51 || g_537a>=g_5c4f) return;
    plot_color=pixel&color_index_mask;
    draw_x=g_537e+g_537c/g_6025;
    draw_y=g_5380+g_537a/g_6025;
}

/* objcmp: {"function":"bars","address":"066a:1036","size":373,"segments":{"_BSS":"29bf:482c"}} */
void bars(int rebuild) {
    if(rebuild || draw_x>g_5dae || draw_y>g_5dae) {
        draw_x=g_538c=random(g_5dae/2); draw_y=g_538e=random(g_5dae/2);
        g_5394=random(g_5dae/6)+g_5dae/6;
        g_5396=random(g_5dae/6)+g_5dae/6;
        g_5398=1;
        g_5390=g_538c+g_5394; g_5392=g_538e+g_5396;
        g_6025=random(2);
    }
    g_5398=line_step(g_5390,g_5392,g_5398);
    if(g_5398) {
        choose_color(0);
        draw_x=g_538c; draw_y=g_538e;
        if(g_6025) g_538c++;
        else g_538e++;
        g_5390=g_538c+g_5394; g_5392=g_538e+g_5396;
    }
}

/* objcmp: {"symbols":{"_g_5936":"DS:5936","_g_594e":"DS:594e","_g_5952":"DS:5952","_g_595a":"DS:595a","_trig_table":"DS:5df2","_g_5998":"DS:5998","_g_06c3":"DS:06c3","_g_06c7":"DS:06c7","_g_06cf":"DS:06cf","_g_06d7":"DS:06d7","_g_06db":"DS:06db","_g_06df":"DS:06df","_g_06ef":"DS:06ef"},"function":"bigstar","address":"066a:11ab","size":1161,"segments":{"_BSS":"29bf:482c"}} */
void bigstar(int rebuild) {
    float slope;
    int end_x,end_y;
    if(rebuild || g_53a8) {
        g_53a8=0;
        g_539c=(screen_half_height-1)*1.5;
        g_539a=(screen_half_width-1)*1.25;
        g_53a0=g_539c*0.707;
        g_539e=g_539a*0.707;
        g_5936=((random(3)+3)*g_539c)*0.1;
        g_53b0=2;
        g_53ac=1;
    }
    if(g_5936>4.0) {
        if(g_53ac) {
            g_53ac=0;
            g_539a>>=1; g_539c>>=1; g_539e>>=1; g_53a0>>=1;
            g_5936=g_5936*0.5;
            g_53a2=screen_half_width*g_5936/screen_half_height*0.75;
            g_53a4=g_5936;
            slope=(random(3)+1)*0.25;
            if(--g_53b0<0) g_53b0=0;
            g_53ae=1<<g_53b0;
            g_53b8=0;
            g_53aa=1;
            if(video_adapter!=4) g_53b2=random(2); else g_53b2=2;
            g_53bc=random(color_count-1);
            plot_color=g_53bc&color_index_mask;
            end_x=g_53a2*slope;
            draw_x=draw_y=g_53b4=0;
            g_53a6=1;
            do {
                g_53a6=line_step(end_x,g_53a4,g_53a6);
                g_53be[g_53b4].x1=draw_x;
                g_53be[g_53b4].y1=draw_y;
                g_53b4++;
                if(palette_elapsed_ticks>=palette_cycle_ticks) cycle_palette();
            } while(!g_53a6);
            end_y=g_53a4*slope;
            draw_x=draw_y=g_53b6=0;
            g_53a6=1;
            do {
                g_53a6=line_step(g_53a2,end_y,g_53a6);
                g_53be[g_53b6].x2=draw_x;
                g_53be[g_53b6].y2=draw_y;
                g_53b6++;
                if(palette_elapsed_ticks>=palette_cycle_ticks) cycle_palette();
            } while(!g_53a6);
            g_53a6=1;
        }
        if(g_53a6) {
            if(g_53b8>=g_53b4) g_53aa|=2;
            if(g_53b2==1) plot_color=g_53bc&color_index_mask;
            switch(g_53aa) {
            case 1:
                g_53a2=0; g_53a4=g_539c; g_53aa=2; g_53ba=1;
                break;
            case 0: case 2:
                g_53a2=g_539e; g_53a4=g_53a0; g_53aa=3;
                break;
            case 3:
                g_53a2=g_539e; g_53a4=g_53a0; g_53aa=4; g_53ba=0;
                break;
            case 4: case 6:
                g_53a2=g_539a; g_53a4=0; g_53aa=1;
                break;
            }
            if(g_53ba) {
                draw_x=g_53be[g_53b8].x1; draw_y=g_53be[g_53b8].y1;
            } else {
                draw_x=g_53be[g_53b8].x2; draw_y=g_53be[g_53b8].y2;
            }
            if(g_53b8>=g_53b6) g_53aa&=1;
            if(g_53aa==1) {
                g_53b8+=g_53ae;
                if(g_53b2==0) { plot_color++; if(plot_color>=color_count) plot_color=3; }
                if(g_53b2==2) {
                    ega_color_substep++;
                    if((ega_color_substep&=3)==0) { plot_color++; if(plot_color>=color_count) plot_color=3; }
                }
            }
            if(g_53b8>=g_53b4 && g_53b8>=g_53b6) g_53ac=1;
        }
        g_53a6=line_step(g_53a2,g_53a4,g_53a6);
        if(g_53b2==1) { plot_color++; if(plot_color>=color_count) plot_color=3; }
        g_5cbb=1;
    } else {
        g_5cbb=0x7d00;
        g_53a8=1;
    }
}

/* objcmp: {"function":"boxes","address":"066a:1634","size":427,"segments":{"_BSS":"29bf:482c"}} */
void boxes(int rebuild) {
    if(rebuild || g_594c) {
        g_5942=random(g_5dae/3);
        g_5944=random(g_5dae/3);
        g_593a=g_5942; g_593c=g_5944;
        g_593e=random(g_5dae/4)+g_5dae/4;
        g_5940=random(g_5dae/4)+g_5dae/4;
        g_5946=1; g_594c=0; g_5948=1; g_594a=0;
        draw_x=g_593a; draw_y=g_593c;
        plot_color=3&color_index_mask;
    }
    g_5946=line_step(g_5942,g_5944,g_5946);
    choose_color(0);
    if(g_5946) {
        plot_color=3&color_index_mask;
        draw_x=g_593a; draw_y=g_593c;
        g_594a++;
        switch(g_5948-1) {
        case 0:
            g_5942+=g_593e;
            g_594a=0;
            g_5948++;
            break;
        case 1:
            g_5944++;
            if(g_594a>=g_5940) { g_594a=0; g_5948++; }
            break;
        case 2:
            g_5942--;
            if(g_594a>=g_593e) { g_594a=0; g_5948++; }
            break;
        case 3:
            g_5944-=g_5940;
            g_594a=0;
            g_5948++;
            break;
        default:
            g_594c=1;
            break;
        }
    }
}

/* objcmp: {"function":"cones","address":"066a:17df","size":467,"segments":{"_BSS":"29bf:482c"}} */
void cones(int rebuild) {
    int i,j,dx,dy,kind,palette_cycle_ticks;
    register int cx,cy,variant;
    (void)rebuild;
    plot_color=random(color_count);
    cx=random(screen_half_width/2);
    cy=random(screen_half_height/2);
    kind=random(4);
    variant=random(3);
    switch(kind) {
    case 0:
        cx=screen_half_width-cx;
        break;
    case 1:
        cx=screen_half_width-cx;
        /* The corner orientation also reverses the vertical coordinate. */
    case 2:
        cy=screen_half_height-cy;
        break;
    }
    dx=cone_directions[kind].dx[variant];
    dy=cone_directions[kind].dy[variant];
    if(variant && random(2)) { dx>>=1; dy>>=1; }
    i=0; j=0;
    palette_cycle_ticks=screen_half_height/16+random(screen_half_height/8);
    while(i<palette_cycle_ticks) {
        draw_ellipse(1,i,j,cx,cy,0,plot_color,1);
        plot_color++;
        if(plot_color>=color_count) plot_color=3;
        cx+=dx; cy+=dy;
        j++; i++;
    }
}

/* objcmp: {"function":"corkscrew","address":"066a:19b2","size":432,"segments":{"_BSS":"29bf:482c"}} */
void corkscrew(int rebuild) {
    if(rebuild || draw_x>g_5dae) {
        g_595e=0; g_5956=0;
        g_595a=(random(2)+1)*0.02;
        g_5962=random(g_5dae/2);
        g_5964=random(g_5dae/2);
        plot_color++;
        if(plot_color>=color_count) plot_color=3;
    }
    g_594e=g_5956*(1-cos(g_595e)/4);
    g_5952=((g_5956*g_5956)/g_5dae)*sin(g_595e);
    draw_x=g_5962+(int)(g_594e-g_5952);
    draw_y=g_5964+(int)(g_594e+g_5952);
    g_595e+=0.02;
    g_5956+=g_595a;
    g_5966++;
    if((g_5966=g_5966&15)==0) {
        plot_color++;
        if(plot_color>=color_count) plot_color=3;
    }
}

/* objcmp: {"function":"curves","address":"066a:1b62","size":352,"symbols":{"_g_06bf":"DS:06bf","_g_06e7":"DS:06e7"},"segments":{"_BSS":"29bf:482c"}} */
void curves(int rebuild) {
    float a,b;
    if(rebuild) {
        g_596a=(g_596c=1);
        g_5972=g_5dae/1.5;
        g_5968=255;
        g_596e=random(8)+2;
        g_5970=random(6)+4;
    }
    g_5968++;
    if((g_5968=g_5968&15)==0) {
        a=g_596a*0.001;
        b=g_596c*0.001;
        draw_x=(int)(cos(a)*g_5972)+4;
        draw_y=(int)(sin(b)*g_5972)+4;
        g_596a+=g_596e;
        g_596c+=g_5970;
        if(g_596a>32000) g_596a=0;
        if(g_596c>32000) g_596c=0;
        if((draw_y&2) && ++plot_color>=color_count) plot_color=3;
    } else {
        draw_x++;
    }
}

/* objcmp: {"function":"cycloids","address":"066a:1cc2","size":1107,"segments":{"_BSS":"29bf:482c"}} */
void cycloids(int rebuild) {
    int end_x,end_y;
    if(rebuild || g_599a>=g_5984) {
        g_599e=((long)screen_half_width*65536L/screen_half_height)*3L/4L;
        g_597c=screen_half_height*3/4;
        g_597e=random(g_597c*5/16)+g_597c/16;
        g_5980=random(g_597e)+g_597e/2;
        g_5982=g_597c-g_597e;
        g_598a=random(5);
        g_598c=1;
        g_598e=(g_597c/g_597e)*g_598c;
        g_5988=g_5980/8;
        if(random(2)) g_5988=-g_5988;
        g_5974=48;
        g_5976=g_5974;
        g_5978=0;
        g_599a=0;
        end_x=(g_5980+g_5982)*trig_table[g_5976].cosine;
        end_x=(g_599e*end_x)>>16;
        end_y=(g_5980+g_5982)*trig_table[g_5976].sine;
        g_5984=random(10)+1;
        plot_color=random(color_count-1)&color_index_mask;
        g_599c=0;
        g_5986=random(2);
    }
    if(g_599c) {
        g_599c=0;
        if(g_5986==0) { plot_color++; if(plot_color>=color_count) plot_color=3; }
        g_5980+=g_5988;
        if(g_5980<0) g_5980=-g_5980;
        g_5974=(g_5974+g_598a)%64;
        g_5976=g_5974;
        g_5978=0;
        end_x=(g_5980+g_5982)*trig_table[g_5976].cosine;
        end_x=(g_599e*end_x)>>16;
        end_y=(g_5980+g_5982)*trig_table[g_5976].sine;
        g_599a++;
    }
    do {
        g_5976=(g_5976+g_598c)%64;
        g_5978=(g_5978+g_598e)%64;
        g_597a=(g_5976-g_5978)%64;
        if(g_597a<0) g_597a+=64;
        g_5990=g_5982*trig_table[g_5976].cosine;
        g_5992=g_5982*trig_table[g_5976].sine;
        g_5994=g_5980*trig_table[g_597a].cosine;
        g_5996=g_5980*trig_table[g_597a].sine;
        draw_x=end_x; draw_y=end_y;
        end_x=(g_599e*(g_5990+g_5994))>>16;
        end_y=g_5992+g_5996;
        g_5998=1;
        do {
            g_5998=line_step(end_x,end_y,g_5998);
            (*plot_callback)(0);
            g_5cbb++;
            if(g_5986) { plot_color++; if(plot_color>=color_count) plot_color=3; }
            if(palette_elapsed_ticks>=palette_cycle_ticks) cycle_palette();
        } while(!g_5998);
    } while(g_5976!=g_5974 || g_597a!=g_5974);
    g_599c=1;
    draw_x=draw_y=g_5dae+1;
}

/* objcmp: {"function":"daisies","address":"066a:2115","size":1238,"segments":{"_BSS":"29bf:482c"}} */
void daisies(int rebuild) {
    if(rebuild || g_59ae>=g_5ace) {
        g_5ad0=((long)screen_half_width*65536L/screen_half_height)*3L/4L;
        g_59aa=random(g_5dae/2)+g_5dae/4;
        g_59ac=random(g_5dae/2)+g_5dae/4;
        g_59ba=random(g_5dae/3)+g_5dae/6;
        g_59bc=g_59ba>>(random(3)+1);
        g_59be=1;
        if(random(3)) { g_5acc=0; g_5ace=9; }
        else {
            g_59b8=random(2);
            g_5acc=g_59b8+2;
            g_5ace=7-g_59b8;
            if(g_59b8==0) g_59be+=random(2);
        }
        g_59c0=random(2);
        g_59c2=random(color_count);
        for(g_59b4=0;g_59b4<=9;g_59b4++) {
            if(g_59b4>1 && g_59b4<9) g_59b6=g_59b4*2-1;
            else if(g_59b4<=1) g_59b6=g_59b4;
            else g_59b6=16;
            g_5ad4[g_59b4]=(long)(trig_table[g_59b6].cosine*65536.0);
            g_5afc[g_59b4]=(long)(trig_table[g_59b6].sine*65536.0);
        }
        g_59ae=g_5acc;
    }
    draw_x=(g_5afc[g_59ae]*g_59ba)>>16;
    draw_y=(g_5ad4[g_59ae]*g_59ba)>>16;
    g_59ae+=g_59be;
    g_59a6=(g_5afc[g_59ae]*g_59ba)>>16;
    g_59a8=(g_5ad4[g_59ae]*g_59ba)>>16;
    g_59b2=0;
    g_59b0=1;
    do {
        g_59b0=line_step(g_59a6,g_59a8,g_59b0);
        g_59c4[g_59b2].x=(g_5ad0*draw_x)>>16;
        g_59c4[g_59b2].y=draw_y;
        g_59b2++;
        if(palette_elapsed_ticks>=palette_cycle_ticks) cycle_palette();
    } while(!g_59b0);
    for(g_59b4=0;g_59b4<g_59b2;g_59b4++) {
        draw_x=g_59c4[g_59b4].x;
        draw_y=g_59c4[g_59b4].y;
        g_59b0=1;
        if(g_59c0) { plot_color++; if(plot_color>=color_count) plot_color=3; }
        else plot_color=g_59c2&color_index_mask;
        do {
            g_59b0=line_step(0,0,g_59b0);
            g_59a2=draw_x; g_59a4=draw_y;
            draw_x=g_59aa+g_59a2; draw_y=g_59ac+g_59a4; (*plot_callback)(1);
            draw_x=g_59aa-g_59a2; draw_y=g_59ac-g_59a4; (*plot_callback)(1);
            draw_x=g_59aa+g_59a2; draw_y=g_59ac-g_59a4; (*plot_callback)(1);
            draw_x=g_59aa-g_59a2; draw_y=g_59ac+g_59a4; (*plot_callback)(1);
            draw_x=g_59a2; draw_y=g_59a4;
            g_5cbb++;
            if(g_59c0==0) { plot_color++; if(plot_color>=color_count) plot_color=3; }
            if(palette_elapsed_ticks>=palette_cycle_ticks) cycle_palette();
        } while(!g_59b0);
    }
    g_59bc=-g_59bc;
    g_59ba+=g_59bc;
    draw_x=g_59aa;
    draw_y=g_59ac;
}

/* DS:06B7: 0.0002 */
/* DS:06F3: 0.01 */
/* DS:06FB/0703 */
/* objcmp: {"function":"diag1","address":"066a:25eb","size":172,"symbols":{"_draw_x":"DS:5df0","_draw_y":"DS:5dee","_g_5dae":"DS:5dae","_g_6025":"DS:6025","_choose_color":"066a:011b","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7"},"segments":{"_BSS":"29bf:482c"}} */
void diag1(int size) {
    --draw_y;
    if (size || draw_x++ > g_5dae - 1) {
        draw_x = 0;
        ++draw_y;
        if (size) {
            g_6025 = (int)((long)rand() * 9L / 32768L);
            if (g_6025 == 1)
                --g_6025;
            if ((int)((long)rand() * 2L / 32768L) != 0)
                g_6025 = 0;
        }
        if (g_6025 == 0)
            choose_color(0);
    }
    if (draw_y < 0)
        draw_y = g_5dae;
    if (g_6025 != 0)
        choose_color(g_6025);
}

/* objcmp: {"function":"diag2","address":"066a:2697","size":185,"symbols":{"_draw_x":"DS:5df0","_draw_y":"DS:5dee","_g_5b24":"DS:5b24","_g_5b26":"DS:5b26","_g_5dae":"DS:5dae","_color_index_mask":"DS:5dc0","_g_6025":"DS:6025","_plot_color":"DS:7488","_choose_color":"066a:011b","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7"},"segments":{"_BSS":"29bf:482c"}} */
void diag2(int size) {
    register int mode = size;
    if (mode) {
        g_5b24 = g_5b26 = 0;
        g_6025 = (int)((long)rand() * 2L / 32768L);
    }
    ++draw_y;
    if (g_6025 != 0) {
        if (draw_x++ >= g_5dae) {
            draw_x = 0;
            --draw_y;
            plot_color = 4 & color_index_mask;
            if (++g_5b26 > 20)
                g_5b26 = 0;
        }
        if (g_5b24++ > g_5b26) {
            g_5b24 = 0;
            choose_color(0);
        }
    } else if (mode || draw_x++ > g_5dae - 1) {
        draw_x = 0;
        --draw_y;
        choose_color(0);
    }
    if (draw_y > g_5dae)
        draw_y = 0;
}

/* objcmp: {"function":"ellipse","address":"066a:2750","size":1548,"symbols":{"_draw_x":"DS:5df0","_draw_y":"DS:5dee","_g_5b2a":"DS:5b2a","_g_5b2c":"DS:5b2c","_g_5b2e":"DS:5b2e","_g_5b30":"DS:5b30","_g_5b32":"DS:5b32","_g_5b34":"DS:5b34","_g_6025":"DS:6025","_g_5dae":"DS:5dae","_color_index_mask":"DS:5dc0","_color_count":"DS:5db6","_plot_color":"DS:7488","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395","_choose_color":"066a:011b","_g_long5b36":"DS:5b36","_g_long5b3a":"DS:5b3a","_g_long5b3e":"DS:5b3e","_g_long5b42":"DS:5b42","_g_long5b46":"DS:5b46","_g_long5b4a":"DS:5b4a","_g_long5b4e":"DS:5b4e","_g_long5b52":"DS:5b52","_g_long5b56":"DS:5b56","_g_5b28":"DS:5b28","_g_5cbb":"DS:5cbb","_palette_elapsed_ticks":"DS:5d7a","_palette_cycle_ticks":"DS:5da6","_plot_callback":"DS:5c3f","_cycle_palette":"11c1:2515","_line_step":"066a:000e"},"segments":{"_BSS":"29bf:482c"}} */
void ellipse(int size) {
    if (size || (unsigned)g_5b2a >= 2) {
        g_5b2a = 0;
        g_6025 = (int)((long)rand() * 2L / 32768L);
        g_5b2c = (int)((long)rand() * 3L / 32768L);
        g_5b2e = 0;
        if (g_5b2c == 0) {
            g_5b32 = 0;
            g_5b34 = 0;
        } else {
            g_5b32 = (int)((long)rand() * (g_5dae / 2) / 32768L) + g_5dae / 4;
            g_5b34 = (int)((long)rand() * (g_5dae / 2) / 32768L) + g_5dae / 4;
        }
        g_long5b42 = (int)((long)rand() * (g_5dae >> 1) / 32768L) + (g_5dae >> 4);
        g_5b30 = (int)((long)rand() * (g_5dae >> 1) / 32768L) + (g_5dae >> 4);
        g_long5b46 = g_5b30;
        g_long5b4a = g_long5b42 * g_long5b42;
        g_long5b4e = g_long5b4a << 1;
        g_long5b52 = g_long5b46 * g_long5b46;
        g_long5b56 = g_long5b52 << 1;
        g_long5b36 = g_long5b52 - g_long5b4a * g_long5b46 + g_long5b4a / 4L;
        g_long5b3a = 0L;
        g_long5b3e = g_long5b4e * g_long5b46;
        plot_color = 3 & color_index_mask;
    }
    switch (g_5b2a) {
    case 0:
        if (g_long5b3a < g_long5b3e) {
            if (g_6025) {
                if (++plot_color >= color_count)
                    plot_color = 3;
            } else {
                plot_color = (g_5b2e + g_5b30) & color_index_mask;
            }
            if ((unsigned)g_5b2c < 2) {
                draw_x = g_5b32;
                draw_y = g_5b34;
                g_5b28 = 1;
                do {
                    g_5b28 = line_step(g_5b32 + g_5b2e, g_5b34 + g_5b30, g_5b28);
                    ++g_5cbb;
                    plot_callback(1);
                    if (!g_6025 && ++plot_color >= color_count)
                        plot_color = 3;
                } while (!g_5b28);
            } else {
                draw_x = g_5b32 - g_5b2e;
                draw_y = g_5b34 - g_5b30;
                g_5b28 = 1;
                do {
                    g_5b28 = line_step(g_5b32 + g_5b2e, g_5b34 + g_5b30, g_5b28);
                    ++g_5cbb;
                    plot_callback(1);
                    if (!g_6025 && ++plot_color >= color_count)
                        plot_color = 3;
                } while (!g_5b28);
                draw_x = g_5b32 - g_5b2e;
                draw_y = g_5b34 + g_5b30;
                do {
                    g_5b28 = line_step(g_5b32 + g_5b2e, g_5b34 - g_5b30, g_5b28);
                    plot_callback(1);
                    if (!g_6025 && ++plot_color >= color_count)
                        plot_color = 3;
                } while (!g_5b28);
            }
            if (g_long5b36 > 0L) {
                --g_5b30;
                g_long5b3e -= g_long5b4e;
                g_long5b36 -= g_long5b3e;
            }
            ++g_5b2e;
            g_long5b3a += g_long5b56;
            g_long5b36 += g_long5b52 + g_long5b3a;
            break;
        }
        ++g_5b2a;
        g_long5b36 += (3L * (g_long5b4a - g_long5b52) / 2L
                      - (g_long5b3a + g_long5b3e)) / 2L;
        /* Continue directly with the second region of the ellipse. */
    case 1:
        if (g_5b30 >= 0) {
            if (g_6025) {
                if (++plot_color >= color_count)
                    plot_color = 3;
            } else {
                plot_color = (g_5b2e + g_5b30) & color_index_mask;
            }
            if ((unsigned)g_5b2c < 2) {
                draw_x = g_5b32;
                draw_y = g_5b34;
                g_5b28 = 1;
                do {
                    g_5b28 = line_step(g_5b32 + g_5b2e, g_5b34 + g_5b30, g_5b28);
                    ++g_5cbb;
                    plot_callback(1);
                    if (!g_6025 && ++plot_color >= color_count)
                        plot_color = 3;
                } while (!g_5b28);
            } else {
                draw_x = g_5b32 - g_5b2e;
                draw_y = g_5b34 - g_5b30;
                g_5b28 = 1;
                do {
                    g_5b28 = line_step(g_5b32 + g_5b2e, g_5b34 + g_5b30, g_5b28);
                    ++g_5cbb;
                    plot_callback(1);
                    if (!g_6025 && ++plot_color >= color_count)
                        plot_color = 3;
                } while (!g_5b28);
                draw_x = g_5b32 - g_5b2e;
                draw_y = g_5b34 + g_5b30;
                do {
                    g_5b28 = line_step(g_5b32 + g_5b2e, g_5b34 - g_5b30, g_5b28);
                    plot_callback(1);
                    if (!g_6025 && ++plot_color >= color_count)
                        plot_color = 3;
                } while (!g_5b28);
            }
            if (g_long5b36 < 0L) {
                ++g_5b2e;
                g_long5b3a += g_long5b56;
                g_long5b36 += g_long5b3a;
            }
            --g_5b30;
            g_long5b3e -= g_long5b4e;
            g_long5b36 += g_long5b4a - g_long5b3e;
        } else {
            ++g_5b2a;
        }
    }
}

/* objcmp: {"function":"formula1","address":"066a:2d5c","size":190,"symbols":{"_draw_x":"DS:5df0","_draw_y":"DS:5dee","_g_5b5a":"DS:5b5a","_g_5b5c":"DS:5b5c","_g_5b5e":"DS:5b5e","_g_5dae":"DS:5dae","_color_index_mask":"DS:5dc0","_plot_color":"DS:7488","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395"},"segments":{"_BSS":"29bf:482c"}} */
void formula1(int size) {
    if (size || draw_x > g_5dae) {
        draw_x = 0;
        draw_y = -1;
        g_5b5a = (int)((long)rand() * 2L / 32768L) + 1;
        g_5b5c = (int)((long)rand() * 3L / 32768L) + 1;
        g_5b5e = (int)((long)rand() * 4L / 32768L) + 2;
    }
    if (draw_y++ > draw_x) {
        ++draw_x;
        draw_y = 0;
    }
    plot_color = (draw_y * draw_y * g_5b5c - (draw_x << g_5b5a)) >> g_5b5e & color_index_mask;
}

/* objcmp: {"function":"grid","address":"066a:2e1a","size":313,"symbols":{"_draw_x":"DS:5df0","_draw_y":"DS:5dee","_g_5b60":"DS:5b60","_g_5b62":"DS:5b62","_g_5b64":"DS:5b64","_g_5b66":"DS:5b66","_g_5b68":"DS:5b68","_grid_patterns":"DS:00e6","_g_5dae":"DS:5dae","_color_index_mask":"DS:5dc0","_color_count":"DS:5db6","_g_6025":"DS:6025","_plot_color":"DS:7488","_rand":"0000:1694","_rgb8_to_ega_palette":"1899:0006","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395"},"segments":{"_BSS":"29bf:482c"}} */
void grid(int size) {
    if (size) {
        plot_color = (int)(((long)rand() << 2) / 32768L) & color_index_mask;
        g_5b64 = g_5b66 = g_5b68 = 0;
        g_6025 = (int)((long)rand() * 3L / 32768L);
    }
    do {
        if ((g_5b64 & 0x3f) == 0) {
            if (g_6025 == 0 && ++plot_color >= color_count)
                plot_color = 3;
            g_5b62 = g_5b66;
            draw_x = g_5b66 - 1;
            if (FP_SEG(plot_callback) == FP_SEG(rgb8_to_ega_palette) && FP_OFF(plot_callback) == 0x012d)
                draw_y = g_5b68 << 1;
            else
                draw_y = g_5b68;
            g_5b60 = g_5b62 + 7;
            g_5b64 = ((g_5b66 + g_5b68) & 0x18) << 3;
            g_5b66 += 8;
            if (g_5b66 >= 0x60) {
                if (++plot_color >= color_count)
                    plot_color = 3;
                g_5b66 = 0;
                g_5b68 += 8;
                if (g_5b68 > 0x60)
                    g_5b68 = 0;
            }
        }
        if (++draw_x > g_5b60) {
            draw_x = g_5b62;
            ++draw_y;
            if (FP_SEG(plot_callback) == FP_SEG(rgb8_to_ega_palette) && FP_OFF(plot_callback) == 0x012d)
                ++draw_y;
        }
    } while (grid_patterns[g_5b64++] != 0);
}

/* objcmp: {"function":"hexagon","address":"066a:2f53","size":119,"symbols":{"_draw_x":"DS:5df0","_draw_y":"DS:5dee","_g_5b6a":"DS:5b6a","_g_5b6c":"DS:5b6c","_g_5dae":"DS:5dae","_color_count":"DS:5db6","_plot_color":"DS:7488","_choose_color":"066a:011b"},"segments":{"_BSS":"29bf:482c"}} */
void hexagon(int size) {
    if (size || (draw_x > g_5dae && draw_y > g_5dae)) {
        g_5b6c = g_5b6a = 1;
        draw_x = draw_y = 0;
        if (++plot_color >= color_count)
            plot_color = 3;
    }
    if (draw_x < 0) {
        draw_y = 0;
        ++g_5b6c;
        draw_x = g_5b6c;
        ++g_5b6a;
        choose_color(0);
    } else {
        ++draw_y;
        if (draw_y > g_5b6a)
            --draw_x;
    }
}

/* objcmp: {"function":"hilbert","address":"066a:2fca","size":715,"symbols":{"_draw_x":"DS:5df0","_draw_y":"DS:5dee","_g_5b6e":"DS:5b6e","_g_5b70":"DS:5b70","_g_5b72":"DS:5b72","_g_5b74":"DS:5b74","_g_5b76":"DS:5b76","_g_5b78":"DS:5b78","_g_5b7a":"DS:5b7a","_g_5b7c":"DS:5b7c","_g_5b7e":"DS:5b7e","_g_5b80":"DS:5b80","_g_5dae":"DS:5dae","_palette_cycle_ticks":"DS:5da6","_palette_elapsed_ticks":"DS:5d7a","_color_index_mask":"DS:5dc0","_color_count":"DS:5db6","_g_5cbb":"DS:5cbb","_plot_color":"DS:7488","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395","_line_step":"066a:000e","_hilbert":"066a:2fca","_cycle_palette":"11c1:2515","_square":"066a:026b","_plot_callback":"DS:5c3f"},"segments":{"_BSS":"29bf:482c"}} */
void hilbert(int size) {
    int initialize, direction, column, mode, color, step;
    register int dx, dy;
    if ((unsigned)palette_elapsed_ticks >= (unsigned)palette_cycle_ticks)
        cycle_palette();
    if (size || g_5b80) {
        g_5b80 = 0;
        dx = g_5dae >> 1;
        step = random(3);
        g_5b76 = step + 3;
        g_5b7e = 6 >> step;
        g_5b78 = dx / (2 << (g_5b76 - 1));
        g_5b7a = 0;
        g_5b6e = g_5b70 = g_5b72 = g_5b74 = 0;
        g_5b7c = random(2);
        if (size && random(2)) {
            draw_x = draw_y = 0;
            mode = random(2);
            if (mode) {
                if (++plot_color >= color_count)
                    plot_color = 3;
            } else {
                color = random(3) + 2;
            }
            for (dy = 1; dy <= dx; ++dy) {
                for (column = 1; column <= dx; ++column) {
                    if (!mode)
                        choose_color(color);
                    plot_callback(1);
                    ++draw_y;
                }
                ++draw_x;
                draw_y -= dx;
                if ((unsigned)palette_elapsed_ticks >= (unsigned)palette_cycle_ticks)
                    cycle_palette();
            }
            set_ega_raster_xor(0);
        }
        hilbert(0);
        g_5cbb = 32000;
        g_5b80 = 1;
    } else {
        --g_5b76;
        if (g_5b76) {
            step = g_5b78;
            g_5b78 = g_5b7a;
            g_5b7a = step;
            hilbert(0);
            step = g_5b7a;
            g_5b7a = g_5b78;
            g_5b78 = step;
        }
        for (direction = 1; direction <= 3; ++direction) {
            switch (direction) {
            case 1: dx = g_5b78; dy = g_5b7a; break;
            case 2: dx = g_5b7a; dy = g_5b78; break;
            case 3:
                dx = -g_5b78;
                dy = -g_5b7a;
                g_5b78 = dy;
                g_5b7a = dx;
                break;
            }
            g_5b72 += dx;
            g_5b74 += dy;
            draw_x = g_5b6e;
            draw_y = g_5b70;
            initialize = 1;
            do {
                initialize = line_step(g_5b72, g_5b74, initialize);
                square(g_5b7e, 1);
                if (g_5b7c && ++plot_color >= color_count)
                    plot_color = 3;
            } while (!initialize);
            if (!g_5b7c && ++plot_color >= color_count)
                plot_color = 3;
            g_5b6e = g_5b72;
            g_5b70 = g_5b74;
            if (g_5b76)
                hilbert(0);
        }
        g_5b78 = -dx;
        g_5b7a = -dy;
        ++g_5b76;
    }
}

/* objcmp: {"function":"lines","address":"066a:3295","size":266,"symbols":{"_draw_x":"DS:5df0","_draw_y":"DS:5dee","_g_5b82":"DS:5b82","_g_5b84":"DS:5b84","_g_5b86":"DS:5b86","_g_5b88":"DS:5b88","_g_5dae":"DS:5dae","_color_index_mask":"DS:5dc0","_color_count":"DS:5db6","_plot_color":"DS:7488","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395"},"segments":{"_BSS":"29bf:482c"}} */
void lines(int size) {
    if (size) {
        g_5b82 = 255;
        g_5b88 = 0;
        g_5b84 = g_5b86 = 1;
        plot_color = (int)(((long)rand() << 2) / 32768L) & color_index_mask;
    }
    ++g_5b82;
    if ((g_5b82 = g_5b82 & 3) == 0) {
        draw_x += g_5b84;
        g_5b88 += g_5b86;
        if (draw_x < 0) {
            g_5b84 = 1;
            draw_x = 0;
            if (++plot_color >= color_count)
                plot_color = 3;
        }
        if (g_5b88 < 0) {
            g_5b86 = (int)(((long)rand() << 2) / 32768L) + 1;
            g_5b88 = 0;
        }
        if (draw_x > g_5dae) {
            g_5b84 = -1;
            draw_x = g_5dae;
        }
        if (g_5b88 > g_5dae) {
            g_5b86 = 0 - ((int)(((long)rand() << 2) / 32768L) + 1);
            g_5b88 = g_5dae;
        }
        draw_y = g_5b88;
    } else {
        ++draw_y;
    }
}

/* objcmp: {"function":"littleStars","address":"066a:339f","size":869,"symbols":{"_draw_x":"DS:5df0","_draw_y":"DS:5dee","_g_5cbb":"DS:5cbb","_g_5dae":"DS:5dae","_color_count":"DS:5db6","_plot_color":"DS:7488","_color_index_mask":"DS:5dc0","_palette_cycle_ticks":"DS:5da6","_palette_elapsed_ticks":"DS:5d7a","_ega_color_substep":"DS:5daa","_video_adapter":"DS:7484","_plot_callback":"DS:5c3f","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395","_line_step":"066a:000e","_choose_color":"066a:011b","_cycle_palette":"11c1:2515"},"segments":{"_BSS":"29bf:482c"}} */
void littleStars(void) {
    int length, start, spacing, dx, dy, minimum, initial_color, inset;
    int span, sweep_y, sweep_x, end_x, end_y, line_done, swap, row, column;
    register int mode, point;
    length = random(3 * g_5dae / 8) + g_5dae / 8;
    if (random(2))
        start = 0;
    else
        start = (random(3) + 1) * (length / 4);
    spacing = ((random(4) + 2) * length) / 20 & 0xfffe;
    dx = random(g_5dae);
    dy = random(g_5dae);
    if (random(2))
        minimum = random(spacing);
    else
        minimum = 0;
    if (video_adapter == 8)
        mode = random(3);
    else
        mode = 3;
    initial_color = plot_color = random(color_count - 1) & color_index_mask;
    for (row = 0; row <= 1; ++row) {
        sweep_x = (sweep_y = spacing) << 1;
        plot_color = initial_color & color_index_mask;
        if (row) {
            end_y = end_x = start;
            inset = 1;
        } else {
            end_x = 0;
            end_y = length;
            inset = 0;
        }
        do {
            draw_x = sweep_y;
            draw_y = sweep_x;
            if (mode == 0 || mode == 2)
                plot_color = initial_color & color_index_mask;
            line_done = 1;
            do {
                line_done = line_step(end_x, end_y, line_done);
                point = draw_x;
                span = draw_y;
                for (column = 0; column <= 7; ++column) {
                    draw_x = dx + point;
                    draw_y = dy + span;
                    if (draw_x < g_5dae && draw_y < g_5dae)
                        plot_callback(1);
                    span = -span;
                    if (column & 1)
                        point = -point;
                    if (column == 3) {
                        swap = point;
                        point = span;
                        span = swap;
                    }
                    if (mode == 2 && ++plot_color >= color_count)
                        plot_color = 3;
                }
                draw_x = span;
                draw_y = point;
                if (mode == 0 && ++plot_color >= color_count)
                    plot_color = 3;
                ++g_5cbb;
                if ((unsigned)palette_elapsed_ticks >= (unsigned)palette_cycle_ticks)
                    cycle_palette();
            } while (!line_done);
            if (mode == 1 && ++plot_color >= color_count)
                plot_color = 3;
            if (mode == 3) {
                ++ega_color_substep;
                if ((ega_color_substep &= 3) == 0 && ++plot_color >= color_count)
                    plot_color = 3;
            }
            end_x -= inset;
            --end_y;
            sweep_y -= sweep_x & 1;
            --sweep_x;
        } while (sweep_y >= minimum);
    }
    draw_x = dx;
    draw_y = dy;
}

/* objcmp: {"function":"lotsadots","address":"066a:3704","size":129,"symbols":{"_draw_x":"DS:5df0","_draw_y":"DS:5dee","_g_5dae":"DS:5dae","_g_6025":"DS:6025","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","_choose_color":"066a:011b"},"segments":{"_BSS":"29bf:482c"}} */
void lotsadots(int size) {
    if (size)
        g_6025 = (int)((long)rand() * 9L / 32768L);
    draw_x = (int)((long)rand() * g_5dae / 32768L);
    draw_y = (int)((long)rand() * g_5dae / 32768L);
    choose_color(g_6025);
}

/* objcmp: {"function":"pedals","address":"066a:3785","size":302,"absolute_symbols":{"FIDRQQ":"0x5c32"},"symbols":{"_draw_x":"DS:5df0","_draw_y":"DS:5dee","_g_5b8a":"DS:5b8a","_g_5b8c":"DS:5b8c","_g_5b8e":"DS:5b8e","_g_5b90":"DS:5b90","_g_5dae":"DS:5dae","_color_index_mask":"DS:5dc0","_color_count":"DS:5db6","_plot_color":"DS:7488","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395","F_LXRSH@":"0000:13b6","F_LXURSH@":"0000:13d6","_cos":"0000:0f6e","_sin":"0000:0e75","F_FTOL@":"0000:0fbe","_pedals_phase_scale":"DS:06b7"},"segments":{"_BSS":"29bf:482c"}} */
void pedals(int size) {
    float angle;
    if (size || g_5b8c <= 0) {
        g_5b90 = 255;
        g_5b8e = 0;
        g_5b8c = g_5b8a = 3 * (g_5dae / 4);
        if (size)
            plot_color = 3 & color_index_mask;
        else if (++plot_color >= color_count)
            plot_color = 3;
    }
    if (g_5b8a <= 0) {
        g_5b8c -= 16;
        g_5b8a = g_5b8c;
        g_5b8e = 0;
    }
    ++g_5b90;
    if ((g_5b90 = g_5b90 & 7) == 0) {
    g_5b8e += 31;
    angle = g_5b8e * 0.0002;
    draw_x = g_5b8a * sin(angle);
    draw_y = g_5b8a * cos(angle);
    if ((--g_5b8a & 7) == 0 && ++plot_color >= color_count)
        plot_color = 3;
    } else {
        ++draw_x;
        ++draw_y;
    }
}

/* objcmp: {"function":"puffs","address":"066a:38b3","size":534,"symbols":{"_draw_x":"DS:5df0","_draw_y":"DS:5dee","_g_5b92":"DS:5b92","_g_5b94":"DS:5b94","_g_5b96":"DS:5b96","_g_5b98":"DS:5b98","_g_5b9a":"DS:5b9a","_g_5b9c":"DS:5b9c","_g_5b9e":"DS:5b9e","_g_6025":"DS:6025","_g_5dae":"DS:5dae","_color_count":"DS:5db6","_plot_color":"DS:7488","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","_line_step":"066a:000e","_choose_color":"066a:011b"},"segments":{"_BSS":"29bf:482c"}} */
void puffs(int size) {
    if (size || abs(draw_x) >= g_5dae || abs(draw_y) >= g_5dae) {
        draw_x = g_5b92 = (int)((long)rand() * g_5dae / 32768L);
        draw_y = g_5b94 = (int)((long)rand() * g_5dae / 32768L);
        g_5b9a = (int)((long)rand() * (g_5dae / 6) / 32768L) + g_5dae / 6;
        g_5b9c = 1;
        g_5b96 = g_5b92 + g_5b9a;
        g_5b98 = g_5b94 + g_5b9a;
        if (size)
            g_6025 = (int)((long)rand() * 2L / 32768L);
        g_5b9e = (int)((long)rand() * 2L / 32768L) + 1;
    }
    g_5b9c = line_step(g_5b96, g_5b98, g_5b9c);
    choose_color(0);
    if (g_5b9c) {
        if (g_6025) {
            draw_x = g_5b92;
            draw_y = g_5b94;
            g_5b96 = g_5b92 + (int)((long)rand() * (g_5b9a * 2) / 32768L) - g_5b9a;
            g_5b98 = g_5b94 + (int)((long)rand() * (g_5b9a * 2) / 32768L) - g_5b9a;
        } else {
            g_5b92 += g_5b9e;
            draw_x = g_5b92;
            g_5b94 += g_5b9e;
            draw_y = g_5b94;
            g_5b96 = g_5b92 + (int)((long)rand() * g_5b9a / 32768L);
            g_5b98 = g_5b94 + (int)((long)rand() * g_5b9a / 32768L);
        }
    }
}

/* objcmp: {"function":"ribbons","address":"066a:3ac9","size":338,"symbols":{"_draw_x":"DS:5df0","_draw_y":"DS:5dee","_g_5b9f":"DS:5b9f","_g_5ba1":"DS:5ba1","_g_5ba3":"DS:5ba3","_g_6025":"DS:6025","_plot_color":"DS:7488","_g_5dae":"DS:5dae","_color_count":"DS:5db6","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7"},"segments":{"_BSS":"29bf:482c"}} */
void ribbons(int size) {
    if (size) {
        g_5b9f = g_5ba1 = 0;
        g_5ba3 = 24;
        g_6025 = (int)((long)rand() * 2L / 32768L);
    }
    if (g_5b9f++ > g_5ba1) {
        draw_x -= g_5ba1;
        g_5b9f = 0;
        if (++g_5ba1 > g_5ba3) {
            g_5ba1 = 0;
            if ((g_5ba3 += (int)((long)rand() * 4L / 32768L)) > 44)
                g_5ba3 = 0;
        }
        if (g_6025 && ++plot_color >= color_count)
            plot_color = 3;
        if (++draw_y > g_5dae)
            draw_y = (int)((long)rand() * (g_5dae / 2) / 32768L);
    }
    if (++draw_x > g_5dae)
        draw_x = (int)((long)rand() * (g_5dae / 2) / 32768L);
    if ((int)((long)rand() * 1000L / 32768L) == 0 && ++plot_color >= color_count)
        plot_color = 3;
}

/* objcmp: {"function":"sierpinski","address":"066a:3c1b","size":311,"symbols":{"_draw_x":"DS:5df0","_draw_y":"DS:5dee","_g_5ba5":"DS:5ba5","_g_5bb1":"DS:5bb1","_g_5dae":"DS:5dae","_color_index_mask":"DS:5dc0","_color_count":"DS:5db6","_plot_color":"DS:7488","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395"},"segments":{"_BSS":"29bf:482c"}} */
void sierpinski(int size) {
    int choice;
    register int center;
    if (size) {
        center = (int)((long)rand() * (g_5dae / 2) / 32768L) + g_5dae / 3;
        draw_x = draw_y = center;
        g_5ba5[1].x = g_5ba5[1].y = g_5ba5[2].y = center;
        if ((int)((long)rand() * 2L / 32768L))
            g_5ba5[0].x = g_5ba5[0].y = g_5ba5[2].x = center / 2;
        else
            g_5ba5[0].x = g_5ba5[0].y = g_5ba5[2].x = 0;
        g_5bb1 = (int)((long)rand() * 2L / 32768L);
        plot_color = (int)(((long)rand() << 2) / 32768L) & color_index_mask;
    }
    choice = (int)((long)rand() * 3L / 32768L);
    if (choice == 0 && g_5bb1 == 0 && ++plot_color >= color_count)
        plot_color = 3;
    draw_x += (g_5ba5[choice].x - draw_x) / 2;
    draw_y += (g_5ba5[choice].y - draw_y) / 2;
}

/* objcmp: {"function":"sine","address":"066a:3d52","size":505,"symbols":{"_draw_x":"DS:5df0","_draw_y":"DS:5dee","_amplitude_x":"DS:5bb3","_amplitude_y":"DS:5bb7","_sine_phase":"DS:5bbb","_mod_x":"DS:5bbf","_mod_y":"DS:5bc3","_step_x":"DS:5bc7","_step_y":"DS:5bcb","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_FTOL@":"0000:0fbe","_sin":"0000:0e75","_cos":"0000:0f6e","_g_5dae":"DS:5dae","_color_count":"DS:5db6","_plot_color":"DS:7488","_plot_callback":"DS:5c3f","_ega_plot_symmetric_640x700":"1899:012d","_sine_phase_step":"DS:06f3"},"absolute_symbols":{"FIDRQQ":"0x5c32","FIWRQQ":"0xa23d"},"segments":{"_BSS":"29bf:482c"}} */
void sine(int rebuild)
{
    if (rebuild) {
        sine_phase = mod_x = mod_y = 0;
        if (++plot_color >= color_count)
            plot_color = 3;
        amplitude_x = g_5dae / 8 + random(g_5dae / 8);
        amplitude_y = g_5dae / 8 + random(g_5dae / 8);
        step_x = 1.0 / (1 + amplitude_x);
        step_y = 1.0 / (1 + amplitude_y);
    }
    draw_x = amplitude_x * sin(sine_phase);
    draw_y = amplitude_y * cos(sine_phase);
    sine_phase += 0.01;
    mod_x += step_x;
    mod_y += step_y;
    amplitude_x += sin(mod_x);
    amplitude_y += cos(mod_y);
    if (draw_x == 0 && ++plot_color >= color_count)
        plot_color = 3;
    if (plot_callback == ega_plot_symmetric_640x700) {
        draw_x &= ~1;
        draw_y &= ~1;
    }
}

/* objcmp: {"function":"squares","address":"066a:3f4b","size":354,"symbols":{"_draw_x":"DS:5df0","_draw_y":"DS:5dee","_g_5bcf":"DS:5bcf","_g_5bd1":"DS:5bd1","_g_5bd3":"DS:5bd3","_g_5bd5":"DS:5bd5","_g_5bd7":"DS:5bd7","_g_5bd9":"DS:5bd9","_g_5dae":"DS:5dae","_g_5dc4":"DS:5dc4","_color_count":"DS:5db6","_g_6025":"DS:6025","_plot_color":"DS:7488","_direction_x":"DS:00a6","_direction_y":"DS:00ae","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7"},"segments":{"_BSS":"29bf:482c"}} */
void squares(int size) {
    if (size || g_5bd1 >= g_5bd9 || draw_x > g_5dae || draw_y > g_5dae) {
        g_5bd5 = g_5bd7 = g_5bcf = g_5bd1 = g_5bd3 = 0;
        draw_x = (int)((long)rand() * g_5dae / 32768L);
        draw_y = (int)((long)rand() * g_5dae / 32768L);
        g_6025 = (int)((long)rand() * 2L / 32768L);
        if (g_5dc4)
            g_6025 = 0;
        g_5bd9 = (int)((long)rand() * (g_5dae / 4) / 32768L) + g_5dae / 4;
        if (g_6025)
            g_5bd9 >>= 1;
    }
    if (g_5bcf++ >= g_5bd1) {
        g_5bcf = 0;
        if (++g_5bd3 > 3) {
            ++g_5bd1;
            if (g_6025)
                --draw_x;
            else
                ++draw_x;
            if (++plot_color >= color_count)
                plot_color = 3;
            g_5bd3 = 0;
        }
        g_5bd5 = direction_x[g_5bd3];
        g_5bd7 = direction_y[g_5bd3];
    }
    draw_x += g_5bd5;
    draw_y += g_5bd7;
}

/* objcmp: {"function":"squirals","address":"066a:40ad","size":778,"symbols":{"_draw_x":"DS:5df0","_draw_y":"DS:5dee","_g_5dae":"DS:5dae","_color_count":"DS:5db6","_plot_color":"DS:7488","_g_5cbb":"DS:5cbb","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","_sin":"0000:0e75","_cos":"0000:0f6e","F_FTOL@":"0000:0fbe","_palette_cycle_ticks":"DS:5da6","_palette_elapsed_ticks":"DS:5d7a","_plot_callback":"DS:5c3f","_line_step":"066a:000e","_cycle_palette":"11c1:2515","F_LXLSH@":"0000:1395","_squirals_start_angle":"DS:06fb","_squirals_angle_step":"DS:0703"},"absolute_symbols":{"FIDRQQ":"0x5c32"},"segments":{"_BSS":"29bf:482c"}} */
void squirals(int size) {
    int end_x, end_y, center_x, center_y, radius, symmetry, line_done;
    float scale = 1.0, decay, angle_step, cosine, sine_value, angle;
    register int saved_x, saved_y;

    angle = random(2) * 0.7853975;
    angle_step = (random(3) + 1) * 0.03;
    decay = 1.0 / (sin(angle_step) + cos(angle_step));
    radius = random(3 * g_5dae / 8) + g_5dae / 8;
    center_x = random(3 * g_5dae / 4);
    center_y = random(3 * g_5dae / 4);
    symmetry = random(4);
    do {
        cosine = cos(angle);
        sine_value = sin(angle);
        end_x = radius * scale * (cosine - sine_value);
        end_y = radius * scale * (cosine + sine_value);
        draw_x = end_y;
        draw_y = -end_x;
        line_done = 1;
        do {
            line_done = line_step(end_x, end_y, line_done);
            saved_x = draw_x;
            saved_y = draw_y;
            switch (symmetry) {
            case 0:
            case 1:
            case 2:
                draw_x = center_x + saved_x;
                draw_y = center_y + saved_y;
                plot_callback(1);
                draw_x = center_x - saved_x;
                draw_y = center_y - saved_y;
                plot_callback(1);
                if (symmetry == 2)
                    break;
            case 3:
                draw_x = center_x - saved_y;
                draw_y = center_y + saved_x;
                plot_callback(1);
                draw_x = center_x + saved_y;
                draw_y = center_y - saved_x;
                plot_callback(1);
            }
            draw_x = saved_x;
            draw_y = saved_y;
            if ((unsigned)palette_elapsed_ticks >= (unsigned)palette_cycle_ticks)
                cycle_palette();
            ++g_5cbb;
        } while (!line_done);
        angle += angle_step;
        scale *= decay;
        if (++plot_color >= color_count)
            plot_color = 3;
    } while (abs(end_x) >= 1 || abs(end_y) >= 1);
    draw_x = center_x;
    draw_y = center_y;
}

/* objcmp: {"function":"spiral","address":"066a:43b7","size":235,"symbols":{"_draw_x":"DS:5df0","_draw_y":"DS:5dee","_g_5bdb":"DS:5bdb","_g_5bdd":"DS:5bdd","_g_5bdf":"DS:5bdf","_g_5be1":"DS:5be1","_g_5be3":"DS:5be3","_g_5be5":"DS:5be5","_direction_x":"DS:00a6","_direction_y":"DS:00ae","_g_5dae":"DS:5dae","_color_count":"DS:5db6","_plot_color":"DS:7488","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7"},"segments":{"_BSS":"29bf:482c"}} */
void spiral(int size) {
    if (size || draw_x > g_5dae || draw_y > g_5dae) {
        g_5bdb = g_5bdd = g_5be1 = g_5bdf = 0;
        draw_x = (int)((long)rand() * g_5dae / 32768L);
        draw_y = (int)((long)rand() * g_5dae / 32768L);
    }
    if (g_5bdb++ >= g_5bdd) {
        g_5bdb = 0;
        ++g_5be1;
        g_5be1 = g_5be1 & 3;
        if ((g_5be1 & 1) == 0)
            ++g_5bdd;
    g_5be3 = direction_x[g_5be1];
    g_5be5 = direction_y[g_5be1];
    }
    draw_x += g_5be3;
    draw_y += g_5be5;
    ++g_5bdf;
    if ((g_5bdf = g_5bdf & 7) == 0 && ++plot_color >= color_count)
        plot_color = 3;
}

/* objcmp: {"function":"spiral1","address":"066a:44a2","size":345,"symbols":{"_draw_x":"DS:5df0","_draw_y":"DS:5dee","_g_5be7":"DS:5be7","_g_5be9":"DS:5be9","_g_5beb":"DS:5beb","_g_5bed":"DS:5bed","_g_5bef":"DS:5bef","_g_5bf1":"DS:5bf1","_diamond_x":"DS:01e6","_diamond_y":"DS:01ee","_g_5dae":"DS:5dae","_g_5dc4":"DS:5dc4","_color_count":"DS:5db6","_g_6025":"DS:6025","_plot_color":"DS:7488","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7"},"segments":{"_BSS":"29bf:482c"}} */
void spiral1(int size) {
    if (size || g_5be9 >= g_5bf1 || draw_x > g_5dae || draw_y > g_5dae) {
        g_5bed = g_5bef = g_5be7 = g_5be9 = g_5beb = 0;
        draw_x = (int)((long)rand() * g_5dae / 32768L);
        draw_y = (int)((long)rand() * g_5dae / 32768L);
        g_6025 = (int)((long)rand() * 2L / 32768L);
        if (g_5dc4)
            g_6025 = 1;
        g_5bf1 = (int)((long)rand() * (3 * (g_5dae / 4)) / 32768L);
        if (g_6025)
            g_5bf1 >>= 1;
    }
    if (g_5be7++ >= g_5be9) {
        g_5be7 = 0;
        if (++g_5beb > 3) {
            ++g_5be9;
            if (g_6025)
                --draw_x;
            else
                ++draw_x;
            g_5beb = 0;
            if (++plot_color >= color_count)
                plot_color = 3;
        }
        g_5bed = diamond_x[g_5beb];
        g_5bef = diamond_y[g_5beb];
    }
    draw_x += g_5bed;
    draw_y += g_5bef;
}

/* objcmp: {"function":"sweep","address":"066a:45fb","size":107,"symbols":{"_draw_x":"DS:5df0","_draw_y":"DS:5dee","_g_5bf3":"DS:5bf3","_g_5bf5":"DS:5bf5","_g_5dae":"DS:5dae","_color_count":"DS:5db6","_plot_color":"DS:7488"},"segments":{"_BSS":"29bf:482c"}} */
void sweep(int size) {
    if (size || g_5bf5 > g_5dae / 2)
        g_5bf5 = g_5bf3 = draw_x = draw_y = 0;
    if (g_5bf3++ > g_5bf5) {
        ++draw_y;
        g_5bf3 = 0;
    }
    if (draw_x++ > g_5dae) {
        if (++plot_color >= color_count)
            plot_color = 3;
        ++g_5bf5;
        draw_x = draw_y = 0;
    }
}

/* objcmp: {"function":"sweep2","address":"066a:4666","size":202,"symbols":{"_draw_x":"DS:5df0","_draw_y":"DS:5dee","_g_5bf7":"DS:5bf7","_g_5bf9":"DS:5bf9","_g_5bfb":"DS:5bfb","_g_5bfd":"DS:5bfd","_g_5dae":"DS:5dae","_color_count":"DS:5db6","_g_5dc4":"DS:5dc4","_g_6025":"DS:6025","_plot_color":"DS:7488","_line_step":"066a:000e","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7"},"segments":{"_BSS":"29bf:482c"}} */
void sweep2(int size) {
    register int mode = size;
    if (mode || g_5bfd != 0) {
        draw_x = draw_y = 0;
        if (mode) {
            g_6025 = (int)((long)rand() * 2L / 32768L);
            if (g_5dc4)
                g_6025 = 0;
            if (g_6025) {
                g_5bf9 = g_5bfb = 0;
            } else {
                g_5bf9 = -1;
            }
        }
        if (g_6025) {
            g_5bf9 += g_5bfb;
            if (++g_5bfb > g_5dae)
                g_5bfb = -g_5dae;
        } else {
            ++g_5bf9;
            g_5bf9 %= (g_5dae + 2);
        }
        g_5bf7 = g_5dae;
        if (++plot_color >= color_count)
            plot_color = 3;
        mode = 1;
    }
    g_5bfd = line_step(g_5bf9, g_5bf7, mode);
}

/* objcmp: {"function":"triangles","address":"066a:4730","size":417,"symbols":{"_draw_x":"DS:5df0","_draw_y":"DS:5dee","_g_5bff":"DS:5bff","_g_5c01":"DS:5c01","_g_5c03":"DS:5c03","_g_5c05":"DS:5c05","_g_5c07":"DS:5c07","_g_5dae":"DS:5dae","_color_index_mask":"DS:5dc0","_color_count":"DS:5db6","_g_6025":"DS:6025","_plot_color":"DS:7488","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395"},"segments":{"_BSS":"29bf:482c"}} */
void triangles(int size) {
    if (size) {
        g_5c03 = 0;
        g_6025 = (int)((long)rand() * 3L / 32768L);
        g_5c07 = (int)((long)rand() * 3L / 32768L);
        plot_color = (int)(((long)rand() << 2) / 32768L) & color_index_mask;
    }
    if (g_5c03) {
        if (++draw_x > g_5bff) {
            if (g_6025 && ++plot_color >= color_count)
                plot_color = 3;
            if (g_5c07)
                --g_5c01;
            if (g_5bff++ >= g_5c05) {
                g_5c03 = 0;
            } else {
                draw_x = g_5c01;
                if (++draw_y > g_5dae) {
                    g_5c03 = 0;
                    draw_y = g_5dae;
                }
            }
        }
    }
    if (!g_5c03) {
        g_5c03 = 1;
        g_6025 = !g_6025;
        g_5c07 = !g_5c07;
        g_5c01 = (int)((long)rand() * (g_5dae / 2) / 32768L);
        g_5bff = g_5c01;
        draw_x = g_5c01;
        g_5c05 = (int)((long)rand() * (g_5dae / 3) / 32768L) + g_5c01 + 8;
        if (g_5c05 > g_5dae)
            g_5c05 = g_5dae;
        draw_y = (int)((long)rand() * g_5dae / 32768L);
    }
}

/* objcmp: {"function":"tunnel","address":"066a:48d1","size":181,"symbols":{"_draw_x":"DS:5df0","_draw_y":"DS:5dee","_g_5c09":"DS:5c09","_g_5c0b":"DS:5c0b","_g_5c0d":"DS:5c0d","_g_5c0f":"DS:5c0f","_g_5dae":"DS:5dae","_color_count":"DS:5db6","_color_index_mask":"DS:5dc0","_plot_color":"DS:7488","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7"},"segments":{"_BSS":"29bf:482c"}} */
void tunnel(int size) {
    if (size) {
        g_5c0b = 0;
        if ((int)((long)rand() * 2L / 32768L)) {
            g_5c0f = &g_5c0d;
        } else {
            g_5c0f = &draw_y;
        }
        plot_color = 3 & color_index_mask;
    }
    if (g_5c0b) {
        if (++draw_x > *g_5c0f) {
            draw_x = g_5c09;
            ++draw_y;
            --g_5c0b;
            if (draw_y > g_5dae) {
                g_5c0b = 0;
                draw_y = g_5dae;
            }
            if (++plot_color >= color_count)
                plot_color = 3;
        }
    }
    if (g_5c0b <= 0) {
        g_5c0d = g_5c0b = g_5dae;
        draw_x = g_5c09 = draw_y = 0;
    }
}

/* objcmp: {"function":"wandering","address":"066a:4986","size":456,"symbols":{"_draw_x":"DS:5df0","_draw_y":"DS:5dee","_g_5c13":"DS:5c13","_g_5c15":"DS:5c15","_g_5c17":"DS:5c17","_g_5c19":"DS:5c19","_g_5c1b":"DS:5c1b","_g_5c1d":"DS:5c1d","_g_5c1f":"DS:5c1f","_g_5c21":"DS:5c21","_g_5c23":"DS:5c23","_g_5c25":"DS:5c25","_g_5dae":"DS:5dae","_color_count":"DS:5db6","_plot_color":"DS:7488","_g_6025":"DS:6025","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395","_line_step":"066a:000e"},"segments":{"_BSS":"29bf:482c"}} */
int wandering(int size) {
    if (size) {
        g_5c13 = g_5c15 = g_5c17 = g_5c19 = g_5c1b = 0;
        g_5c25 = 0;
        if (++plot_color >= color_count)
            plot_color = 3;
    }
    if (!size && !g_5c25)
        return g_5c25 = line_step(g_5c17, g_5c19, 0);
    if (g_5c1b <= 0) {
        g_5c1d = (int)((long)rand() * 9L / 32768L) - 4;
        g_5c1f = (int)((long)rand() * 9L / 32768L) - 4;
        g_5c21 = (int)((long)rand() * 9L / 32768L) - 4;
        g_5c23 = (int)((long)rand() * 9L / 32768L) - 4;
        g_5c1b = ((int)((long)rand() * 8L / 32768L) + 1) << 2;
    }
    if (++plot_color >= color_count)
        plot_color = 3;
    g_5c13 += g_5c1d;
    if (g_5c13 < 1 || g_5c13 > g_5dae)
        g_5c1d = -g_5c1d;
    g_5c17 += g_5c1f;
    if (g_5c17 < 1 || g_5c17 > g_5dae)
        g_5c1f = -g_5c1f;
    g_5c15 += g_5c21;
    if (g_5c15 < 1 || g_5c15 > g_5dae)
        g_5c21 = -g_5c21;
    g_5c19 += g_5c23;
    if (g_5c19 < 1 || g_5c19 > g_5dae)
        g_5c23 = -g_5c23;
    --g_5c1b;
    draw_x = g_5c13;
    draw_y = g_5c15;
    return g_5c25 = line_step(g_5c17, g_5c19, 1);
}

/* objcmp: {"function":"zigstream","address":"066a:4b4e","size":495,"symbols":{"_draw_x":"DS:5df0","_draw_y":"DS:5dee","_g_5c27":"DS:5c27","_g_5c29":"DS:5c29","_g_5c2b":"DS:5c2b","_g_5c2d":"DS:5c2d","_g_5c2f":"DS:5c2f","_g_5c31":"DS:5c31","_zigstream_dx":"DS:01f6","_zigstream_dy":"DS:01fa","_g_6025":"DS:6025","_g_5dae":"DS:5dae","_color_count":"DS:5db6","_plot_color":"DS:7488","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","_line_step":"066a:000e"},"segments":{"_BSS":"29bf:482c"}} */
void zigstream(int size) {
    int mode;
    if (size || draw_x > g_5dae || draw_y > g_5dae) {
        if (size)
            g_6025 = (int)((long)rand() * 2L / 32768L);
        g_5c27 = g_5c29 = g_5c2d = g_5c2b = 0;
        draw_x = (int)((long)rand() * g_5dae / 32768L);
        draw_y = (int)((long)rand() * g_5dae / 32768L);
        if (++plot_color >= color_count)
            plot_color = 3;
    }
    if ((mode = g_6025) == 0) {
        if (g_5c27++ >= g_5c29) {
            g_5c27 = 0;
            if ((++g_5c2d & 1) == 0) {
                ++g_5c29;
                g_5c2d = 0;
            }
            g_5c2f = zigstream_dx[g_5c2d];
            g_5c31 = zigstream_dy[g_5c2d];
        }
        ++g_5c2b;
        if ((g_5c2b = g_5c2b & 7) == 0) {
            ++plot_color;
            if (plot_color >= color_count)
                plot_color = 3;
        }
    } else {
        if (--g_5c27 <= 0) {
            g_5c2f = (int)((long)rand() * 2L / 32768L) ? -1 : 1;
            g_5c31 = (int)((long)rand() * 3L / 32768L) - 1;
            g_5c27 = (int)((long)rand() * 12L / 32768L) + 12;
            if (++plot_color >= color_count)
                plot_color = 3;
        }
        if (draw_x <= 0 || draw_x >= g_5dae)
            g_5c2f = -g_5c2f;
        if (draw_y <= 0 || draw_y >= g_5dae)
            g_5c31 = -g_5c31;
    }
    draw_x += g_5c2f;
    draw_y += g_5c31;
}

/* Image definitions are packed 15-byte rows; fades use 7-byte rows. */
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
    {littleStars,10,"littleStars",0,0,'p'},
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
    {(ImageDrawRoutine)wandering,40,"wandering",1,0,'n'},
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

/* objcmp: {"function":"main","address":"066a:4d3d","size":1123,"symbols":{"_countdown_ticks":"DS:5d78","_exit_requested":"DS:5d80","_startup_config_failure":"DS:5d84","_semi_auto_mode":"DS:5da0","_window_mode":"DS:5dbc","_insecure_argument_index":"DS:5dc8","_bashful_mode":"DS:5dca","_no_disk_park":"DS:5dcc","_show_mode":"DS:5dce","_keep_typematic":"DS:5dd2","_palette_cycle_offset":"DS:5dec","_mouse_abort_pending":"DS:5d7c","_input_pending":"DS:5d7e","_background_fill_word":"DS:5d8a","_startup_active":"DS:5d8e","_fade_count":"DS:5d98","_ctrl_image_count":"DS:5d9a","_alt_image_count":"DS:5d9c","_palette_upload_callback":"DS:5c37","_mode_setup_callback":"DS:5c43","_palette_build_callback":"DS:5c47","_atexit":"0000:0fea","_chdir":"0000:1017","_exit":"0000:11a8","_setdisk":"0000:128e","_srand":"0000:1683","_time":"0000:1ef6","_tolower":"0000:1f66","_bioskey":"0000:2553","_textcolor":"0000:25fc","_cprintf":"0000:277a","_int86":"0000:320e","_signal":"0000:3bcb","_nosound":"0000:3db4","_gettextinfo":"0000:3dbb","_clrscr":"0000:4332","_fprintf":"0000:45d4","_getcwd":"0000:4b1f","_strchr":"0000:598f","_strncpy":"0000:5a79","_executable_path":"DS:5d17","_startup_directory":"DS:5cc7","_saved_text_info":"DS:6016","_keyboard_regs":"DS:6006","_palette_buffer":"DS:5d94","_startup_directory_error":"DS:091d","_assistance_message":"DS:0941","_parser_error":"DS:097a","_loading_box_top":"DS:098c","_loading_box_left":"DS:09a4","_loading_label":"DS:09a8","_loading_box_right":"DS:09b8","_loading_box_bottom":"DS:09be","_restore_directory_error":"DS:09d7","_startup_config_error":"DS:09eb","_restore_text_attributes":"11c1:01b5","_restore_runtime_callbacks":"11c1:01c7","_signal_diagnostic":"11c1:02d8","_initialize_program_defaults":"11c1:1f29","_save_video_state":"1899:0932","_detect_video_adapter":"1899:0959","_parse_command_line":"11c1:2eed","_print_startup_information":"11c1:339f","_keyboard_led_flags":"11c1:000a","_fatal_selfcheck_error":"11c1:052f","_check_executable_integrity":"11c1:0949","_configure_video_backend":"11c1:21a6","_resolve_startup_configuration":"11c1:2d26","_install_runtime_callbacks":"11c1:00c9","_beep":"11c1:0e07","_park_disk_heads":"1899:0efb","_prepare_logo_image_resources":"17f5:06ef","_semi_auto_loop":"11c1:3539","_auto_loop":"11c1:38c3","_set_ega_raster_xor":"11c1:0e2a","_closing_graphic_and_music":"11c1:2161","_restore_video_state":"1899:0bcd","_print_closing_text_and_exit":"11c1:0d46","__streams":"DS:43f4"},"segments":{"_BSS":"29bf:482c"}} */
void main(int argc, char far * far *argv)
{
    int config_option, insecure_character;
    char far *drive_separator;

    strncpy(executable_path, argv[0], 80);
    gettextinfo(&saved_text_info);
    atexit(restore_text_attributes);
    nosound();
    startup_active = 1;
    initialize_program_defaults();
    save_video_state();
    alt_image_count = 36;
    ctrl_image_count = 6;
    fade_count = 24;
    if (getcwd(startup_directory, 80) == NULL) {
        fprintf(stderr, "\r Startup error getting directory.\n");
        fprintf(stderr, "Please write for assistance. Press any key to continue.\a");
        bioskey(0);
    }
    config_option = parse_command_line(argc, argv, &insecure_character);
    detect_video_adapter();
    print_startup_information();
    keyboard_led_flags(0x70, 1);
    if (insecure_argument_index && tolower(argv[insecure_argument_index][insecure_character]) != 'i') {
        exit_requested = 1;
        fatal_selfcheck_error(1, "parser corrupted!");
    }
    if (!show_mode && !window_mode)
        check_executable_integrity();
    else
        exit_requested = 0;
    configure_video_backend(1);
    resolve_startup_configuration(config_option);
    if (show_mode || bashful_mode || window_mode)
        keep_typematic = 1;
    if (!keep_typematic) {
        keyboard_regs.h.ah = 3;
        keyboard_regs.h.al = 5;
        keyboard_regs.h.bh = 3;
        keyboard_regs.h.bl = 31;
        int86(0x16, &keyboard_regs, &keyboard_regs);
    }
    install_runtime_callbacks();
    atexit(restore_runtime_callbacks);
    signal(0x16, signal_diagnostic);
    signal(8, signal_diagnostic);
    signal(4, signal_diagnostic);
    signal(0x0b, signal_diagnostic);
    if (!show_mode && !exit_requested && !window_mode) {
        countdown_ticks = 40;
        if (!bashful_mode || !no_disk_park)
            beep(880, 40);
        if (!no_disk_park)
            park_disk_heads();
        if (!g_5dc4 && (!bashful_mode || !no_disk_park)) {
            textcolor(14);
            cprintf(" \311\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\273 \r\n");
            cprintf(" \272 ");
            textcolor(140);
            cprintf("Program Loading");
            textcolor(14);
            cprintf(" \272 \r\n");
            cprintf(" \310\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\274 \r\n\n");
            textcolor(3);
        }
    }
    prepare_logo_image_resources();
    palette_build_callback(0);
    while (countdown_ticks)
        ;
    background_fill_word = 0;
    mode_setup_callback(1);
    clrscr();
    palette_upload_callback(palette_buffer + palette_cycle_offset);
    srand((unsigned)time(NULL));
    while (bioskey(1))
        bioskey(0);
    startup_active = input_pending = mouse_abort_pending = 0;
    while (!exit_requested) {
        if (semi_auto_mode)
            semi_auto_loop();
        else
            auto_loop();
    }
    keyboard_led_flags(0x70, 1);
    set_ega_raster_xor(0);
    if (!show_mode && !bashful_mode && !startup_config_failure)
        closing_graphic_and_music();
    if (!bashful_mode)
        while (bioskey(1))
            bioskey(0);
    restore_video_state(1);
    if (!bashful_mode && !show_mode && startup_directory != NULL) {
        if (chdir(startup_directory)) {
            fprintf(stderr, "PATH ERROR ON EXIT\a");
        } else {
            drive_separator = strchr(startup_directory, ':');
            if (drive_separator != NULL)
                setdisk(tolower(drive_separator[-1]) - 'a');
        }
    }
    if (startup_config_failure) {
        fprintf(stderr, "Program aborting in startup routines due to defective configuration file!\n\a");
        exit(14);
    }
    nosound();
    if (!show_mode && !bashful_mode && !window_mode)
        print_closing_text_and_exit(0);
    else
        exit(0);
}

/* Uninitialized module state. BC++ 3.0 emits BSS in reverse declaration order.
 * Offsets below are the independently recovered DGROUP addresses. */
unsigned image_data_offset; /* DS:602F */
unsigned int serial_r2; /* DS:602D */
unsigned int serial_r1; /* DS:602B */
unsigned long expected_crc; /* DS:6027 */
int g_6025; /* DS:6025 */
unsigned long computed_crc; /* DS:6021 */
struct text_info saved_text_info; /* DS:6016 */
union REGS keyboard_regs; /* DS:6006 */
FILE far *selfcheck_stream; /* DS:6002 */
FILE far *g_5ffe; /* DS:5FFE */
char far *default_config_name; /* DS:5FFA */
TrigPair trig_table[64]; /* DS:5DF2 */
int draw_x; /* DS:5DF0 */
int draw_y; /* DS:5DEE */
unsigned palette_cycle_offset; /* DS:5DEC */
unsigned int original_ctrlbreak_setting; /* DS:5DEA */
unsigned g_5de8; /* DS:5DE8 */
unsigned g_5de6; /* DS:5DE6 */
unsigned g_5de4; /* DS:5DE4 */
unsigned int palette_fixed_mode; /* DS:5DE2 */
unsigned int palette_alternate_mode; /* DS:5DE0 */
unsigned g_5dde; /* DS:5DDE */
unsigned g_5ddc; /* DS:5DDC */
unsigned int palette_random_mode; /* DS:5DDA */
unsigned g_5dd8; /* DS:5DD8 */
unsigned unused_palette_state; /* DS:5DD6 */
unsigned g_5dd4; /* DS:5DD4 */
unsigned keep_typematic; /* DS:5DD2 */
unsigned int palette_four_ega_colors; /* DS:5DD0 */
unsigned show_mode; /* DS:5DCE */
unsigned no_disk_park; /* DS:5DCC */
unsigned bashful_mode; /* DS:5DCA */
unsigned insecure_argument_index; /* DS:5DC8 */
unsigned draw_delay_count; /* DS:5DC6 */
unsigned g_5dc4; /* DS:5DC4 */
unsigned g_5dc2; /* DS:5DC2 */
unsigned color_index_mask; /* DS:5DC0 */
int muted; /* DS:5DBE */
unsigned window_mode; /* DS:5DBC */
unsigned int palette_entries; /* DS:5DBA */
unsigned g_5db8; /* DS:5DB8 */
unsigned color_count; /* DS:5DB6 */
unsigned g_5db4; /* DS:5DB4 */
int screen_half_height; /* DS:5DB2 */
int screen_half_width; /* DS:5DB0 */
int g_5dae; /* DS:5DAE */
int g_5dac; /* DS:5DAC */
int ega_color_substep; /* DS:5DAA */
unsigned g_5da8; /* DS:5DA8 */
unsigned palette_cycle_ticks; /* DS:5DA6 */
unsigned int palette_step; /* DS:5DA4 */
unsigned g_5da2; /* DS:5DA2 */
unsigned semi_auto_mode; /* DS:5DA0 */
unsigned int g_5d9e; /* DS:5D9E */
unsigned alt_image_count; /* DS:5D9C */
unsigned ctrl_image_count; /* DS:5D9A */
unsigned fade_count; /* DS:5D98 */
unsigned char far *palette_buffer; /* DS:5D94 */
unsigned int g_5d92; /* DS:5D92 */
unsigned int g_5d90; /* DS:5D90 */
unsigned startup_active; /* DS:5D8E */
int g_5d8c; /* DS:5D8C */
unsigned background_fill_word; /* DS:5D8A */
unsigned g_5d88; /* DS:5D88 */
unsigned g_5d86; /* DS:5D86 */
unsigned startup_config_failure; /* DS:5D84 */
unsigned int freeze_led_state; /* DS:5D82 */
unsigned exit_requested; /* DS:5D80 */
unsigned input_pending; /* DS:5D7E */
unsigned mouse_abort_pending; /* DS:5D7C */
unsigned palette_elapsed_ticks; /* DS:5D7A */
unsigned countdown_ticks; /* DS:5D78 */
unsigned unused_input_state; /* DS:5D76 */
unsigned int timer_hook_installed; /* DS:5D74 */
unsigned int keyboard_hook_installed; /* DS:5D72 */
unsigned int skip_ctrlbreak_control; /* DS:5D70 */
unsigned palette_base_blue; /* DS:5D6E */
unsigned palette_base_green; /* DS:5D6C */
long resource_base; /* DS:5D68 */
unsigned char unused_path_state; /* DS:5D67 */
char near executable_path[80]; /* DS:5D17 */
char startup_directory[80]; /* DS:5CC7 */
unsigned g_5cc5; /* DS:5CC5 */
unsigned g_5cc3; /* DS:5CC3 */
unsigned g_5cc1; /* DS:5CC1 */
unsigned g_5cbf; /* DS:5CBF */
unsigned g_5cbd; /* DS:5CBD */
int g_5cbb; /* DS:5CBB */
unsigned int mouse_callback_installed; /* DS:5CB9 */
unsigned g_5cb7; /* DS:5CB7 */
unsigned int mouse_available; /* DS:5CB5 */
unsigned g_5cb3; /* DS:5CB3 */
int g_5cb1; /* DS:5CB1 */
unsigned int draw_symmetry; /* DS:5CAF */
char g_5c5f[80]; /* DS:5C5F */
unsigned int far *current_image_header; /* DS:5C5B */
void far *g_5c57; /* DS:5C57 */
unsigned char far *g_5c53; /* DS:5C53 */
unsigned int g_5c51; /* DS:5C51 */
unsigned int g_5c4f; /* DS:5C4F */
ImageDrawRoutine g_5c4b; /* DS:5C4B */
void (far *palette_build_callback)(int); /* DS:5C47 */
void (far *mode_setup_callback)(int); /* DS:5C43 */
void (far *plot_callback)(unsigned); /* DS:5C3F */
unsigned (far *read_pixel_callback)(int,int); /* DS:5C3B */
void (far *palette_upload_callback)(unsigned char far *); /* DS:5C37 */
void (*g_5c33)(unsigned, unsigned); /* DS:5C33 */
int g_5c31; /* DS:5C31 */
int g_5c2f; /* DS:5C2F */
int g_5c2d; /* DS:5C2D */
int g_5c2b; /* DS:5C2B */
int g_5c29; /* DS:5C29 */
int g_5c27; /* DS:5C27 */
int g_5c25; /* DS:5C25 */
int g_5c23; /* DS:5C23 */
int g_5c21; /* DS:5C21 */
int g_5c1f; /* DS:5C1F */
int g_5c1d; /* DS:5C1D */
int g_5c1b; /* DS:5C1B */
int g_5c19; /* DS:5C19 */
int g_5c17; /* DS:5C17 */
int g_5c15; /* DS:5C15 */
int g_5c13; /* DS:5C13 */
int far *g_5c0f; /* DS:5C0F */
int g_5c0d; /* DS:5C0D */
int g_5c0b; /* DS:5C0B */
int g_5c09; /* DS:5C09 */
int g_5c07; /* DS:5C07 */
int g_5c05; /* DS:5C05 */
int g_5c03; /* DS:5C03 */
int g_5c01; /* DS:5C01 */
int g_5bff; /* DS:5BFF */
int g_5bfd; /* DS:5BFD */
int g_5bfb; /* DS:5BFB */
int g_5bf9; /* DS:5BF9 */
int g_5bf7; /* DS:5BF7 */
int g_5bf5; /* DS:5BF5 */
int g_5bf3; /* DS:5BF3 */
int g_5bf1; /* DS:5BF1 */
int g_5bef; /* DS:5BEF */
int g_5bed; /* DS:5BED */
int g_5beb; /* DS:5BEB */
int g_5be9; /* DS:5BE9 */
int g_5be7; /* DS:5BE7 */
int g_5be5; /* DS:5BE5 */
int g_5be3; /* DS:5BE3 */
int g_5be1; /* DS:5BE1 */
int g_5bdf; /* DS:5BDF */
int g_5bdd; /* DS:5BDD */
int g_5bdb; /* DS:5BDB */
int g_5bd9; /* DS:5BD9 */
int g_5bd7; /* DS:5BD7 */
int g_5bd5; /* DS:5BD5 */
int g_5bd3; /* DS:5BD3 */
int g_5bd1; /* DS:5BD1 */
int g_5bcf; /* DS:5BCF */
float step_y; /* DS:5BCB */
float step_x; /* DS:5BC7 */
float mod_y; /* DS:5BC3 */
float mod_x; /* DS:5BBF */
float sine_phase; /* DS:5BBB */
float amplitude_y; /* DS:5BB7 */
float amplitude_x; /* DS:5BB3 */
int g_5bb1; /* DS:5BB1 */
Point16 g_5ba5[3]; /* DS:5BA5 */
int g_5ba3; /* DS:5BA3 */
int g_5ba1; /* DS:5BA1 */
int g_5b9f; /* DS:5B9F */
char g_5b9e; /* DS:5B9E */
int g_5b9c; /* DS:5B9C */
int g_5b9a; /* DS:5B9A */
int g_5b98; /* DS:5B98 */
int g_5b96; /* DS:5B96 */
int g_5b94; /* DS:5B94 */
int g_5b92; /* DS:5B92 */
int g_5b90; /* DS:5B90 */
int g_5b8e; /* DS:5B8E */
int g_5b8c; /* DS:5B8C */
int g_5b8a; /* DS:5B8A */
int g_5b88; /* DS:5B88 */
int g_5b86; /* DS:5B86 */
int g_5b84; /* DS:5B84 */
int g_5b82; /* DS:5B82 */
int g_5b80; /* DS:5B80 */
int g_5b7e; /* DS:5B7E */
int g_5b7c; /* DS:5B7C */
int g_5b7a; /* DS:5B7A */
int g_5b78; /* DS:5B78 */
int g_5b76; /* DS:5B76 */
int g_5b74; /* DS:5B74 */
int g_5b72; /* DS:5B72 */
int g_5b70; /* DS:5B70 */
int g_5b6e; /* DS:5B6E */
int g_5b6c; /* DS:5B6C */
int g_5b6a; /* DS:5B6A */
unsigned int g_5b68; /* DS:5B68 */
unsigned int g_5b66; /* DS:5B66 */
int g_5b64; /* DS:5B64 */
int g_5b62; /* DS:5B62 */
int g_5b60; /* DS:5B60 */
int g_5b5e; /* DS:5B5E */
int g_5b5c; /* DS:5B5C */
int g_5b5a; /* DS:5B5A */
long g_long5b56; /* DS:5B56 */
long g_long5b52; /* DS:5B52 */
long g_long5b4e; /* DS:5B4E */
long g_long5b4a; /* DS:5B4A */
long g_long5b46; /* DS:5B46 */
long g_long5b42; /* DS:5B42 */
long g_long5b3e; /* DS:5B3E */
long g_long5b3a; /* DS:5B3A */
long g_long5b36; /* DS:5B36 */
int g_5b34; /* DS:5B34 */
int g_5b32; /* DS:5B32 */
int g_5b30; /* DS:5B30 */
int g_5b2e; /* DS:5B2E */
int g_5b2c; /* DS:5B2C */
int g_5b2a; /* DS:5B2A */
int g_5b28; /* DS:5B28 */
int g_5b26; /* DS:5B26 */
int g_5b24; /* DS:5B24 */
long g_5afc[10]; /* DS:5AFC */
long g_5ad4[10]; /* DS:5AD4 */
long g_5ad0; /* DS:5AD0 */
int g_5ace; /* DS:5ACE */
int g_5acc; /* DS:5ACC */
Point16 g_59c4[66]; /* DS:59C4 */
int g_59c2; /* DS:59C2 */
int g_59c0; /* DS:59C0 */
int g_59be; /* DS:59BE */
int g_59bc; /* DS:59BC */
int g_59ba; /* DS:59BA */
int g_59b8; /* DS:59B8 */
int g_59b6; /* DS:59B6 */
int g_59b4; /* DS:59B4 */
int g_59b2; /* DS:59B2 */
int g_59b0; /* DS:59B0 */
int g_59ae; /* DS:59AE */
int g_59ac; /* DS:59AC */
int g_59aa; /* DS:59AA */
int g_59a8; /* DS:59A8 */
int g_59a6; /* DS:59A6 */
int g_59a4; /* DS:59A4 */
int g_59a2; /* DS:59A2 */
long g_599e; /* DS:599E */
int g_599c; /* DS:599C */
int g_599a; /* DS:599A */
int g_5998; /* DS:5998 */
int g_5996; /* DS:5996 */
int g_5994; /* DS:5994 */
int g_5992; /* DS:5992 */
int g_5990; /* DS:5990 */
int g_598e; /* DS:598E */
int g_598c; /* DS:598C */
int g_598a; /* DS:598A */
int g_5988; /* DS:5988 */
int g_5986; /* DS:5986 */
int g_5984; /* DS:5984 */
int g_5982; /* DS:5982 */
int g_5980; /* DS:5980 */
int g_597e; /* DS:597E */
int g_597c; /* DS:597C */
int g_597a; /* DS:597A */
int g_5978; /* DS:5978 */
int g_5976; /* DS:5976 */
int g_5974; /* DS:5974 */
int g_5972; /* DS:5972 */
int g_5970; /* DS:5970 */
int g_596e; /* DS:596E */
int g_596c; /* DS:596C */
int g_596a; /* DS:596A */
int g_5968; /* DS:5968 */
int g_5966; /* DS:5966 */
int g_5964; /* DS:5964 */
int g_5962; /* DS:5962 */
float g_595e; /* DS:595E */
float g_595a; /* DS:595A */
float g_5956; /* DS:5956 */
float g_5952; /* DS:5952 */
float g_594e; /* DS:594E */
int g_594c; /* DS:594C */
unsigned int g_594a; /* DS:594A */
int g_5948; /* DS:5948 */
int g_5946; /* DS:5946 */
int g_5944; /* DS:5944 */
int g_5942; /* DS:5942 */
int g_5940; /* DS:5940 */
int g_593e; /* DS:593E */
int g_593c; /* DS:593C */
int g_593a; /* DS:593A */
float g_5936; /* DS:5936 */
StarTracePoint g_53be[175]; /* DS:53BE */
int g_53bc; /* DS:53BC */
int g_53ba; /* DS:53BA */
int g_53b8; /* DS:53B8 */
int g_53b6; /* DS:53B6 */
int g_53b4; /* DS:53B4 */
int g_53b2; /* DS:53B2 */
int g_53b0; /* DS:53B0 */
int g_53ae; /* DS:53AE */
int g_53ac; /* DS:53AC */
int g_53aa; /* DS:53AA */
int g_53a8; /* DS:53A8 */
int g_53a6; /* DS:53A6 */
int g_53a4; /* DS:53A4 */
int g_53a2; /* DS:53A2 */
int g_53a0; /* DS:53A0 */
int g_539e; /* DS:539E */
int g_539c; /* DS:539C */
int g_539a; /* DS:539A */
unsigned int g_5398; /* DS:5398 */
unsigned int g_5396; /* DS:5396 */
unsigned int g_5394; /* DS:5394 */
unsigned int g_5392; /* DS:5392 */
unsigned int g_5390; /* DS:5390 */
unsigned int g_538e; /* DS:538E */
unsigned int g_538c; /* DS:538C */
unsigned int g_538a; /* DS:538A */
int g_5388; /* DS:5388 */
int g_5386; /* DS:5386 */
int g_5384; /* DS:5384 */
int g_5382; /* DS:5382 */
int g_5380; /* DS:5380 */
int g_537e; /* DS:537E */
unsigned int g_537c; /* DS:537C */
unsigned int g_537a; /* DS:537A */
int g_5378; /* DS:5378 */
int g_5376; /* DS:5376 */
int g_5374; /* DS:5374 */
int g_5372; /* DS:5372 */
int g_5370; /* DS:5370 */
int g_536e; /* DS:536E */
int g_536c; /* DS:536C */
int g_536a; /* DS:536A */
int g_5368; /* DS:5368 */
int g_5366; /* DS:5366 */
int g_5364; /* DS:5364 */
int g_5362; /* DS:5362 */
int g_5360; /* DS:5360 */
int g_535e; /* DS:535E */
int g_535c; /* DS:535C */
int ellipse_direction_steps[700]; /* DS:4DE4 */
int g_4de2; /* DS:4DE2 */
int g_4de0; /* DS:4DE0 */
int g_4dde; /* DS:4DDE */
int ellipse_first_region; /* DS:4DDC */
long g_4dd8; /* DS:4DD8 */
long g_4dd4; /* DS:4DD4 */
long g_4dd0; /* DS:4DD0 */
long g_4dcc; /* DS:4DCC */
long g_4dc8; /* DS:4DC8 */
long g_4dc4; /* DS:4DC4 */
long g_4dc0; /* DS:4DC0 */
long g_4dbc; /* DS:4DBC */
long g_4db8; /* DS:4DB8 */
int g_4db6; /* DS:4DB6 */
int g_4db4; /* DS:4DB4 */
int g_4db2; /* DS:4DB2 */
int g_4db0; /* DS:4DB0 */
int g_4dae; /* DS:4DAE */
int polygon_row_right[350]; /* DS:4AF2 */
int polygon_row_left[350]; /* DS:4836 */
int error; /* DS:4834 */
int sy; /* DS:4832 */
int sx; /* DS:4830 */
int dy; /* DS:482E */
int dx; /* DS:482C */

void (interrupt far *saved_timer_vector)(void); /* DS:5FF6 */
void (interrupt far *saved_keyboard_vector)(void); /* DS:5FF2 */
