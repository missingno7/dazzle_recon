#include <stdlib.h>
#include <dos.h>
#include <dos.h>

extern int draw_x;
extern int draw_y;
extern int screen_half_width;
extern int screen_half_height;
extern unsigned color_count;
extern unsigned plot_color;
extern unsigned video_adapter;
extern unsigned palette_elapsed_ticks;
extern unsigned palette_cycle_ticks;
extern unsigned draw_symmetry;
extern void (far *plot_callback)(unsigned);
extern void far ega_plot_symmetric_640x700(unsigned);
extern void draw_polygon(int, int, int far *, int far *, unsigned, unsigned, unsigned);
extern void draw_ellipse(int, int, int, int, int, unsigned, unsigned, unsigned);
extern void cycle_palette(void);

extern int butterflies_wing_x1;
extern int butterflies_wing_x2;
extern int butterflies_wing_x3;
extern int butterflies_wing_x4;
extern int butterflies_wing_y1;
extern int butterflies_wing_y2;
extern int butterflies_wing_y3;
extern int butterflies_wing_y4;

extern float yarnball_aspect_scale;
extern int image_work_count;
extern int tiles_direction_x[4];
extern int tiles_direction_y[4];
extern int tiles_side_lengths[2];

#define EGA_PLOT_CALLBACK (FP_SEG(plot_callback) == FP_SEG(ega_plot_symmetric_640x700) && FP_OFF(plot_callback) == FP_OFF(ega_plot_symmetric_640x700))

extern float yarnball_sin30;
extern float yarnball_cos30;
extern float yarnball_aspect_ratio;
extern int yarnball_radius_step;
extern int yarnball_sparse_shift;
extern int yarnball_full_ring;
extern int yarnball_draw_flags;
extern int yarnball_center_y;
extern int yarnball_center_x;
extern int yarnball_radius_y;
extern int yarnball_radius_x;
extern int yarnball_x_sin30;
extern int yarnball_x_cos30;
extern int yarnball_radius_sin30;
extern int yarnball_radius_cos30;
extern int wedges_step_y[3];
extern int wedges_step_x[3];
extern int wedges_vertex_y[3];
extern int wedges_vertex_x[3];
extern int tiles_y_extent;
extern int tiles_fill_color_mode;
extern int tiles_saved_y;
extern int tiles_saved_x;
extern int tiles_base_fill_color;
extern int tiles_fill_rows_left;
extern int tiles_edge_step_y;
extern int tiles_edge_step_x;
extern int tiles_move_y;
extern int tiles_move_x;
extern int tiles_edge_index;
extern int tiles_edge_progress;
extern int tiles_side_length;
extern int gumballs_shading_choice;
extern int gumballs_draw_flags;
extern int gumballs_edge_color;
extern int gumballs_step_y;
extern int gumballs_step_x;
extern int gumballs_radius_step;
extern int gumballs_center_y;
extern int gumballs_center_x;
extern int gumballs_radius_y;
extern int diamonds_height_limit;
extern int diamonds_width_limit;
extern int diamonds_origin_limit_y;
extern int diamonds_origin_limit_x;
extern int diamonds_height_step;
extern int diamonds_width_step;
extern int diamonds_step_y;
extern int diamonds_step_x;
extern int diamonds_height;
extern int diamonds_width;
extern int diamonds_closing_y;
extern int diamonds_vertex3_y;
extern int diamonds_vertex2_y;
extern int diamonds_vertex1_y;
extern int diamonds_origin_y;
extern int diamonds_closing_x;
extern int diamonds_vertex3_x;
extern int diamonds_vertex2_x;
extern int diamonds_vertex1_x;
extern int diamonds_origin_x;
extern int butterflies_restart;
extern int butterflies_tip_offset;
extern int butterflies_height_limit;
extern int butterflies_draw_flags;
extern int butterflies_edge_color;
extern int butterflies_fill_color;
extern int butterflies_center_y;
extern int butterflies_center_x;
extern int butterflies_wing_y[6];
extern int butterflies_mirrored_x[6];
extern int butterflies_wing_x[6];
extern int butterflies_vertex_index;

