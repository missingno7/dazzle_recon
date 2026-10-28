#include <dos.h>
/* objcmp: {"function":"near_cs","address":"066a:000b","size":3,"level":"PROVISIONAL","blocker":"static-no-pubdef"} */
static unsigned near_cs(void) { return _CS; }

/* 066a:000e; RELOCATED; profile dazzle (BC++ 3.0 -ml -O1).
 * Complete extent: 269 bytes. Bindings recovered from the original image.
 */
/* objcmp: {"function":"line_step","address":"066a:000e","size":269,"symbols":{"_x":"DS:5df0","_y":"DS:5dee","_dx":"DS:482c","_dy":"DS:482e","_sx":"DS:4830","_sy":"DS:4832","_error":"DS:4834"}} */
extern int x,y,dx,dy,sx,sy,error;
#include <stdlib.h>
#include <dos.h>
int line_step(int endx,int endy,int initialize) {
    if(initialize) {
        dx=endx-x; dy=endy-y;
        sx=dx>0?1:dx==0?0:-1;
        sy=dy>0?1:dy==0?0:-1;
        dx=abs(dx)<<1; dy=abs(dy)<<1;
        if(dx>=dy) error=dy-(dx>>1); else error=dx-(dy>>1);
        if(dx==0 && dy==0) return 1; else return 0;
    } else {
        if(dx>=dy) {
            if(error>=0) {error-=dx; y+=sy;}
            x+=sx; error+=dy;
            if(x==endx) return 1; else return 0;
        } else {
            if(error>=0) {error-=dy; x+=sx;}
            y+=sy; error+=dx;
            if(y==endy) return 1;
        }
    }
    return 0;
}

/* 066a:011b; RELOCATED; profile dazzle (BC++ 3.0 -ml -O1).
 * Complete extent: 336 bytes. Bindings recovered from the original image.
 */
/* objcmp: {"function":"choose_color","address":"066a:011b","size":336,"symbols":{"_x":"DS:5df0","_y":"DS:5dee","_mode":"DS:7484","_phase":"DS:5daa","_colors":"DS:5db6","_color":"DS:7488","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXURSH@":"0000:13d6"}} */
#include <stdlib.h>
extern int x,y,mode,phase;
extern unsigned colors,color;
void choose_color(int kind) {
    unsigned long a,b;
    switch(kind) {
    default: if(mode!=4) color++; else {phase++; if((phase&=3)==0) color++;} break;
    case 1: color=random(colors); break;
    case 2: color=abs(x); break;
    case 3: color=abs(y); break;
    case 4: color=abs(x)+abs(y); break;
    case 5: a=x*x; b=y*y; a+=b; color=(a>>5)&(colors-1); break;
    case 6: a=x*x; b=y*y; a-=b; color=(a>>5)&(colors-1); break;
    case 7: b=(y*y)>>5; color=b-abs(x); break;
    case 8: a=abs(x*y)>>5; color=a; break;
    }
    color&=colors-1;
    if(color<3) color=3;
}

/* 066a:026b; RELOCATED; profile dazzle (BC++ 3.0 -ml -O1).
 * Complete extent: 80 bytes. Bindings recovered from the original image.
 */
/* objcmp: {"function":"square","address":"066a:026b","size":80,"symbols":{"_x":"DS:5df0","_y":"DS:5dee","_plot":"DS:5c3f","_tick":"DS:5d7a","_limit":"DS:5da6","_service":"11c1:2515"}} */
extern int x,y;
extern unsigned tick,limit;
extern void (*plot)(int);
extern void service(void);
void square(int n,int color) {
    int i,j;
    for(i=1;i<=n;i++) {
        for(j=1;j<=n;j++) { plot(color); y++; }
        x++; y-=n;
        if(tick>=limit) service();
    }
    x-=n;
}

