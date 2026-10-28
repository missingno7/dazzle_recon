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
extern unsigned int draw_color,color_count,draw_symmetry;
extern int drawing_delay;
extern unsigned int palette_elapsed_ticks,palette_cycle_interval;
extern int g_5cbb;
extern int g_4dae,g_4db0,g_4db2,g_4db4,g_4db6;
extern int g_4db8,g_4dba,g_4dbc,g_4dbe,g_4dc0,g_4dc2;
extern int g_4dc4,g_4dc6,g_4dc8,g_4dca,g_4dcc,g_4dce;
extern int g_4dd0,g_4dd2,g_4dd4,g_4dd6,g_4dd8,g_4dda,g_4ddc;
extern int g_4836[],g_4af2[],g_4de4[];
extern int g_5368,g_536a,g_536c,g_536e,g_5370,g_5372;
extern unsigned int g_5382,g_5384,g_5386,g_5388,g_538a,g_538c,g_538e,g_5390,g_5392,g_5394,g_5396,g_5398;
extern int g_5dae,g_5942,g_5944,g_5946,g_5948,g_594a,g_594c;
extern int g_5968,g_596a,g_596c,g_596e,g_5970,g_5972,g_597c,g_597e,g_5980,g_5984,g_599a;
extern int g_5db0,g_5db2,g_6025;
extern unsigned int g_5c3f,g_5c41,g_5d7e,g_5d80,g_5d92;
extern int poll_graphics_keys(void);
extern void cycle_palette(void);
extern unsigned char (*read_pixel)(int,int);
extern void far ega_plot_backend(void);

/* objcmp: {"function":"draw_polygon","address":"066a:02bb","size":797,"symbols":{"_line_step":"066a:000e","_draw_x":"DS:5df0","_draw_y":"DS:5dee","_screen_half_width":"DS:5db0","_screen_half_height":"DS:5db2","_draw_color":"DS:7488","_color_count":"DS:5db6","_draw_symmetry":"DS:5caf","_drawing_delay":"DS:5dc6","_g_4dae":"DS:4dae","_g_4db0":"DS:4db0","_g_4db2":"DS:4db2","_g_4db4":"DS:4db4","_g_4db6":"DS:4db6","_g_4836":"DS:4836","_g_4af2":"DS:4af2","_plot":"DS:5c3f","_cycle_palette":"11c1:2515","_palette_elapsed_ticks":"DS:5d7a","_palette_cycle_interval":"DS:5da6","_read_pixel":"DS:5c3b","_draw_ellipse":"066a:089b","_ellipse_step":"066a:05d8","_rand":"0000:1694","F_LDIV@":"0000:12e7","F_LXMUL@":"0000:11e3","_g_5dae":"DS:5dae","_g_5db0":"DS:5db0","_g_5db2":"DS:5db2","_g_6025":"DS:6025","_g_5942":"DS:5942","_g_5944":"DS:5944","_g_5946":"DS:5946","_g_5948":"DS:5948","_g_594a":"DS:594a","_g_594c":"DS:594c","_g_5968":"DS:5968","_g_596a":"DS:596a","_g_596c":"DS:596c","_g_596e":"DS:596e","_g_5970":"DS:5970","_g_5972":"DS:5972","_g_597c":"DS:597c","_g_597e":"DS:597e","_g_5980":"DS:5980","_g_5984":"DS:5984","_g_599a":"DS:599a","_g_5382":"DS:5382","_g_5384":"DS:5384","_g_5386":"DS:5386","_g_5388":"DS:5388","_g_538a":"DS:538a","_g_538c":"DS:538c","_g_538e":"DS:538e","_g_5390":"DS:5390","_g_5392":"DS:5392","_g_5394":"DS:5394","_g_5396":"DS:5396","_g_5398":"DS:5398","_g_5cbb":"DS:5cbb","_g_5c3f":"DS:5c3f","_g_5c41":"DS:5c41","_g_5d7e":"DS:5d7e","_g_5d80":"DS:5d80","_g_5d92":"DS:5d92","_poll_graphics_keys":"11c1:18e7","_ega_plot_backend":"1899:012d","_g_4db8":"DS:4db8","_g_4dba":"DS:4dba","_g_4dbc":"DS:4dbc","_g_4dbe":"DS:4dbe","_g_4dc0":"DS:4dc0","_g_4dc2":"DS:4dc2","_g_4dc4":"DS:4dc4","_g_4dc6":"DS:4dc6","_g_4dc8":"DS:4dc8","_g_4dca":"DS:4dca","_g_4dcc":"DS:4dcc","_g_4dce":"DS:4dce","_g_4de4":"DS:4de4"}} */
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