/* Scalar views of the closed polygon vertex arrays. */
#define butterflies_wing_x1 (*(int near *)((unsigned)butterflies_wing_x+2))
#define butterflies_wing_x2 (*(int near *)((unsigned)butterflies_wing_x+4))
#define butterflies_wing_x3 (*(int near *)((unsigned)butterflies_wing_x+6))
#define butterflies_wing_x4 (*(int near *)((unsigned)butterflies_wing_x+8))
#define butterflies_wing_y1 (*(int near *)((unsigned)butterflies_wing_y+2))
#define butterflies_wing_y2 (*(int near *)((unsigned)butterflies_wing_y+4))
#define butterflies_wing_y3 (*(int near *)((unsigned)butterflies_wing_y+6))
#define butterflies_wing_y4 (*(int near *)((unsigned)butterflies_wing_y+8))

/* 0B84:0000, 3 bytes */
unsigned dzl2_seg(void)
{
    return (unsigned)_CS;
}

/* 0B84:0003, 748 bytes */
void butterflies(int initialize)
{
    int i;

    if (initialize || butterflies_restart) {
        butterflies_center_x = random(screen_half_width * 3 / 4) + screen_half_width / 8;
        butterflies_center_y = random(screen_half_height * 3 / 4) + screen_half_height / 8;
        butterflies_wing_x1 = random(screen_half_width / 8) + screen_half_width / 16;
        butterflies_wing_y1 = random(screen_half_height / 16);
        butterflies_height_limit = random(screen_half_height / 2) + screen_half_height / 8;
        butterflies_tip_offset = random(screen_half_width / 4) + 8;
        butterflies_edge_color = random(color_count);
        butterflies_draw_flags = random(3) + 1;
        if (video_adapter == 8)
            butterflies_draw_flags += 8;
        if (random(2))
            butterflies_draw_flags += 0x10;
        butterflies_mirrored_x[0] = butterflies_center_x;
        butterflies_wing_x[0] = butterflies_center_x;
        butterflies_wing_y[0] = butterflies_center_y;
        butterflies_restart = 0;
    } else {
        butterflies_wing_x1 += 4;
        butterflies_wing_y1 += 16;
        butterflies_wing_x2 = butterflies_wing_x1 + butterflies_tip_offset;
        butterflies_wing_y2 = butterflies_wing_y1;
        butterflies_wing_x3 = -butterflies_wing_x2 / 2;
        butterflies_wing_y3 = -butterflies_wing_y2 / 2;
        butterflies_wing_x4 = butterflies_wing_x3;
        butterflies_wing_y4 = butterflies_wing_x3 * butterflies_wing_y1 / butterflies_wing_x1;
        for (butterflies_vertex_index = 1; butterflies_vertex_index <= 4; ++butterflies_vertex_index)
            butterflies_mirrored_x[butterflies_vertex_index] = -butterflies_wing_x[butterflies_vertex_index];
        if (butterflies_wing_y1 > butterflies_height_limit || butterflies_wing_y1 - butterflies_wing_y4 >= screen_half_height - 1) {
            butterflies_restart = 1;
            return;
        } else {
            butterflies_fill_color = random(color_count);
            draw_polygon(1, 5, butterflies_wing_x, butterflies_wing_y, butterflies_fill_color, butterflies_edge_color, butterflies_draw_flags);
            draw_polygon(1, 5, butterflies_mirrored_x, butterflies_wing_y, butterflies_fill_color, butterflies_edge_color, butterflies_draw_flags);
        }
    }
    draw_x = butterflies_center_x;
    draw_y = butterflies_center_y;
}