extern int draw_x,draw_y,screen_half_width,screen_half_height;
extern float g_06af,g_06b3;
extern const float g_06bf;
extern const double g_06e7;
extern unsigned int draw_color,color_count,draw_symmetry;
extern int drawing_delay;
extern unsigned int palette_elapsed_ticks,palette_cycle_interval;
extern int g_5cbb;
extern int g_4dae,g_4db0,g_4db2,g_4db4,g_4db6;
extern long g_4db8,g_4dbc;
extern long g_4dc0,g_4dc4,g_4dc8,g_4dcc;
extern long g_4dd0,g_4dd4,g_4dd8;
extern int g_4ddc;
extern int g_4836[],g_4af2[],g_4de4[];
extern int g_5368,g_536a,g_536c,g_536e,g_5370,g_5372;
extern int g_5382,g_5384,g_5386,g_5388;
extern unsigned int g_538a;
extern unsigned int g_538c,g_538e,g_5390,g_5392,g_5394,g_5396,g_5398;
extern int g_5dae,g_5dc0,g_5942,g_5944,g_5946,g_5948,g_594c;
extern unsigned int g_594a;
extern int g_593a,g_593c,g_593e,g_5940;
extern unsigned int g_5c4f,g_5c51;
extern unsigned char far *g_5c53;
extern int g_5374,g_5376,g_5378,g_537e,g_5380;
extern volatile int g_537a,g_537c,g_6025;
extern int g_5968,g_596a,g_596c,g_596e,g_5970,g_597c,g_597e,g_5980,g_5984,g_599a;
extern int g_5972;
extern int g_5956,g_5958,g_595e,g_5960,g_5962,g_5964,g_5966;
extern int g_00a6[],g_00ae[];
extern int g_00b6[],g_00bc[];
extern int g_5db0,g_5db2;
extern int g_535c,g_535e,g_5360,g_5362,g_5364,g_5366;
extern int g_4dde,g_4de0,g_4de2;
extern unsigned int g_5c3f,g_5c41,g_5d7e,g_5d80,g_5d92;
extern int poll_graphics_keys(void);
extern void cycle_palette(void);
extern unsigned char (*read_pixel)(int,int);
extern void far ega_plot_backend(void);
extern double cos(double),sin(double);
typedef struct { long x,y; } long_pair;
typedef struct { int x,y; } int_pair;
extern int g_539a,g_539c,g_539e,g_53a0,g_53a2,g_53a4,g_53a6,g_53a8;
extern int g_53aa,g_53ac,g_53ae,g_53b0,g_53b2,g_53b4,g_53b6,g_53b8;
extern int g_53ba,g_53bc;
extern long_pair g_53be[];
extern int g_5974,g_5976,g_5978,g_597a,g_5982,g_5984,g_5986,g_5988,g_598a,g_598c,g_598e;
extern long g_5990,g_5992,g_5994,g_5996,g_599e,g_59a0;
extern int g_599c,g_59a2,g_59a4,g_59a6,g_59a8,g_59aa,g_59ac,g_59ae;
extern int g_59b0,g_59b2,g_59b4,g_59b6,g_59b8,g_59ba,g_59bc,g_59be,g_59c0,g_59c2;
extern int_pair g_59c4[];
extern int g_5acc,g_5ace;
extern long g_5ad0;
extern long g_5ad4[],g_5afc[];


