#include "fades.h"
#include "pens.h"
#include "geometry.h"
#include "dazzle.h"
#include "dzconst.h"
#include "platform/canvas.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

void (*dz_native_fade_plot)(unsigned, unsigned char);
static unsigned active_key;
static const char available[] = "beiklmrtuxz";
static struct { double ax,ay,bx,by; int count,step; } line;
static struct { int x,y,stride,flat,offset,bx,by; } blister;
static int color_rule=-1;

int dz_native_fade_enabled(unsigned key)
{
    return key >= 'a' && key <= 'z' && strchr(available,(int)key) &&
        dz_display.canvas == DZ_CANVAS_SCREEN && (dz_display.native_fades >> (key-'a') & 1u);
}
int dz_native_fades_any(void)
{
    if (dz_display.canvas!=DZ_CANVAS_SCREEN) return 0;
    for (const char *p=available;*p;++p) if (dz_display.native_fades>>(*p-'a')&1u) return 1;
    return 0;
}
int dz_native_fades_select(const char *keys)
{
    uint32_t bits = 0;
    if (!strcmp(keys,"all")) bits = UINT32_MAX;
    else if (strcmp(keys,"none")) for (const char *p = keys; *p; ++p) {
        unsigned c = (unsigned char)*p;
        if (c == ',') continue;
        if (c >= 'A' && c <= 'Z') c += 'a'-'A';
        if (!strchr("abcdefgijklmopqrstuvwxyz",(int)c)) return 0;
        bits |= UINT32_C(1) << (c-'a');
    }
    dz_display.native_fades = bits;
    return 1;
}

static int accepted(unsigned swap)
{
    int y = swap ? draw_x : draw_y;
    return video_backend != BACKEND_EGA_640X700 || !((screen_half_height+y)&1);
}
static int field_color(double x,double y)
{
    double v;
    switch (color_rule) {
    case 2: v=fabs(x); break;
    case 3: v=fabs(y); break;
    case 4: v=fabs(x)+fabs(y); break;
    case 5: v=(x*x+y*y)/32; break;
    case 6: v=(x*x-y*y)/32; break;
    case 7: v=y*y/32-fabs(x); break;
    case 8: v=fabs(x*y-65536*floor((x*y+32768)/65536))/32; break;
    default: return 3;
    }
    int c=(int64_t)floor(v)&color_index_mask;
    return c<3 ? 3 : c;
}
void dz_native_fade_color_rule(int rule) { color_rule=rule>=2 && rule<=8 ? rule : -1; }
static void polygon(const double *x, const double *y, int n, uint8_t color)
{
    int w,h; dz_canvas_native_size(&w,&h);
    double lo = y[0], hi = y[0];
    for (int i = 1; i < n; ++i) { lo = fmin(lo,y[i]); hi = fmax(hi,y[i]); }
    int first = (int)fmax(0,ceil(lo-0.5)), end = (int)fmin(h,ceil(hi-0.5));
    for (int row = first; row < end; ++row) {
        double scan = row+0.5, a = HUGE_VAL, b = -HUGE_VAL;
        for (int i = 0; i < n; ++i) {
            int j = (i+1)%n;
            if (scan < fmin(y[i],y[j]) || scan >= fmax(y[i],y[j])) continue;
            double cut = x[i]+(x[j]-x[i])*(scan-y[i])/(y[j]-y[i]);
            a = fmin(a,cut); b = fmax(b,cut);
        }
        if (a < b) {
            int first=(int)ceil(a-0.5-1e-9),end=(int)ceil(b-0.5-1e-9);
            if (color_rule<0) dz_canvas_fade_span(row,first,end,color);
            else {
                struct dz_native_mapping m=dz_canvas_native_mapping(screen_half_width,screen_half_height);
                int start=first,previous=-1;
                for (int px=first;px<end;++px) {
                    int c=field_color((px+0.5-m.cx)/m.unit_x,(row+0.5-m.cy)/m.unit_y);
                    if (c==previous) continue;
                    if (previous>=0) dz_canvas_fade_span(row,start,px,(uint8_t)previous);
                    start=px; previous=c;
                }
                if (previous>=0) dz_canvas_fade_span(row,start,end,(uint8_t)previous);
            }
        }
    }
}
static void shape_pass(const double *x,const double *y,int n,unsigned symmetry,uint8_t color,int swap,int parity)
{
    if (parity && !accepted((unsigned)swap)) return;
    struct dz_native_mapping m=dz_canvas_native_mapping(screen_half_width,screen_half_height);
    int branches=symmetry&1u ? 4 : 1;
    for (int branch=0;branch<branches;++branch) {
        double px[16]={0},py[16]={0};
        for (int i=0;i<n;++i) {
            px[i]=m.cx+(branch>=2 ? -1 : 1)*(swap ? y[i] : x[i])*m.unit_x;
            py[i]=m.cy+(branch&1 ? -1 : 1)*(swap ? x[i] : y[i])*m.unit_y;
        }
        polygon(px,py,n,color);
    }
}
void dz_native_fade_shape(const double *x,const double *y,int n,unsigned symmetry,uint8_t color)
{
    if (n<3 || n>16) return;
    int swaps=(symmetry&3u)==3u ? 2 : 1;
    for (int swap=0;swap<swaps;++swap) shape_pass(x,y,n,symmetry,color,swap,1);
}
/* Bresenham is a sample of a band one cell wide on its minor axis. Partition
 * that band along its major axis, rather than joining cell centres with caps.
 * Every cut uses the full line's anchor: adjacent slices share the same edge. */