/* 0B84:02EF, 1079 bytes */
void diamonds(int initialize)
{
    int i;
    int fill_color;
    int flags;
    int selector;

    if (initialize) {
        i = 0;
        diamonds_width_limit = screen_half_width / 2;
        diamonds_height_limit = screen_half_height / 2;
        diamonds_origin_limit_x = screen_half_width - screen_half_width / 4;
        diamonds_origin_limit_y = screen_half_height - screen_half_height / 4;
        diamonds_origin_x = random(diamonds_origin_limit_x);
        diamonds_origin_y = random(diamonds_origin_limit_y);
        diamonds_width = random(diamonds_width_limit - 8) + 8;
        diamonds_height = random(diamonds_height_limit - 8) + 8;
    }

    if (i <= 0) {
        (void)random(4);
        if (random(2))
            diamonds_step_x = random(screen_half_width / 16) + 4;
        else
            diamonds_step_x = -(random(screen_half_width / 16) + 4);
        if (random(2))
            diamonds_step_y = random(screen_half_height / 16) + 4;
        else
            diamonds_step_y = -(random(screen_half_height / 16) + 4);
        diamonds_width_step = random(screen_half_width / 4) - screen_half_width / 8;
        diamonds_height_step = random(screen_half_width / 4) - screen_half_width / 8;
    }

    diamonds_origin_x += diamonds_step_x;
    diamonds_origin_y += diamonds_step_y;
    diamonds_width += diamonds_width_step;
    diamonds_height += diamonds_height_step;
    if (diamonds_origin_x < 0 || diamonds_origin_x > diamonds_origin_limit_x) {
        diamonds_step_x = -diamonds_step_x;
        diamonds_origin_x += diamonds_step_x;
    }
    if (diamonds_origin_y < 0 || diamonds_origin_y > diamonds_origin_limit_y) {
        diamonds_step_y = -diamonds_step_y;
        diamonds_origin_y += diamonds_step_y;
    }
    if (diamonds_width < 8 || diamonds_width > diamonds_width_limit) {
        diamonds_width_step = -diamonds_width_step;
        diamonds_width += diamonds_width_step;
    }
    if (diamonds_height < 8 || diamonds_height > diamonds_height_limit) {
        diamonds_height_step = -diamonds_height_step;
        diamonds_height += diamonds_height_step;
    }

    diamonds_vertex1_x = diamonds_width >> 1;
    diamonds_vertex1_y = diamonds_height >> 1;
    diamonds_vertex2_x = diamonds_width;
    diamonds_vertex2_y = 0;
    diamonds_vertex3_x = diamonds_vertex1_x;
    diamonds_vertex3_y = -diamonds_vertex1_y;

    fill_color = random(color_count);
    if (++plot_color >= color_count)
        plot_color = 3;
    flags = 0;
    if (random(2))
        flags |= 3;
    else {
        flags |= 1;
        if (video_adapter == 8 && random(2))
            flags |= 0x10;
    }
    if (video_adapter == 8) {
        selector = random(3);
        switch (selector) {
        case 0: plot_color = random(color_count); break;
        case 1: flags |= 4; break;
        case 2: flags |= 8; break;
        }
    }
    draw_polygon(1, 4, &diamonds_origin_x, &diamonds_origin_y, plot_color, fill_color, flags);
}