/* objcmp: {"function":"draw_polygon","address":"066a:02bb","size":797,"symbols":{"_line_step":"066a:000e","_draw_x":"DS:5df0","_draw_y":"DS:5dee","_screen_half_width":"DS:5db0","_screen_half_height":"DS:5db2","_draw_color":"DS:7488","_color_count":"DS:5db6","_draw_symmetry":"DS:5caf","_drawing_delay":"DS:5dc6","_g_4dae":"DS:4dae","_g_4db0":"DS:4db0","_g_4db2":"DS:4db2","_g_4db4":"DS:4db4","_g_4db6":"DS:4db6","_g_4836":"DS:4836","_g_4af2":"DS:4af2","_plot":"DS:5c3f","_cycle_palette":"11c1:2515","_palette_elapsed_ticks":"DS:5d7a","_palette_cycle_interval":"DS:5da6","_read_pixel":"DS:5c3b","_draw_ellipse":"066a:089b","_ellipse_step":"066a:05d8","_rand":"0000:1694","F_LDIV@":"0000:12e7","F_LXMUL@":"0000:11e3","_g_5dae":"DS:5dae","_g_5db0":"DS:5db0","_g_5db2":"DS:5db2","_g_6025":"DS:6025","_g_5942":"DS:5942","_g_5944":"DS:5944","_g_5946":"DS:5946","_g_5948":"DS:5948","_g_594a":"DS:594a","_g_594c":"DS:594c","_g_5968":"DS:5968","_g_596a":"DS:596a","_g_596c":"DS:596c","_g_596e":"DS:596e","_g_5970":"DS:5970","_g_5972":"DS:5972","_g_597c":"DS:597c","_g_597e":"DS:597e","_g_5980":"DS:5980","_g_5984":"DS:5984","_g_599a":"DS:599a","_g_5382":"DS:5382","_g_5384":"DS:5384","_g_5386":"DS:5386","_g_5388":"DS:5388","_g_538a":"DS:538a","_g_538c":"DS:538c","_g_538e":"DS:538e","_g_5390":"DS:5390","_g_5392":"DS:5392","_g_5394":"DS:5394","_g_5396":"DS:5396","_g_5398":"DS:5398","_g_5cbb":"DS:5cbb","_g_5c3f":"DS:5c3f","_g_5c41":"DS:5c41","_g_5d7e":"DS:5d7e","_g_5d80":"DS:5d80","_g_5d92":"DS:5d92","_poll_graphics_keys":"11c1:18e7","_ega_plot_backend":"1899:012d","_g_4db8":"DS:4db8","_g_4dba":"DS:4dba","_g_4dbc":"DS:4dbc","_g_4dbe":"DS:4dbe","_g_4dc0":"DS:4dc0","_g_4dc2":"DS:4dc2","_g_4dc4":"DS:4dc4","_g_4dc6":"DS:4dc6","_g_4dc8":"DS:4dc8","_g_4dca":"DS:4dca","_g_4dcc":"DS:4dcc","_g_4dce":"DS:4dce","_g_4de4":"DS:4de4","_g_4dde":"DS:4dde","_g_4de0":"DS:4de0","_g_4de2":"DS:4de2","_g_535c":"DS:535c","_g_535e":"DS:535e","_g_5360":"DS:5360","_g_5362":"DS:5362","_g_5364":"DS:5364","_g_5366":"DS:5366","F_FTOL@":"0000:0fbe","F_LXRSH@":"0000:13b6","_g_06af":"DS:06af","_g_06b3":"DS:06b3","_g_5374":"DS:5374","_g_5376":"DS:5376","_g_5378":"DS:5378","_g_537a":"DS:537a","_g_537c":"DS:537c","_g_537e":"DS:537e","_g_5380":"DS:5380","_g_5c4f":"DS:5c4f","_g_5c51":"DS:5c51","_g_5c53":"DS:5c53","_g_5dc0":"DS:5dc0","_cos":"0000:0f6e","_sin":"0000:0e75","_g_00a6":"DS:00a6","_g_00ae":"DS:00ae","_g_593a":"DS:593a","_g_593c":"DS:593c","_g_593e":"DS:593e","_g_5940":"DS:5940","F_LXLSH@":"0000:1395","_g_00b6":"DS:00b6","_g_00bc":"DS:00bc","_g_5956":"DS:5956","_g_5958":"DS:5958","_g_595e":"DS:595e","_g_5960":"DS:5960","_g_5962":"DS:5962","_g_5964":"DS:5964","_g_5966":"DS:5966","_g_539a":"DS:539a","_g_539c":"DS:539c","_g_539e":"DS:539e","_g_53a0":"DS:53a0","_g_53a2":"DS:53a2","_g_53a4":"DS:53a4","_g_53a6":"DS:53a6","_g_53a8":"DS:53a8","_g_53aa":"DS:53aa","_g_53ac":"DS:53ac","_g_53ae":"DS:53ae","_g_53b0":"DS:53b0","_g_53b2":"DS:53b2","_g_53b4":"DS:53b4","_g_53b6":"DS:53b6","_g_53b8":"DS:53b8","_g_53ba":"DS:53ba","_g_53bc":"DS:53bc","_g_53be":"DS:53be","_g_53c2":"DS:53c2","_g_5974":"DS:5974","_g_5976":"DS:5976","_g_5978":"DS:5978","_g_597a":"DS:597a","_g_5982":"DS:5982","_g_5986":"DS:5986","_g_5988":"DS:5988","_g_598a":"DS:598a","_g_598c":"DS:598c","_g_598e":"DS:598e","_g_5990":"DS:5990","_g_5992":"DS:5992","_g_5994":"DS:5994","_g_5996":"DS:5996","_g_599e":"DS:599e","_g_59a0":"DS:59a0","_g_599c":"DS:599c","_g_59a2":"DS:59a2","_g_59a4":"DS:59a4","_g_59a6":"DS:59a6","_g_59a8":"DS:59a8","_g_59aa":"DS:59aa","_g_59ac":"DS:59ac","_g_59ae":"DS:59ae","_g_59b0":"DS:59b0","_g_59b2":"DS:59b2","_g_59b4":"DS:59b4","_g_59b6":"DS:59b6","_g_59b8":"DS:59b8","_g_59ba":"DS:59ba","_g_59bc":"DS:59bc","_g_59be":"DS:59be","_g_59c0":"DS:59c0","_g_59c2":"DS:59c2","_g_59c4":"DS:59c4","_g_5acc":"DS:5acc","_g_5ace":"DS:5ace","_g_5ad0":"DS:5ad0","_g_5ad2":"DS:5ad2","_g_5ad4":"DS:5ad4","_g_5afc":"DS:5afc","_g_4dd0":"DS:4dd0","_g_4dd4":"DS:4dd4","_g_4dd8":"DS:4dd8","_g_4ddc":"DS:4ddc"},"absolute_symbols":{"FIDRQQ":"0x5c32","FIWRQQ":"0xa23d"}} */
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
            g_4836[i]=screen_half_width;
            g_4af2[i]=-screen_half_width;
        }
        for(i=0;i<n;i++) {
            draw_x=px[i]; draw_y=py[i]-g_4db0;
            end_x=px[i+1]; end_y=py[i+1]-g_4db0; done=1;
            do {
                done=line_step(end_x,end_y,done);
                if(draw_x<g_4836[draw_y]) g_4836[draw_y]=draw_x;
                if(draw_x>g_4af2[draw_y]) g_4af2[draw_y]=draw_x;
            } while(!done);
            if(palette_elapsed_ticks>=palette_cycle_interval) cycle_palette();
        }
    }
    if(flags&2) {
        draw_color=fill_color;
        saved_fill=fill_color;
        color_mode=flags&12;
        draw_y=g_4db0+g_4db4;
        for(i=0;i<=g_4db6;i++) {
            if(color_mode==4) draw_color=saved_fill;
            draw_x=g_4836[i]+g_4db2;
            row_end=g_4af2[i]+g_4db2;
            while(draw_x<=row_end) {
                (*plot)(draw_symmetry);
                draw_x++;
                if(color_mode==4) { draw_color++; if(draw_color>=color_count) draw_color=3; }
                g_5cbb++;
                if(palette_elapsed_ticks>=palette_cycle_interval) cycle_palette();
            }
            if(color_mode==8) { draw_color++; if(draw_color>=color_count) draw_color=3; }
            draw_y++;
        }
    }
    if(flags&1) {
        draw_color=edge_color;
        if(flags&16) color_mode=1;
        else color_mode=0;
        for(i=0;i<n;i++) {
            draw_x=px[i]+g_4db2; draw_y=py[i]+g_4db4;
            end_x=px[i+1]+g_4db2; end_y=py[i+1]+g_4db4; done=1;
            do {
                done=line_step(end_x,end_y,done);
                (*plot)(draw_symmetry);
                if(color_mode) { draw_color++; if(draw_color>=color_count) draw_color=3; }
                g_5cbb++;
                if(palette_elapsed_ticks>=palette_cycle_interval) cycle_palette();
            } while(!done);
        }
    }
    px[0]=g_4db2; py[0]=g_4db4;
}

