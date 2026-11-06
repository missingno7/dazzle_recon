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
static int mesh_spacing;
/* Pixel-centre cuts own ties on the positive side. Use the SAME decision for
 * edge admission and intersections; independently rounded shared vertices can
 * otherwise omit an entire scanline. This is tie resolution, not pen growth. */
static int pixel_cut(double edge) { return (int)ceil(edge-0.5-1e-9); }

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
static void paint_span(int row,int first,int end,uint8_t color)
{
    if (color_rule<0) { dz_canvas_fade_span(row,first,end,color);return; }
    struct dz_native_mapping m=dz_canvas_native_mapping(screen_half_width,screen_half_height);
    int start=first,previous=-1;
    for (int px=first;px<end;++px) {
        int c=field_color((px+0.5-m.cx)/m.unit_x,(row+0.5-m.cy)/m.unit_y);
        if (c==previous) continue;
        if (previous>=0) dz_canvas_fade_span(row,start,px,(uint8_t)previous);
        start=px;previous=c;
    }
    if (previous>=0) dz_canvas_fade_span(row,start,end,(uint8_t)previous);
}
static void polygon(const double *x, const double *y, int n, uint8_t color)
{
    int w,h; dz_canvas_native_size(&w,&h);
    double lo = y[0], hi = y[0];
    for (int i = 1; i < n; ++i) { lo = fmin(lo,y[i]); hi = fmax(hi,y[i]); }
    int first = (int)fmax(0,pixel_cut(lo)), end = (int)fmin(h,pixel_cut(hi));
    for (int row = first; row < end; ++row) {
        double scan = row+0.5, a = HUGE_VAL, b = -HUGE_VAL;
        for (int i = 0; i < n; ++i) {
            int j = (i+1)%n;
            if (row < pixel_cut(fmin(y[i],y[j])) || row >= pixel_cut(fmax(y[i],y[j]))) continue;
            double cut = x[i]+(x[j]-x[i])*(scan-y[i])/(y[j]-y[i]);
            a = fmin(a,cut); b = fmax(b,cut);
        }
        if (a < b) {
            int first=pixel_cut(a),end=pixel_cut(b);
            paint_span(row,first,end,color);
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
    /* Integer fans meet at the cell of their first/last actual sample. A
     * major-axis slice alone leaves triangular gaps at a boundary endpoint.
     * Keep the body weight; finish only the end slice in its original cell. */
    if ((active_key=='k' || active_key=='u' || (active_key=='z' && !color)) &&
        (line.step==1 || line.step==line.count+1)) {
        double ex=line.step==1 ? ax : bx,ey=line.step==1 ? ay : by;
        if (vertical) {
            x[0]=fmin(x[0],ex-width*0.5); x[1]=fmin(x[1],ex-width*0.5);
            x[2]=fmax(x[2],ex+width*0.5); x[3]=fmax(x[3],ex+width*0.5);
        } else {
            y[0]=fmin(y[0],ey-width*0.5); y[1]=fmin(y[1],ey-width*0.5);
            y[2]=fmax(y[2],ey+width*0.5); y[3]=fmax(y[3],ey+width*0.5);
        }
    }
    if (active_key=='i' && (line.step==1 || line.step==line.count+1)) {
        struct dz_native_mapping m=dz_canvas_native_mapping(screen_half_width,screen_half_height);
        double ux=swap ? m.unit_y : m.unit_x,uy=swap ? m.unit_x : m.unit_y;
        double length=dx*dx*ux*ux+dy*dy*uy*uy;
        if (length) for (int i=0;i<4;++i) {
            if ((line.step==1 && (i==0 || i==3)) || (line.step==line.count+1 && (i==1 || i==2))) {
                double t=i==0 || i==3 ? a : b;
                double shift=((x[i]-ax-dx*t)*dx*ux*ux+(y[i]-ay-dy*t)*dy*uy*uy)/length;
                x[i]-=dx*shift;y[i]-=dy*shift;
            }
        }
    }
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
    /* Coarse passes reveal circles circumscribing their cells. The last pass
     * partitions their overhang with exact neighbour cells, not more circles. */
    struct dz_native_mapping m = dz_canvas_native_mapping(screen_half_width,screen_half_height);
    int branches = symmetry&1u ? 4 : 1, swaps = (symmetry&3u) == 3u ? 2 : 1;
    for (int swap = 0; swap < swaps; ++swap) {
        if (!accepted((unsigned)swap)) continue;
        for (int b = 0; b < branches; ++b) {
            int cell_x=screen_half_width+(b>=2 ? -1 : 1)*(swap ? draw_y : draw_x);
            int cell_y=screen_half_height+(b&1 ? -1 : 1)*(swap ? draw_x : draw_y);
            if (video_backend==BACKEND_EGA_640X700) cell_y/=2;
            dz_canvas_fade_cell_owner(cell_x,cell_y);
            double cx = m.cx+(b>=2 ? -1 : 1)*(swap ? draw_y : draw_x)*m.unit_x;
            double cy = m.cy+(b&1 ? -1 : 1)*(swap ? draw_x : draw_y)*m.unit_y;
            SDL_Rect cell;
            int has_cell=dz_canvas_fade_cell_rect(cell_x,cell_y,&cell);
            if (mesh_spacing==1) {
                if (has_cell)
                    for (int y=cell.y;y<cell.y+cell.h;++y) paint_span(y,cell.x,cell.x+cell.w,color);
                continue;
            }
            /* Circumscribe the actual nearest-renderer cell, including its
             * asymmetric fractional edges. Ideal half-weights alone miss EGA
             * corner pixels where the probed nearest map owns a row tie. */
            double rx=has_cell ? fmax(fabs(cell.x-cx),fabs(cell.x+cell.w-cx)) : m.weight_x*0.5;
            double ry=has_cell ? fmax(fabs(cell.y-cy),fabs(cell.y+cell.h-cy)) : m.weight_y*0.5;
            double radius=hypot(rx,ry);
            int w,h; dz_canvas_native_size(&w,&h);
            int first = (int)fmax(0,pixel_cut(cy-radius)), end=(int)fmin(h,pixel_cut(cy+radius));
            for (int y = first; y < end; ++y) {
                double dy=y+0.5-cy, dx=sqrt(fmax(0,radius*radius-dy*dy));
                int a=pixel_cut(cx-dx),end=pixel_cut(cx+dx);
                paint_span(y,a,end,color);
            }
        }
    }
}
void dz_native_fade_mesh(int spacing,int rule)
{
    if (active_key!='m') return;
    mesh_spacing=spacing;
    dz_native_fade_color_rule(rule);
}
void dz_native_fade_mesh_finish(void)
{ if (active_key=='m') dz_canvas_fade_refold(); }
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
static void blister_span(int row,int first,int end,uint8_t color,const int *lo,const int *hi,int count)
{
    /* Competing reflected discs own overlap by continuous original walk time,
     * not by which tiny reveal polygon happened to draw last. */
    while (first<end) {
        int next=end,skip=first;
        for (int i=0;i<count;++i) {
            if (lo[i]<=first && hi[i]>skip) skip=hi[i];
            else if (lo[i]>first && lo[i]<next) next=lo[i];
        }
        if (skip>first) { first=skip; continue; }
        dz_canvas_fade_span(row,first,next,color);first=next;
    }
}
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
        double step=blister.stride;
        /* Missing zero samples have no reveal patch. Let their nonzero
         * neighbours' Voronoi regions run to the actual disc boundary; the
         * analytic disc, not an arbitrary square support, clips the outline.
         * This alone did not fix the inter-reflection shading wedges below. */
        double x[16]={-blister.bx,ball_bitmap_width-blister.bx,ball_bitmap_width-blister.bx,-blister.bx};
        double y[16]={-blister.by,-blister.by,ball_bitmap_height-blister.by,ball_bitmap_height-blister.by};
        int n=4;
        int skip_odd=video_backend==BACKEND_EGA_640X700 && blister.stride==1;
        int sample_x=skip_odd && swap ? 2 : 1,sample_y=skip_odd && !swap ? 2 : 1;
        for (int ny=-1;ny<=1;++ny) for (int nx=-1;nx<=1;++nx) {
            if ((!nx && !ny) || !blister_neighbour(nx*sample_x,ny*sample_y,swap)) continue;
            double dx=nx*sample_x*step,dy=ny*sample_y*step;
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
            int first=(int)fmax(0,pixel_cut(low)),end=(int)fmin(h,pixel_cut(high));
            for (int row=first;row<end;++row) {
                double a=HUGE_VAL,b=-HUGE_VAL;
                for (int i=0;i<n;++i) {
                    int j=(i+1)%n;
                    if (row<pixel_cut(fmin(py[i],py[j])) || row>=pixel_cut(fmax(py[i],py[j]))) continue;
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
                int omit_lo[8],omit_hi[8],omit=0;
                double time_x=swap ? sx/m.unit_x : sx/m.unit_x*column_bytes/step;
                double time_y=swap ? sy/m.unit_y*column_bytes/step : sy/m.unit_y;
                double time_at=at+time_y*(row+0.5-cy)-time_x*cx;
                for (int other=0;other<branches*swaps;++other) {
                    int oswap=other/branches,ob=other%branches;
                    if ((oswap==swap && ob==branch) || !accepted((unsigned)oswap)) continue;
                    int ox=ob>=2 ? -1 : 1,oy=ob&1 ? -1 : 1;
                    double ocx=m.cx+ox*(oswap ? draw_y : draw_x)*m.unit_x;
                    double ocy=m.cy+oy*(oswap ? draw_x : draw_y)*m.unit_y;
                    double tx=oswap ? ox/m.unit_x : ox/m.unit_x*column_bytes/step;
                    double ty=oswap ? oy/m.unit_y*column_bytes/step : oy/m.unit_y;
                    double slope=tx-time_x,constant=at+ty*(row+0.5-ocy)-tx*ocx-time_at;
                    /* A later row of this reveal patch cannot belong to a
                     * reflected field whose walk time is earlier throughout
                     * the physical interval. Reject before solving its circle. */
                    if (fmax(slope*(x0+0.5)+constant,slope*(x1-0.5)+constant)<-1e-9) continue;
                    double ou=source_col+(oswap ? oy*(row+0.5-ocy)/(m.unit_y*step) : 0);
                    double ov=source_row+(oswap ? carry*(ou-source_col) : oy*(row+0.5-ocy)/m.unit_y);
                    double ux=oswap ? 0 : ox/(m.unit_x*step),vx=oswap ? ox/m.unit_x : carry*ux;
                    double qa=ux*ux+vx*vx,qb=(ou-hx)*ux+(ov-hy)*vx;
                    double dist=(ou-hx)*(ou-hx)+(ov-hy)*(ov-hy)-qb*qb/qa;
                    if (dist>=radius*radius) continue;
                    double extent=sqrt((radius*radius-fmax(0,dist))/qa),centre=ocx-qb/qa;
                    double low=centre-extent,high=centre+extent;
                    if (fabs(slope)<1e-12) {
                        if (fabs(constant)<1e-9 ? other<swap*branches+branch : constant<0) continue;
                    } else {
                        double cut=-constant/slope;
                        if (slope>0) low=fmax(low,cut);else high=fmin(high,cut);
                    }
                    if (low<high) { omit_lo[omit]=pixel_cut(low);omit_hi[omit++]=pixel_cut(high); }
                }
                if (blister.flat) { blister_span(row,x0,x1,color,omit_lo,omit_hi,omit); continue; }
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
                    blister_span(row,cuts[i-1],cuts[i],(uint8_t)((c+blister.offset)&color_index_mask),omit_lo,omit_hi,omit);
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