/* 0B84:0726, 764 bytes */
void gumballs(int initialize)
{
    if (initialize) {
        gumballs_radius_y = random(screen_half_height / 4 - 8) + 8;
        gumballs_center_x = screen_half_width / 4 + random(screen_half_width / 2);
        gumballs_center_y = screen_half_height / 4 + random(screen_half_height / 2);
    }
    gumballs_radius_step = random(screen_half_height / 4) - screen_half_height / 8;
    gumballs_step_x = random(screen_half_width / 4) - screen_half_width / 8;
    gumballs_step_y = random(screen_half_height / 4) - screen_half_height / 8;
    gumballs_radius_y += gumballs_radius_step;
    gumballs_center_x += gumballs_step_x;
    gumballs_center_y += gumballs_step_y;
    if (screen_half_width / 8 > gumballs_center_x || gumballs_center_x > screen_half_width) {
        gumballs_step_x = -gumballs_step_x;
        gumballs_center_x += gumballs_step_x;
    }
    if (screen_half_height / 8 > gumballs_center_y || gumballs_center_y > screen_half_height) {
        gumballs_step_y = -gumballs_step_y;
        gumballs_center_y += gumballs_step_y;
    }
    if (gumballs_radius_y < 8 || gumballs_radius_y > screen_half_height / 4) {
        gumballs_radius_step = -gumballs_radius_step;
        gumballs_radius_y += gumballs_radius_step;
    }
    gumballs_edge_color = random(color_count);
    if (++plot_color >= color_count) plot_color = 3;
    gumballs_draw_flags = 0;
    if (random(2))
        gumballs_draw_flags |= 2;
    else {
        gumballs_draw_flags |= 1;
        if (video_adapter == 8 && random(2))
            gumballs_draw_flags |= 0x20;
    }
    if (video_adapter == 8) {
        gumballs_shading_choice = random(4);
        switch (gumballs_shading_choice) {
        case 0: plot_color = random(color_count); break;
        case 1: gumballs_draw_flags |= 4; break;
        case 2: gumballs_draw_flags |= 8; break;
        case 3: gumballs_draw_flags |= 0x10; break;
        }
    }
    draw_ellipse(1, gumballs_radius_y, 0, gumballs_center_x, gumballs_center_y, plot_color, gumballs_edge_color, gumballs_draw_flags);
}

/* 0B84:0A22, 1017 bytes */
void tiles(int initialize)
{
    if (EGA_PLOT_CALLBACK)
        draw_y >>= 1;

    if (initialize) {
        if (EGA_PLOT_CALLBACK)
            tiles_y_extent = screen_half_height / 2;
        else
            tiles_y_extent = screen_half_height;
        tiles_base_fill_color = plot_color;
        draw_x = screen_half_width / 4 + random(screen_half_width / 2);
        draw_y = tiles_y_extent / 4 + random(tiles_y_extent / 2);
    }

    tiles_move_x = random(screen_half_width / 4) - screen_half_width / 8;
    tiles_move_y = random(tiles_y_extent / 4) - tiles_y_extent / 8;
    tiles_side_lengths[0] = random(screen_half_width / 2);
    tiles_side_lengths[1] = random(tiles_y_extent / 2);
    draw_x += tiles_move_x;
    draw_y += tiles_move_y;
    if (screen_half_width / 4 > draw_x || draw_x >= screen_half_width) {
        tiles_move_x = -tiles_move_x;
        draw_x += tiles_move_x;
    }
    if (tiles_y_extent / 4 > draw_y || draw_y >= tiles_y_extent) {
        tiles_move_y = -tiles_move_y;
        draw_y += tiles_move_y;
    }

    if (random(2)) {
        if (video_adapter == 8)
            tiles_fill_color_mode = random(2) + 1;
        else
            tiles_fill_color_mode = 0;
        if (tiles_fill_color_mode == 2)
            plot_color = tiles_base_fill_color;
        tiles_saved_x = draw_x;
        tiles_saved_y = draw_y;
        tiles_fill_rows_left = tiles_side_lengths[1];
        if (EGA_PLOT_CALLBACK) {
            draw_y <<= 1;
            tiles_fill_rows_left <<= 1;
        }

        while (tiles_fill_rows_left >= 0) {
            if (palette_elapsed_ticks >= palette_cycle_ticks)
                cycle_palette();
            if (tiles_fill_color_mode == 1)
                plot_color = tiles_base_fill_color;
            tiles_side_length = tiles_side_lengths[0];
            while (tiles_side_length >= 0) {
                plot_callback(draw_symmetry);
                --draw_x;
                --tiles_side_length;
                if (tiles_fill_color_mode == 1 && ++plot_color >= color_count)
                    plot_color = 3;
                ++image_work_count;
            }
            draw_x += tiles_side_lengths[0] + 1;
            --draw_y;
            --tiles_fill_rows_left;
            if (tiles_fill_color_mode == 2 && ++plot_color >= color_count)
                plot_color = 3;
            if (EGA_PLOT_CALLBACK) {
                --draw_y;
                --tiles_fill_rows_left;
            }
        }
        draw_x = tiles_saved_x;
        draw_y = tiles_saved_y;
        if (tiles_fill_color_mode == 0 && ++plot_color >= color_count)
            plot_color = 3;
        tiles_base_fill_color = plot_color;
    }

    if (EGA_PLOT_CALLBACK) {
        draw_y <<= 1;
        tiles_side_lengths[1] <<= 1;
    }
    plot_color = random(color_count);
    tiles_edge_index = -1;
    while (++tiles_edge_index <= 3) {
        tiles_side_length = tiles_side_lengths[tiles_edge_index & 1];
        tiles_edge_step_x = tiles_direction_x[tiles_edge_index];
        tiles_edge_step_y = tiles_direction_y[tiles_edge_index];
        tiles_edge_progress = 0;
        while (tiles_edge_progress++ < tiles_side_length) {
            plot_callback(draw_symmetry);
            draw_x += tiles_edge_step_x;
            draw_y += tiles_edge_step_y;
            if (tiles_edge_step_y != 0 && EGA_PLOT_CALLBACK) {
                draw_y += tiles_edge_step_y;
                ++tiles_edge_progress;
            }
            ++image_work_count;
        }
    }
}