/* objcmp: {"function":"ellipse_step","address":"066a:05d8","size":707,"absolute_symbols":{"FIDRQQ":"0x5c32"},"symbols":{"_g_06af":"DS:06af","_g_06b3":"DS:06b3"},"level":"PROVISIONAL","blocker":"extent 683/707; 066a:05eb JMP 0754 vs target JMP 0763; radius conversion FP body is 24 bytes shorter"} */
int ellipse_step(int radius_y,int radius_x,int initialize) {
    float raw_radius;
    if(initialize) {
        g_4dbc=radius_y;
        if(radius_x) g_4db8=radius_x;
        else {
            raw_radius=(double)screen_half_width/screen_half_height*g_06af*radius_y;
            g_4db8=(long)raw_radius;
            if(raw_radius-(double)g_4db8>g_06b3) g_4db8++;
        }
        draw_y=radius_y;
        draw_x=0;
        g_4dc0=g_4db8*g_4db8;
        g_4dc4=g_4dbc*g_4dbc;
        g_4dc8=g_4dc0*2L;
        g_4dcc=g_4dc4*2L;
        g_4dd4=0;
        g_4dd8=g_4dc8*g_4dbc;
        g_4dd0=g_4dc4-(g_4dc0*g_4dbc)+(g_4dc0>>2);
        g_4ddc=1;
        return radius_y==0;
    }
    if(g_4ddc) {
        g_4dd0+=g_4dd4+g_4dc4;
        g_4dd4+=g_4dcc;
        draw_x++;
        if(g_4dd0>=0) {
            draw_y--;
            g_4dd8-=g_4dc8;
            g_4dd0-=g_4dd8;
        }
        if(g_4dd8<=g_4dd4) {
            g_4dd0+=((g_4dd4+g_4dd8)>>1)+((g_4dc4-g_4dc0)>>2);
            g_4ddc=0;
        }
    } else {
        g_4dd0+=g_4dc0-g_4dd8;
        g_4dd8-=g_4dc8;
        draw_y--;
        if(g_4dd0<0) {
            g_4dd4+=g_4dcc;
            g_4dd0+=g_4dd4;
            draw_x++;
        }
    }
    return draw_y==0;
}