static void band_pass(double ax, double ay, double bx, double by, double a, double b,
                  double width, unsigned symmetry, uint8_t color,int swap)
{
    double dx = bx-ax, dy = by-ay;
    int enhanced=video_backend==BACKEND_EGA_640X700;
    int physical_vertical=fabs(swap ? dx : dy)/(enhanced ? 2 : 1)>fabs(swap ? dy : dx);
    int vertical=enhanced ? (swap ? !physical_vertical : physical_vertical) : fabs(dy)>fabs(dx);
    double nx = vertical ? width*0.5 : 0, ny = vertical ? 0 : width*0.5;
    double x[4] = {ax+dx*a-nx,ax+dx*b-nx,ax+dx*b+nx,ax+dx*a+nx};
    double y[4] = {ay+dy*a-ny,ay+dy*b-ny,ay+dy*b+ny,ay+dy*a+ny};
    shape_pass(x,y,4,symmetry,color,swap,0);
}
void dz_native_fade_polyline(const double *x, const double *y, int n, double width,
                             unsigned symmetry, uint8_t color)
{
    int swaps=(symmetry&3u)==3u ? 2 : 1;
    for (int i=1;i<n;++i) for (int swap=0;swap<swaps;++swap)
        band_pass(x[i-1],y[i-1],x[i],y[i],0,1,width,symmetry,color,swap);
}
static void line_plot(unsigned symmetry,unsigned char color)
{
    int step=line.step++,swaps=(symmetry&3u)==3u ? 2 : 1;
    int enhanced=video_backend==BACKEND_EGA_640X700;
    double dx=line.bx-line.ax,dy=line.by-line.ay;
    for (int swap=0;swap<swaps;++swap) {
        /* Reconstruct the continuous line between software samples. The EGA
         * receipt still rejects odd Y; its actual pen is one physical row.
         * Weight belongs to physical Y after exchange, like image line pens. */
        if (line.count) {
            int physical_vertical=fabs(swap ? dx : dy)/(enhanced ? 2 : 1)>fabs(swap ? dy : dx);
            double half=0.5;
            double width=enhanced && !physical_vertical ? 2 : 1;
            double a=(step-half)/line.count,b=(step+half)/line.count;
            if (enhanced) {
                double physical_count=fmax(fabs(swap ? dy : dx),fabs(swap ? dx : dy)/2);
                if (!step) a=-0.5/physical_count;
                if (step==line.count) b=1+0.5/physical_count;
            }
            band_pass(line.ax,line.ay,line.bx,line.by,a,b,width,symmetry,color,swap);
        } else {
            double hx=enhanced && swap ? 1 : 0.5,hy=enhanced && !swap ? 1 : 0.5;
            const double x[]={line.ax-hx,line.ax+hx,line.ax+hx,line.ax-hx};
            const double y[]={line.ay-hy,line.ay-hy,line.ay+hy,line.ay+hy};
            shape_pass(x,y,4,symmetry,color,swap,0);
        }
    }
}
void dz_native_fade_line(double ax, double ay, double bx, double by)
{
    if (!active_key) return;
    color_rule=-1;
    line.ax=ax; line.ay=ay; line.bx=bx; line.by=by; line.step=0;
    line.count = (int)fmax(fabs(bx-ax),fabs(by-ay));
    dz_native_fade_plot = line_plot;
}
void dz_native_fade_real_line(double ax, double ay, double bx, double by)
{
    if (!active_key) return;
    line.ax=dz_native_untruncated(ax); line.ay=dz_native_untruncated(ay);
    line.bx=dz_native_untruncated(bx); line.by=dz_native_untruncated(by);
}
void dz_native_fade_fixed_line(double ax,double ay,double bx,double by)
{
    if (!active_key) return;
    /* The high word of signed 16.16 floors, including negatives. Its sample
     * owns [k,k+1), unlike the sign-dependent C truncation used by Twist. */
    line.ax=ax-0.5; line.ay=ay-0.5; line.bx=bx-0.5; line.by=by-0.5;
}
static void dot_plot(unsigned symmetry, unsigned char color)
{
    /* Equal area to the original cell; circular on the physical output. */
    struct dz_native_mapping m = dz_canvas_native_mapping(screen_half_width,screen_half_height);
    int branches = symmetry&1u ? 4 : 1, swaps = (symmetry&3u) == 3u ? 2 : 1;
    double radius = sqrt(m.weight_x*m.weight_y/3.14159265358979323846);
    for (int swap = 0; swap < swaps; ++swap) {
        if (!accepted((unsigned)swap)) continue;
        for (int b = 0; b < branches; ++b) {
            double cx = m.cx+(b>=2 ? -1 : 1)*(swap ? draw_y : draw_x)*m.unit_x;
            double cy = m.cy+(b&1 ? -1 : 1)*(swap ? draw_x : draw_y)*m.unit_y;
            int w,h; dz_canvas_native_size(&w,&h);
            int first = (int)fmax(0,ceil(cy-radius-0.5)), end=(int)fmin(h,ceil(cy+radius-0.5));
            for (int y = first; y < end; ++y) {
                double dy=y+0.5-cy, dx=sqrt(fmax(0,radius*radius-dy*dy));
                dz_canvas_fade_span(y,(int)ceil(cx-dx-0.5),(int)ceil(cx+dx-0.5),color);
            }
        }
    }
}
void dz_native_fade_hexagon(void)
{
    if (active_key != 'x') return;
    if (draw_y <= hexagon_vertical_limit) {
        dz_native_fade_line(draw_x,0,draw_x,hexagon_vertical_limit);
        line.step = draw_y;
    } else {
        dz_native_fade_line(hexagon_start_x,hexagon_vertical_limit,-1,
                            hexagon_vertical_limit+hexagon_start_x+1);
        line.step = draw_y-hexagon_vertical_limit;
    }
}
static int blister_neighbour(int dx,int dy,int swap)
{
    int x=blister.bx+dx*blister.stride,y=blister.by+dy*blister.stride;
    if (x<0 || y<0 || x>=ball_bitmap_width || y>=ball_bitmap_height) return 0;
    int row=swap ? blister.x+x : blister.y+y;
    if (video_backend==BACKEND_EGA_640X700 && ((screen_half_height+row)&1)) return 0;
    int bytes=(ball_bitmap_height+blister.stride-1)/blister.stride*blister.stride;
    int at=x/blister.stride*bytes+y;
    return at<ball_bitmap_width*ball_bitmap_height && ball_bitmap_pixels[at]!=0;
}
/* Clip one reveal patch to the nearest visited samples. The outer disc and
 * distance rings are then solved per scanline; no bitmap fetch per output pixel. */