/* 0B84:0E1B, 804 bytes */
void wedges(int initialize)
{
    int i, px[4], py[4], first, fill_color, selector, flags;

    if (initialize) {
        first = 0;
        for (i = 0; i < 3; ++i) {
            wedges_vertex_x[i] = screen_half_width / 4 + random(screen_half_width / 2);
            wedges_vertex_y[i] = screen_half_height / 4 + random(screen_half_height / 2);
        }
    }
    if (first <= 0) {
        (void)random(8);
        for (i = 0; i < 3; ++i) {
            wedges_step_x[i] = random(screen_half_width / 4) - screen_half_width / 8;
            wedges_step_y[i] = random(screen_half_height / 4) - screen_half_height / 8;
        }
    }

    for (i = 0; i < 3; ++i) {
        wedges_vertex_x[i] += wedges_step_x[i];
        wedges_vertex_y[i] += wedges_step_y[i];
        if (wedges_vertex_x[i] < 0 || wedges_vertex_x[i] >= screen_half_width) {
            wedges_step_x[i] = -wedges_step_x[i];
            wedges_vertex_x[i] += wedges_step_x[i];
        }
        if (wedges_vertex_y[i] < 0 || wedges_vertex_y[i] >= screen_half_height) {
            wedges_step_y[i] = -wedges_step_y[i];
            wedges_vertex_y[i] += wedges_step_y[i];
        }
    }

    px[0] = wedges_vertex_x[0];
    py[0] = wedges_vertex_y[0];
    px[1] = wedges_vertex_x[1] - wedges_vertex_x[0];
    py[1] = wedges_vertex_y[1] - wedges_vertex_y[0];
    px[2] = wedges_vertex_x[2] - wedges_vertex_x[0];
    py[2] = wedges_vertex_y[2] - wedges_vertex_y[0];

    fill_color = random(color_count);
    if (++plot_color >= color_count)
        plot_color = 3;
    flags = 0;
    if (random(2))
        flags |= 3;
    else {
        flags |= 1;
        if (video_adapter == 8 && random(2))
            flags |= 0x10;
    }
    if (video_adapter == 8) {
        selector = random(3);
        switch (selector) {
        case 0: plot_color = random(color_count); break;
        case 1: flags |= 4; break;
        case 2: flags |= 8; break;
        }
    }
    draw_polygon(1, 3, px, py, plot_color, fill_color, flags);
}