/* objcmp: {"function":"draw_ellipse","address":"066a:089b","size":778} */
void draw_ellipse(int rebuild,int ry,int rx,int cx,int cy,unsigned fill,unsigned edge,unsigned flags) {
    if(rebuild) {
        draw_y=ry; g_4de2=ry;
        g_4de0=draw_x=g_4dde=0;
        g_535c=1;
        do {
            g_535c=ellipse_step(ry,rx,g_535c);
            if(draw_y==g_4de2) g_4de4[g_4dde]=0;
            else if(draw_x>g_4de0) g_4de4[g_4dde]=1;
            else g_4de4[g_4dde]=-1;
            g_4de2=draw_y; g_4de0=draw_x; g_4dde++;
            if(palette_elapsed_ticks>=palette_cycle_interval) cycle_palette();
        } while(g_535c==0);
        g_5366=g_4dde;
    }
    if(flags&2) {
        draw_color=fill; g_5364=fill;
        g_5362=flags&0x1c;
        g_4dde=1; g_4de0=0; g_4de2=ry;
        while(g_4dde<=g_5366) {
            while(g_4de4[g_4dde]==0 && g_4dde<g_5366) {
                g_4de0++;
                g_4dde++;
            }
            g_535e=g_4de0-1;
            if(g_5362==8) g_5360=0;
            else g_5360=-g_535e;
            if(g_5362&0x0c) draw_color=g_5364;
            do {
                draw_x=cx+g_535e; draw_y=cy+g_4de2; (*plot)(draw_symmetry);
                draw_x=cx+g_535e; draw_y=cy-g_4de2; (*plot)(1);
                if(g_5362==8) {
                    draw_x=cx-g_535e; draw_y=cy-g_4de2; (*plot)(draw_symmetry);
                    draw_x=cx-g_535e; draw_y=cy+g_4de2; (*plot)(draw_symmetry);
                }
                if(g_5362&0x0c) {
                    draw_color++;
                    if(draw_color>=color_count) draw_color=3;
                }
                g_5cbb++;
                if(palette_elapsed_ticks>=palette_cycle_interval) cycle_palette();
                g_535e--;
            } while(g_535e>=g_5360);
            if(g_5362==0x10) {
                draw_color++;
                if(draw_color>=color_count) draw_color=3;
            }
            if(g_4de4[g_4dde]>0) g_4de0++;
            g_4de2--;
            g_4dde++;
        }
    }
    if(flags&1) {
        draw_color=edge;
        g_4dde=1; g_4de0=0; g_4de2=ry;
        while(g_4dde<=g_5366) {
            draw_x=cx+g_4de0; draw_y=cy+g_4de2; (*plot)(draw_symmetry);
            draw_x=cx+g_4de0; draw_y=cy-g_4de2; (*plot)(draw_symmetry);
            draw_x=cx-g_4de0; draw_y=cy-g_4de2; (*plot)(draw_symmetry);
            draw_x=cx-g_4de0; draw_y=cy+g_4de2; (*plot)(draw_symmetry);
            if(flags&0x20) {
                draw_color++;
                if(draw_color>=color_count) draw_color=3;
            }
            g_5cbb++;
            if(g_4de4[g_4dde]==0) g_4de0++;
            else {
                if(g_4de4[g_4dde]>0) g_4de0++;
                g_4de2--;
            }
            g_4dde++;
            if(palette_elapsed_ticks>=palette_cycle_interval) cycle_palette();
        }
    }
}

/* objcmp: {"function":"swap_pixel_rectangles","address":"066a:0ba5","size":296} */
void swap_pixel_rectangles(int flip,int x1,int y1,int x2,int y2,int width,int height) {
    volatile int row;
    int color1,color2,x1col,y1row,x2col,y2row,step,x;
    width--;
    step=1;
    if(g_5c41==FP_SEG(ega_plot_backend) && g_5c3f==FP_OFF(ega_plot_backend)) {
        y1>>=1; y2>>=1; y1<<=1; y2<<=1;
        height-=2; height>>=1; height<<=1;
        step=2;
    } else height--;
    for(row=0;row<=height;row+=step) {
        y1row=y1+row;
        if(flip&1) y2row=y2+height-row;
        else y2row=y2+row;
        for(x=0;x<=width;x++) {
            x1col=x1+x;
            if(flip<2) x2col=x2+x;
            else x2col=x2+width-x;
            color1=(*read_pixel)(x1col,y1row);
            color2=(*read_pixel)(x2col,y2row);
            draw_color=color2; draw_x=x1col; draw_y=y1row; (*plot)(1);
            draw_color=color1; draw_x=x2col; draw_y=y2row; (*plot)(1);
        }
        if(palette_elapsed_ticks>=palette_cycle_interval) cycle_palette();
        if(g_5d7e || g_5d80) if(poll_graphics_keys()) break;
        if(g_5d92==0x6e) break;
    }
}

/* objcmp: {"function":"arches","address":"066a:0ccd","size":153,"symbols":{"_screen_half_width":"DS:5db0","_screen_half_height":"DS:5db2","_g_5368":"DS:5368","_g_536a":"DS:536a","_g_536c":"DS:536c","_g_536e":"DS:536e","_g_5370":"DS:5370","_g_5372":"DS:5372","_rand":"0000:1694","F_LDIV@":"0000:12e7","_draw_ellipse":"066a:089b"}} */
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

/* objcmp: {"function":"atom","address":"066a:0d66","size":304,"level":"PROVISIONAL","blocker":"extent 288/304; 066a:0d69 SUB SP,0c vs target SUB SP,10; angle/radius FP temporaries differ"} */
void atom(int rebuild) {
    double angle,radius;
    if(!rebuild && g_5374>1) goto calculate;
    g_5378=0xff;
    g_5376=0;
    g_5374=g_5dae/2;
    if(rebuild) draw_color=3&g_5dc0;
    g_5378++;
    g_5378&=15;
    if(g_5378==0) {
        g_5376+=g_5dae/2-g_5374+45;
        if(g_5376>=18*g_5dae || g_5376<0) {
            g_5374-=g_5dae/20;
            g_5376=0;
            draw_color++;
            if(draw_color>=color_count) draw_color=3;
        }
    }
calculate:
    angle=(double)g_5376*3.141592653589793/180.0;
    radius=(double)g_5374;
    draw_x=cos(angle)*radius;
    draw_y=sin(angle)*radius;
    if(mode==4) draw_y/=2;
    else draw_x++;
}