/* objcmp: {"function":"ellipse_step","address":"066a:05d8","size":707,"level":"PROVISIONAL","blocker":"algorithm source incomplete; first difference +0x03; object 155, oracle 707"} */
int ellipse_step(int radius_y,int radius_x,int initialize) {
    int xstep,ystep,decision;
    if(initialize) {
        g_4dbc=radius_y; g_4dbe=0;
        if(radius_x) { g_4db8=radius_x; g_4dba=0; }
        else { g_4db8=radius_y*screen_half_width/screen_half_height; g_4dba=0; }
        g_4dc0=g_4db8*g_4db8; g_4dc2=0;
        g_4dc4=g_4dbc*g_4dbc; g_4dc6=0;
        g_4dc8=2*g_4dc0; g_4dca=0; g_4dcc=2*g_4dc4; g_4dce=0;
        draw_y=radius_y; draw_x=0;
        return radius_y==0;
    }
    draw_x++; draw_y--;
    decision=draw_x*draw_x*radius_y*radius_y+draw_y*draw_y*radius_x*radius_x-radius_x*radius_x*radius_y*radius_y;
    return draw_y==0;
}

/* objcmp: {"function":"draw_ellipse","address":"066a:089b","size":778,"level":"PROVISIONAL","blocker":"algorithm source incomplete; first difference +0x03; object 165, oracle 778"} */
void draw_ellipse(int rebuild,int ry,int rx,int cx,int cy,unsigned fill,unsigned edge,unsigned flags) {
    int i,j,done;
    if(rebuild) ellipse_step(ry,rx,1);
    do {
        draw_x=cx+g_4de4[draw_y]; draw_y=cy+draw_y;
        if(flags&2) for(j=cx-g_4de4[draw_y];j<=cx+g_4de4[draw_y];j++) { draw_x=j; draw_color=fill; (*plot)(draw_color); }
        if(flags&1) { draw_color=edge; (*plot)(draw_color); }
        done=ellipse_step(ry,rx,0);
    } while(!done);
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

/* objcmp: {"function":"atom","address":"066a:0d66","size":304,"level":"PROVISIONAL","blocker":"algorithm source incomplete; first difference +0x05; object 138, oracle 304"} */
void atom(int rebuild) {
    int i,turn;
    if(rebuild) { g_5382=0; g_5384=screen_half_width; g_538a=0; }
    for(i=0;i<g_5384;i++) {
        turn=(g_5382+i)&255;
        draw_x=(screen_half_width/2)+(turn-screen_half_width/2);
        draw_y=(screen_half_height/2)+((i&1)?i:-i);
        draw_color=(i&7)+3;
        (*plot)(draw_color);
    }
    g_5382++;
}

/* objcmp: {"function":"balls","address":"066a:0e96","size":416,"level":"PROVISIONAL","blocker":"algorithm source incomplete; first difference +0x03; object 134, oracle 416"} */
void balls(int rebuild) {
    int i,j;
    if(rebuild) { g_5382=0; g_5384=0; g_5386=0; }
    for(i=0;i<4;i++) {
        draw_x=(g_5382+i*7)%screen_half_width;
        draw_y=(g_5384+i*5)%screen_half_height;
        draw_color=(i%color_count);
        for(j=0;j<8;j++) { (*plot)(draw_color); draw_x++; draw_y++; }
    }
    g_5382++; g_5384++;
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

/* objcmp: {"function":"bigstar","address":"066a:11ab","size":1147,"level":"PROVISIONAL","blocker":"algorithm source incomplete; first difference +0x03; object 153, oracle 1147"} */
void bigstar(int rebuild) {
    int i,done;
    if(rebuild) { g_5382=0; g_5384=screen_half_width; g_5386=screen_half_height; }
    for(i=0;i<5;i++) {
        draw_x=(i&1)?g_5384:-g_5384;
        draw_y=(i&2)?g_5386:-g_5386;
        done=line_step(draw_x,draw_y,1);
        while(!done) {
            (*plot)(draw_color);
            done=line_step(draw_x,draw_y,0);
            if(palette_elapsed_ticks>=palette_cycle_interval) cycle_palette();
        }
    }
    g_5382++;
}

/* objcmp: {"function":"boxes","address":"066a:1634","size":419,"level":"PROVISIONAL","blocker":"algorithm source incomplete; first difference +0x03; object 258, oracle 419"} */
void boxes(int rebuild) {
    int i,j,done;
    if(rebuild || !g_594c) { g_5942=random(g_5dae/3); g_5944=random(g_5dae/3); g_5946=1; g_5948=1; g_594a=0; g_594c=0; }
    draw_x=g_5942; draw_y=g_5944;
    for(i=0;i<g_5948;i++) {
        choose_color(0);
        for(j=0;j<4;j++) {
            done=line_step(draw_x+((j==0||j==3)?g_5942:0),draw_y+((j<2)?g_5944:0),1);
            while(!done) { (*plot)(draw_color); done=line_step(draw_x,draw_y,0); }
        }
    }
    g_594a++;
}

/* objcmp: {"function":"cones","address":"066a:17df","size":467,"level":"PROVISIONAL","blocker":"algorithm source incomplete; first difference +0x03; object 377, oracle 467"} */
void cones(int rebuild) {
    int i,step;
    if(rebuild) {
        draw_color=random(color_count);
        g_538c=screen_half_width-random(screen_half_width/2);
        g_538e=screen_half_height-random(screen_half_height/2);
        g_538a=random(3); g_5386=random(screen_half_width/2); g_5388=random(screen_half_height/2);
        g_5382=0; g_5384=0;
    }
    step=random(3)+1;
    for(i=0;i<step;i++) {
        draw_x=g_538c+g_5382; draw_y=g_538e+g_5384;
        (*plot)(draw_color);
        g_5382+=g_5386; g_5384+=g_5388;
    }
}

/* objcmp: {"function":"corkscrew","address":"066a:19b2","size":432,"level":"PROVISIONAL","blocker":"algorithm source incomplete; first difference +0x03; object 89, oracle 432"} */
void corkscrew(int rebuild) {
    int i;
    if(rebuild) { g_5dae=0; g_5db0=1; g_5db2=1; }
    for(i=0;i<g_5db0;i++) {
        draw_x=(g_5dae+i)&(screen_half_width-1);
        draw_y=(g_5dae-i)&(screen_half_height-1);
        (*plot)(draw_color);
    }
    g_5dae++;
}

/* objcmp: {"function":"curves","address":"066a:1b62","size":352,"level":"PROVISIONAL","blocker":"algorithm source incomplete; first difference +0x03; object 121, oracle 352"} */
void curves(int rebuild) {
    int i;
    if(rebuild) { g_5968=0; g_596a=1; g_596c=1; }
    g_5968=(g_5968+1)&15;
    for(i=0;i<g_596e;i++) {
        draw_x=(screen_half_width+i*g_596a)/g_5972;
        draw_y=(screen_half_height+i*g_596c)/g_5972;
        (*plot)(draw_color);
    }
    g_596a+=g_596e; g_596c+=g_5970;
}

/* objcmp: {"function":"cycloids","address":"066a:1cc2","size":1107,"level":"PROVISIONAL","blocker":"algorithm source incomplete; first difference +0x03; object 107, oracle 1107"} */
void cycloids(int rebuild) {
    int i;
    if(rebuild) { g_597c=screen_half_height*3/4; g_597e=screen_half_height/2; g_5980=0; }
    for(i=0;i<g_5984;i++) {
        draw_x=(i*g_597c)/16; draw_y=(i*g_597e)/16;
        (*plot)(draw_color);
    }
    g_5980++; g_599a++;
}

/* objcmp: {"function":"daisies","address":"066a:2115","size":1238,"level":"PROVISIONAL","blocker":"algorithm source incomplete; first difference +0x03; object 187, oracle 1238"} */
void daisies(int rebuild) {
    int i,j,done;
    if(rebuild) { g_5dae=0; g_5db0=0; g_5db2=0; }
    for(i=0;i<8;i++) {
        draw_x=(i-4)*screen_half_width/4;
        draw_y=(i&1)?screen_half_height/3:-screen_half_height/3;
        done=line_step(draw_x,draw_y,1);
        while(!done) {
            (*plot)(draw_color);
            done=line_step(draw_x,draw_y,0);
            if(palette_elapsed_ticks>=palette_cycle_interval) cycle_palette();
        }
    }
    for(j=0;j<3;j++) { draw_color++; if(draw_color>=color_count) draw_color=3; }
}