/* 0B84:113F, 859 bytes */
void yarnball(int initialize)
{
    register int draw_x, draw_y;
    int i;

    if (initialize) {
        yarnball_aspect_ratio = (float)screen_half_width / (float)screen_half_height * yarnball_aspect_scale;
        yarnball_cos30 = 0.866f;
        yarnball_sin30 = 0.5f;
        yarnball_draw_flags = 1;
        yarnball_radius_step = 1;
        yarnball_full_ring = random(3);
        if (yarnball_full_ring != 0) {
            yarnball_radius_y = screen_half_height / 8 + random(screen_half_height / 8);
            yarnball_center_y = yarnball_radius_y * 2 + random(screen_half_height / 4);
            yarnball_center_x = (int)((float)random(screen_half_width / 4) + ((float)(yarnball_radius_y * 2) * yarnball_aspect_ratio));
        } else {
            yarnball_sparse_shift = 1;
            yarnball_radius_y = screen_half_height / 4 + random(screen_half_height * 3 / 4);
            yarnball_center_x = 0;
            yarnball_center_y = 0;
            yarnball_draw_flags |= 0x20;
        }
        return;
    }

    draw_ellipse(1, yarnball_radius_y, 0, yarnball_center_x, yarnball_center_y, 0, plot_color, yarnball_draw_flags);

    yarnball_radius_cos30 = (int)((float)yarnball_radius_y * yarnball_cos30);
    yarnball_radius_sin30 = (int)((float)yarnball_radius_y * yarnball_sin30);
    yarnball_x_cos30 = (int)((float)yarnball_radius_cos30 * yarnball_aspect_ratio);
    yarnball_x_sin30 = (int)((float)yarnball_radius_sin30 * yarnball_aspect_ratio);
    yarnball_radius_x = (int)((float)yarnball_radius_y * yarnball_aspect_ratio);

    i = 0;
    do {
        switch (i) {
        case 0: draw_x = yarnball_center_x + yarnball_radius_x; draw_y = yarnball_center_y; break;
        case 1: draw_x = yarnball_center_x - yarnball_radius_x; break;
        case 2: draw_x = yarnball_center_x; draw_y = yarnball_center_y + yarnball_radius_y; break;
        case 3: draw_y = yarnball_center_y - yarnball_radius_y; break;
        case 4: draw_x = yarnball_center_x + yarnball_x_cos30; draw_y = yarnball_center_y + yarnball_radius_sin30; break;
        case 5: draw_x = yarnball_center_x - yarnball_x_cos30; break;
        case 6: draw_y = yarnball_center_y - yarnball_radius_sin30; break;
        case 7: draw_x = yarnball_center_x + yarnball_x_cos30; break;
        case 8: draw_x = yarnball_center_x + yarnball_x_sin30; draw_y = yarnball_center_y + yarnball_radius_cos30; break;
        case 9: draw_x = yarnball_center_x - yarnball_x_sin30; break;
        case 10: draw_y = yarnball_center_y - yarnball_radius_cos30; break;
        case 11: draw_x = yarnball_center_x + yarnball_x_sin30; break;
        }
        draw_ellipse(0, yarnball_radius_y, 0, draw_x, draw_y, 0, plot_color, yarnball_draw_flags);
        if (yarnball_full_ring != 0)
            ++i;
        else {
            i = 2 << yarnball_sparse_shift++;
        }
    } while (i < 12);

    yarnball_radius_y += yarnball_radius_step;
    if (yarnball_full_ring != 0)
        yarnball_draw_flags ^= 0x20;
    if (++plot_color >= color_count)
        plot_color = 3;
    yarnball_sparse_shift = 1;
}

/* Uninitialized module state. BC++ 3.0 emits BSS in reverse declaration order.
 * Offsets below are the independently recovered DGROUP addresses. */