/* objcmp: {"function":"balls","address":"066a:0e96","size":416,"level":"PROVISIONAL","blocker":"extent 409/416; 066a:0f71 JL 0f9f vs target JL 0fc6; run-count comparison uses DX instead of target AX"} */
void balls(int rebuild) {
    int pixel;
    register unsigned int step;
    if(!rebuild) {
        if(g_537c<g_5c51) goto advance;
        if(g_537a<g_5c4f) goto advance;
    }
    g_5386=g_5388=g_5384=0;
        g_5382=-1; g_538a=5;
        g_5380=random(g_5db2+(g_5c4f>>1))-g_5c4f;
        g_537e=random(g_5db0+(g_5c51>>1))-g_5c51;
        g_537c=g_5c51>>1; g_537a=g_5c4f>>1;
        g_6025=random(3)+1;
advance:
    do {
        if(g_5382++>=g_5384) {
            g_5382=1;
            g_538a++;
            step=g_538a;
            if(step>3) {
                g_5384+=2;
                g_538a=0;
                g_537c+=g_6025;
                g_537a-=g_6025;
            }
        }
        g_5386=g_00a6[g_538a]*g_6025;
        g_5388=g_00ae[g_538a]*g_6025;
        g_537c+=g_5386; g_537a+=g_5388;
        pixel=g_5c53[g_537c*g_5c4f+g_537a];
    } while(pixel==0);
    if(g_537c>=g_5c51 || g_537a>=g_5c4f) return;
    draw_color=pixel&g_5dc0;
    draw_x=g_537e+g_537c/g_6025;
    draw_y=g_5380+g_537a/g_6025;
}

/* objcmp: {"function":"bars","address":"066a:1036","size":373} */
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

/* objcmp: {"function":"bigstar","address":"066a:11ab","size":1147,"level":"PROVISIONAL","blocker":"extent 1155/1147; 066a:11ae SUB SP,30 vs target SUB SP,0a; local FP frame allocation differs"} */
void bigstar(int rebuild) {
    int i,done,point_count,step,branch;
    double fx,fy,scale_x,scale_y,aspect;
    if(rebuild || g_53a8==0) {
        g_53a8=0;
        g_539a=screen_half_height-1;
        g_539c=screen_half_width-1;
        fx=(double)g_539a;
        fy=(double)g_539c;
        aspect=(double)screen_half_width/(double)screen_half_height;
        g_539a=(int)(fx*aspect);
        g_539c=(int)(fy*aspect);
        g_539e=random(0x8000)*3+3;
        g_539e/=0x8000;
        g_53a0=g_539c;
        g_53a2=(int)((double)g_539a/2.0);
        g_53a4=(int)((double)g_539c/2.0);
        step=random(3)+1;
        if(step<0) step=0;
        g_53b0=step;
        g_53ae=1<<step;
        g_53b8=0;
        g_53aa=1;
        if(mode==4) g_53b2=2; else g_53b2=random(2);
        g_53bc=random(color_count-1);
        draw_color=g_53bc&g_5dc0;
        scale_x=(double)g_539a/16.0;
        scale_y=(double)g_539c/16.0;
        g_53b4=g_53b6=0;
        draw_x=draw_y=0;
        g_53a6=1;
        i=0;
        do {
            g_53a6=line_step((int)(scale_x*(g_53a2+i)),
                             (int)(scale_y*(g_53a4+i)),g_53a6);
            g_53be[g_53b4].x=draw_x;
            g_53b4++;
            if(palette_elapsed_ticks>=palette_cycle_interval) cycle_palette();
        } while(!g_53a6 && g_53b4<32);
        g_53a6=1;
        g_53b6=0;
        do {
            g_53a6=line_step((int)(scale_x*(g_53a4+i)),
                             (int)(scale_y*(g_53a2+i)),g_53a6);
            g_53be[g_53b6].y=draw_y;
            g_53b6++;
            if(palette_elapsed_ticks>=palette_cycle_interval) cycle_palette();
        } while(!g_53a6 && g_53b6<32);
        g_53aa=1;
        g_53ba=1;
    }
    if(g_53a6==0) {
        g_5cbb=0x7d00;
        g_53a8=1;
        return;
    }
    point_count=g_53b4;
    if(g_53b8>=point_count) g_53aa|=2;
    if(g_53b2==1) draw_color=g_53bc&g_5dc0;
    branch=g_53aa;
    switch(branch) {
    case 0:
    case 1:
        g_53a2=0; g_53a4=g_539c; g_53aa=2; g_53ba=1;
        break;
    case 2:
        g_53a2=g_539e; g_53a4=g_53a0; g_53aa=3;
        break;
    case 3:
        g_53a2=g_539e; g_53a4=g_53a0; g_53aa=4; g_53ba=0;
        break;
    case 4:
        g_53a2=g_539a; g_53a4=0; g_53aa=1;
        break;
    default:
        break;
    }
    if(g_53ba) {
        draw_x=(int)g_53be[g_53b8].x;
        draw_y=(int)g_53be[g_53b8].y;
    } else {
        draw_x=(int)g_53be[g_53b8].y;
        draw_y=(int)g_53be[g_53b8].x;
    }
    if(g_53b8>=g_53b6) g_53aa&=1;
    if(g_53aa==1) {
        g_53b8+=g_53ae;
        if(g_53b2==0) { draw_color++; if(draw_color>=color_count) draw_color=3; }
        if(g_53b2==2) {
            phase++; phase&=3;
            if(phase==0) { draw_color++; if(draw_color>=color_count) draw_color=3; }
        }
        if(g_53b8>=g_53b4 && g_53b8>=g_53b6) g_53ac=1;
    }
    done=line_step(g_53a2,g_53a4,g_53a6);
    g_53a6=done;
    (*plot)(0);
    if(g_53b2==1) { draw_color++; if(draw_color>=color_count) draw_color=3; }
    g_5cbb=1;
    if(palette_elapsed_ticks>=palette_cycle_interval) cycle_palette();
}