static int clip_patch(double *x,double *y,int n,double dx,double dy,double cut)
{
    double xx[16],yy[16]; int count=0;
    for (int i=0;i<n;++i) {
        int j=(i+1)%n;
        double a=x[i]*dx+y[i]*dy-cut,b=x[j]*dx+y[j]*dy-cut;
        if (a<=0) { xx[count]=x[i]; yy[count++]=y[i]; }
        if ((a<0 && b>0) || (a>0 && b<0)) {
            double t=a/(a-b);
            xx[count]=x[i]+t*(x[j]-x[i]); yy[count++]=y[i]+t*(y[j]-y[i]);
        }
    }
    memcpy(x,xx,(size_t)count*sizeof *x); memcpy(y,yy,(size_t)count*sizeof *y);
    return count;
}
static int integer_order(const void *a,const void *b)
{ int x=*(const int *)a,y=*(const int *)b; return (x>y)-(x<y); }
static void blister_plot(unsigned symmetry,unsigned char color)
{
    struct dz_native_mapping m=dz_canvas_native_mapping(screen_half_width,screen_half_height);
    int w,h; dz_canvas_native_size(&w,&h);
    int branches=symmetry&1u ? 4 : 1,swaps=(symmetry&3u)==3u ? 2 : 1;
    int column_bytes=(ball_bitmap_height+blister.stride-1)/blister.stride*blister.stride;
    int at=blister.bx/blister.stride*column_bytes+blister.by;
    int source_col=at/ball_bitmap_height,source_row=at%ball_bitmap_height;
    double hx=ball_bitmap_width/2,hy=ball_bitmap_height/2;
    double radius=fmin(hx,hy)-1, shade_x=hx+(int)hx/4,shade_y=hy+(int)hy/4;
    for (int swap=0;swap<swaps;++swap) {
        if (!accepted((unsigned)swap)) continue;
        double step=blister.stride,x[16]={-step,step,step,-step},y[16]={-step,-step,step,step};
        int n=4;
        for (int ny=-1;ny<=1;++ny) for (int nx=-1;nx<=1;++nx) {
            if ((!nx && !ny) || !blister_neighbour(nx,ny,swap)) continue;
            double dx=nx*step,dy=ny*step;
            n=clip_patch(x,y,n,dx,dy,(dx*dx+dy*dy)*0.5);
        }
        for (int branch=0;branch<branches;++branch) {
            int sx=branch>=2 ? -1 : 1,sy=branch&1 ? -1 : 1;
            double cx=m.cx+sx*(swap ? draw_y : draw_x)*m.unit_x;
            double cy=m.cy+sy*(swap ? draw_x : draw_y)*m.unit_y;
            double px[16],py[16],low=HUGE_VAL,high=-HUGE_VAL;
            for (int i=0;i<n;++i) {
                px[i]=cx+sx*(swap ? y[i] : x[i])*m.unit_x;
                py[i]=cy+sy*(swap ? x[i] : y[i])*m.unit_y;
                low=fmin(low,py[i]); high=fmax(high,py[i]);
            }
            int first=(int)fmax(0,ceil(low-0.5)),end=(int)fmin(h,ceil(high-0.5));
            for (int row=first;row<end;++row) {
                double a=HUGE_VAL,b=-HUGE_VAL;
                for (int i=0;i<n;++i) {
                    int j=(i+1)%n;
                    if (row+0.5<fmin(py[i],py[j]) || row+0.5>=fmax(py[i],py[j])) continue;
                    double v=px[i]+(px[j]-px[i])*(row+0.5-py[i])/(py[j]-py[i]);
                    a=fmin(a,v); b=fmax(b,v);
                }
                /* Odd bitmap heights carry a row at every stride-two column.
                 * This is an affine shear between samples, not column steps. */
                double carry=column_bytes-ball_bitmap_height;
                double cu=source_col+(swap ? sy*(row+0.5-cy)/(m.unit_y*step) : 0);
                double cv=source_row+(swap ? carry*(cu-source_col) : sy*(row+0.5-cy)/m.unit_y);
                double du=swap ? 0 : sx/(m.unit_x*step);
                double dv=swap ? sx/m.unit_x : carry*du;
                double aa=du*du+dv*dv;
                double bb=(cu-hx)*du+(cv-hy)*dv;
                double cc=(cu-hx)*(cu-hx)+(cv-hy)*(cv-hy);
                double centre=cx-bb/aa,distance=fmax(0,cc-bb*bb/aa);
                if (radius*radius<=distance || a>=b) continue;
                double extent=sqrt((radius*radius-distance)/aa);
                a=fmax(a,centre-extent); b=fmin(b,centre+extent);
                int x0=(int)fmax(0,ceil(a-0.5-1e-9)),x1=(int)fmin(w,ceil(b-0.5-1e-9));
                if (x0>=x1) continue;
                if (blister.flat) { dz_canvas_fade_span(row,x0,x1,color); continue; }
                bb=(cu-shade_x)*du+(cv-shade_y)*dv;
                cc=(cu-shade_x)*(cu-shade_x)+(cv-shade_y)*(cv-shade_y);
                double shade=cx-bb/aa,d=fmax(0,cc-bb*bb/aa);
                double va=a-shade,vb=b-shade;
                double closest=va*vb<=0 ? 0 : fmin(fabs(va),fabs(vb));
                int r0=(int)floor(sqrt(d+aa*closest*closest))+1;
                int r1=(int)ceil(sqrt(d+aa*fmax(va*va,vb*vb)));
                int cuts[2*(r1-r0+1)+2],count=0;
                cuts[count++]=x0; cuts[count++]=x1;
                for (int ring=r0;ring<=r1;++ring) {
                    double v=sqrt(fmax(0,((double)ring*ring-d)/aa));
                    for (int sign=-1;sign<=1;sign+=2) {
                        int cut=(int)ceil(shade+sign*v-0.5-1e-9);
                        if (cut>x0 && cut<x1) cuts[count++]=cut;
                    }
                }
                qsort(cuts,(size_t)count,sizeof *cuts,integer_order);
                for (int i=1;i<count;++i) if (cuts[i]>cuts[i-1]) {
                    double t=(cuts[i-1]+cuts[i])*0.5-cx;
                    int c=dz_native_ball_field(cu+du*t,cv+dv*t);
                    dz_canvas_fade_span(row,cuts[i-1],cuts[i],(uint8_t)((c+blister.offset)&color_index_mask));
                }
            }
        }
    }
}
void dz_native_fade_blister(int x,int y,int stride,int flat,int offset,int bx,int by)
{
    if (active_key != 'b') return;
    blister.x=x; blister.y=y; blister.stride=stride; blister.flat=flat;
    blister.offset=offset; blister.bx=bx; blister.by=by;
    dz_native_fade_plot=blister_plot;
}
void dz_native_fade_begin(unsigned key)
{
    color_rule=-1;
    active_key=dz_native_fade_enabled(key) ? key : 0;
    dz_native_fade_plot=active_key=='m' ? dot_plot : NULL;
}
void dz_native_fade_end(void) { active_key=0; dz_native_fade_plot=NULL; }