float yarnball_sin30; /* DS:60F4 */
float yarnball_cos30; /* DS:60F0 */
float yarnball_aspect_ratio; /* DS:60EC */
int yarnball_radius_step; /* DS:60EA */
int yarnball_sparse_shift; /* DS:60E8 */
int yarnball_full_ring; /* DS:60E6 */
int yarnball_draw_flags; /* DS:60E4 */
int yarnball_center_y; /* DS:60E2 */
int yarnball_center_x; /* DS:60E0 */
int yarnball_radius_y; /* DS:60DE */
int yarnball_radius_x; /* DS:60DC */
int yarnball_x_sin30; /* DS:60DA */
int yarnball_x_cos30; /* DS:60D8 */
int yarnball_radius_sin30; /* DS:60D6 */
int yarnball_radius_cos30; /* DS:60D4 */
int wedges_step_y[3]; /* DS:60CE */
int wedges_step_x[3]; /* DS:60C8 */
int wedges_vertex_y[3]; /* DS:60C2 */
int wedges_vertex_x[3]; /* DS:60BC */
int tiles_y_extent; /* DS:60BA */
int tiles_fill_color_mode; /* DS:60B8 */
int tiles_saved_y; /* DS:60B6 */
int tiles_saved_x; /* DS:60B4 */
int tiles_base_fill_color; /* DS:60B2 */
int tiles_fill_rows_left; /* DS:60B0 */
int tiles_edge_step_y; /* DS:60AE */
int tiles_edge_step_x; /* DS:60AC */
int tiles_move_y; /* DS:60AA */
int tiles_move_x; /* DS:60A8 */
int tiles_edge_index; /* DS:60A6 */
int tiles_edge_progress; /* DS:60A4 */
int tiles_side_length; /* DS:60A2 */
int gumballs_shading_choice; /* DS:60A0 */
int gumballs_draw_flags; /* DS:609E */
int gumballs_edge_color; /* DS:609C */
int gumballs_step_y; /* DS:609A */
int gumballs_step_x; /* DS:6098 */
int gumballs_radius_step; /* DS:6096 */
int gumballs_center_y; /* DS:6094 */
int gumballs_center_x; /* DS:6092 */
int gumballs_radius_y; /* DS:6090 */
int diamonds_height_limit; /* DS:608E */
int diamonds_width_limit; /* DS:608C */
int diamonds_origin_limit_y; /* DS:608A */
int diamonds_origin_limit_x; /* DS:6088 */
int diamonds_height_step; /* DS:6086 */
int diamonds_width_step; /* DS:6084 */
int diamonds_step_y; /* DS:6082 */
int diamonds_step_x; /* DS:6080 */
int diamonds_height; /* DS:607E */
int diamonds_width; /* DS:607C */
int diamonds_closing_y; /* DS:607A */
int diamonds_vertex3_y; /* DS:6078 */
int diamonds_vertex2_y; /* DS:6076 */
int diamonds_vertex1_y; /* DS:6074 */
int diamonds_origin_y; /* DS:6072 */
int diamonds_closing_x; /* DS:6070 */
int diamonds_vertex3_x; /* DS:606E */
int diamonds_vertex2_x; /* DS:606C */
int diamonds_vertex1_x; /* DS:606A */
int diamonds_origin_x; /* DS:6068 */
int butterflies_restart; /* DS:6066 */
int butterflies_tip_offset; /* DS:6064 */
int butterflies_height_limit; /* DS:6062 */
int butterflies_draw_flags; /* DS:6060 */
int butterflies_edge_color; /* DS:605E */
int butterflies_fill_color; /* DS:605C */
int butterflies_center_y; /* DS:605A */
int butterflies_center_x; /* DS:6058 */
int butterflies_wing_y[6]; /* DS:604C */
int butterflies_mirrored_x[6]; /* DS:6040 */
int butterflies_wing_x[6]; /* DS:6034 */
int butterflies_vertex_index; /* DS:6032 */

/* Control-image geometry tables. */
int tiles_direction_x[4] = {
    -1, 0, 1, 0
};
int tiles_direction_y[4] = {
    0, -1, 0, 1
};
int tiles_side_lengths[2] = {
    0, 0
};
float yarnball_aspect_scale = 0.75f;