/* objcmp: {"function":"boxes","address":"066a:1634","size":427} */
void boxes(int rebuild) {
    if(rebuild || g_594c) {
        g_5942=random(g_5dae/3);
        g_5944=random(g_5dae/3);
        g_593a=g_5942; g_593c=g_5944;
        g_593e=random(g_5dae/4)+g_5dae/4;
        g_5940=random(g_5dae/4)+g_5dae/4;
        g_5946=1; g_594c=0; g_5948=1; g_594a=0;
        draw_x=g_593a; draw_y=g_593c;
        draw_color=3&g_5dc0;
    }
    g_5946=line_step(g_5942,g_5944,g_5946);
    choose_color(0);
    if(g_5946) {
        draw_color=3&g_5dc0;
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

/* objcmp: {"function":"cones","address":"066a:17df","size":467,"level":"PROVISIONAL","blocker":"extent 458/467; 066a:18aa JE 18b3 vs target JE 18bc; equal direction cases merge in candidate switch"} */
void cones(int rebuild) {
    int i,j,dx,dy,kind,limit;
    register int cx,cy,variant;
    (void)rebuild;
    draw_color=random(color_count);
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
        break;
    case 2:
        cy=screen_half_height-cy;
        break;
    }
    dx=g_00b6[kind*6+variant];
    dy=g_00bc[kind*6+variant];
    if(variant && random(2)) { dx>>=1; dy>>=1; }
    i=0; j=0;
    limit=random(screen_half_height/8)+screen_half_height/16;
    while(i<limit) {
        draw_ellipse(1,i,j,cx,cy,0,draw_color,1);
        draw_color++;
        if(draw_color>=color_count) draw_color=3;
        cx+=dx; cy+=dy;
        i++; j++;
    }
}

/* objcmp: {"function":"corkscrew","address":"066a:19b2","size":432,"level":"PROVISIONAL","blocker":"extent 361/432; 066a:19b5 SUB SP,0a vs target SUB SP,0c; candidate also saves SI before the parameter check"} */
void corkscrew(int rebuild) {
    double phase;
    int radius;
    if(rebuild || draw_x>g_5dae) {
        g_5960=0; g_595e=0; g_5958=0; g_5956=0;
        g_5958=random(2)+1;
        g_5962=random(g_5dae/2);
        g_5964=random(g_5dae/2);
        draw_color++;
        if(draw_color>=color_count) draw_color=3;
    }
    phase=(double)(g_5966*g_5958)*0.392699081698724;
    radius=g_5956+g_595e+g_5960;
    draw_x=(int)(cos(phase)*radius)+g_5962;
    draw_y=(int)(sin(phase)*radius)+g_5964;
    g_5956++;
    g_595e+=g_5956;
    g_5960+=g_595e;
    g_5966++;
    g_5966&=15;
    if(g_5966==0) {
        draw_color++;
        if(draw_color>=color_count) draw_color=3;
    }
}

/* objcmp: {"function":"curves","address":"066a:1b62","size":352,"symbols":{"_g_06bf":"DS:06bf","_g_06e7":"DS:06e7"}} */
void curves(int rebuild) {
    float a,b;
    if(rebuild) {
        g_596a=(g_596c=1);
        g_5972=g_5dae/g_06bf;
        g_5968=255;
        g_596e=random(8)+2;
        g_5970=random(6)+4;
    }
    g_5968++;
    if((g_5968=g_5968&15)==0) {
        a=g_596a*g_06e7;
        b=g_596c*g_06e7;
        draw_x=(int)(cos(a)*g_5972)+4;
        draw_y=(int)(sin(b)*g_5972)+4;
        g_596a+=g_596e;
        g_596c+=g_5970;
        if(g_596a>32000) g_596a=0;
        if(g_596c>32000) g_596c=0;
        if((draw_y&2) && ++draw_color>=color_count) draw_color=3;
    } else {
        draw_x++;
    }
}

/* objcmp: {"function":"cycloids","address":"066a:1cc2","size":1107,"level":"PROVISIONAL","blocker":"extent 901/1107; target PUSH DI at 066a:1cc9 is absent; FP point construction is 206 bytes short"} */
void cycloids(int rebuild) {
    int done;
    if(rebuild || g_599a>=g_5984) {
        g_599e=((long)screen_half_width/(long)screen_half_height)*3L/4L;
        g_59a0=g_599e;
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
        g_5984=random(10)+1;
        g_5986=random(2);
        g_599c=0;
        draw_color=random(color_count-1)&g_5dc0;
        g_5974=0;
        g_5976=0;
        g_5978=0;
    }
    if(!g_599c) {
        if(g_5986==0) { draw_color++; if(draw_color>=color_count) draw_color=3; }
        g_5980+=g_5988;
        if(g_5980<0) g_5980=-g_5980;
        g_5974=(g_5974+g_598a)&63;
        g_5976=g_5974;
        g_5978=0;
        g_5990=(long)((double)g_5974*(double)g_5980/64.0);
        g_5992=(long)((double)g_5974*(double)g_5982/64.0);
        g_5994=(long)((double)g_5976*(double)g_5980/64.0);
        g_5996=(long)((double)g_5976*(double)g_5982/64.0);
        g_5976=(g_5976+g_598c)&63;
        g_5978=(g_5978+g_598e)&63;
        g_597a=(g_5976-g_5978)&63;
        draw_x=(int)(g_5990+g_5994);
        draw_y=(int)(g_5992+g_5996);
    }
    done=1;
    do {
        done=line_step(draw_x,draw_y,done);
        (*plot)(0);
        if(g_5986) { draw_color++; if(draw_color>=color_count) draw_color=3; }
        g_5cbb++;
        if(palette_elapsed_ticks>=palette_cycle_interval) cycle_palette();
    } while(!done);
    g_599a++;
    if(g_599a>=g_5984) {
        g_599c=1;
        draw_x=g_5dae+1;
        draw_y=g_5dae+1;
    }
}

/* objcmp: {"function":"daisies","address":"066a:2115","size":1238,"level":"PROVISIONAL","blocker":"extent 1260/1238; 066a:2118 SUB SP,2 vs target CMP [BP+6],0; point-table and petal path is 22 bytes longer"} */
void daisies(int rebuild) {
    if(rebuild || g_59ae>=g_5ace) {
        g_5ad0=((long)screen_half_width/(long)screen_half_height)*3L/4L;
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
        g_59c2=random(color_count-1);
        g_59b4=0;
        while(g_59b4<9) {
            if(g_59b4>1 && g_59b4<9) g_59b6=g_59b4*2-1;
            else if(g_59b4<=1) g_59b6=g_59b4;
            else g_59b6=16;
            g_5ad4[g_59b4]=(long)((double)g_59ba*g_59b6/16.0);
            g_5afc[g_59b4]=(long)((double)g_59ba*g_59b6/32.0);
            g_59b4++;
            if(palette_elapsed_ticks>=palette_cycle_interval) cycle_palette();
        }
        g_59ae=g_5acc;
    }
    draw_x=g_59aa;
    draw_y=g_59ac;
    g_59a6=0;
    g_59a8=0;
    g_59b4=0;
    while(g_59b4<g_5ace) {
        g_59b6=g_59ae;
        g_59a2=(int)g_5ad4[g_59b6];
        g_59a4=(int)g_5afc[g_59b6];
        g_59ae+=g_59be;
        g_59c4[g_59b4].x=g_59a2;
        g_59c4[g_59b4].y=g_59a4;
        g_59b4++;
    }
    g_59b2=0;
    g_59b0=1;
    draw_x=draw_y=0;
    g_59b0=line_step(g_59aa,g_59ac,g_59b0);
    while(!g_59b0 && g_59b2<64) {
        g_59c4[g_59b2].x=draw_x;
        g_59c4[g_59b2].y=draw_y;
        g_59b2++;
        g_59b0=line_step(g_59aa,g_59ac,g_59b0);
        if(palette_elapsed_ticks>=palette_cycle_interval) cycle_palette();
    }
    g_59b4=0;
    while(g_59b4<g_59b2) {
        draw_x=g_59c4[g_59b4].x;
        draw_y=g_59c4[g_59b4].y;
        if(g_59c0) { draw_color++; if(draw_color>=color_count) draw_color=3; }
        else draw_color=g_59c2&g_5dc0;
        g_59b0=1;
        g_59b6=draw_x;
        g_59a6=draw_y;
        do {
            draw_x=g_59aa+g_59b6; draw_y=g_59ac+g_59a6; (*plot)(1);
            draw_x=g_59aa-g_59b6; draw_y=g_59ac+g_59a6; (*plot)(1);
            draw_x=g_59aa+g_59b6; draw_y=g_59ac-g_59a6; (*plot)(1);
            draw_x=g_59aa-g_59b6; draw_y=g_59ac-g_59a6; (*plot)(1);
            draw_x=g_59b6; draw_y=g_59a6;
            g_5cbb++;
            if(palette_elapsed_ticks>=palette_cycle_interval) cycle_palette();
            g_59b0=line_step(g_59aa,g_59ac,g_59b0);
        } while(!g_59b0);
        g_59b4++;
    }
    if(g_59b2==0) g_59bc=-g_59bc;
    g_59ba+=g_59bc;
    g_59ae++;
    draw_x=g_5dae+1;
    draw_y=g_5dae+1;
}
