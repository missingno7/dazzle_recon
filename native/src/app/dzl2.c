#include "dazzle.h"
#include "dzconst.h"
#include <stdlib.h>

extern int16_t draw_x;
extern int16_t draw_y;
extern int16_t screen_half_width;
extern int16_t screen_half_height;
extern uint16_t color_count;
extern uint16_t plot_color;
extern uint16_t video_adapter;
extern uint16_t palette_elapsed_ticks;
extern uint16_t palette_cycle_ticks;
extern uint16_t draw_symmetry;
extern void (*plot_callback)(uint16_t);
extern void ega_plot_symmetric_640x700(uint16_t);
extern void draw_polygon(int16_t, int16_t, int16_t  *, int16_t  *, uint16_t, uint16_t, uint16_t);
extern void draw_ellipse(int16_t, int16_t, int16_t, int16_t, int16_t, uint16_t, uint16_t, uint16_t);
extern void cycle_palette(void);

extern float yarnball_aspect_scale;
extern int16_t image_work_count;
extern int16_t tiles_direction_x[4];
extern int16_t tiles_direction_y[4];
extern int16_t tiles_side_lengths[2];

#define EGA_PLOT_CALLBACK (video_backend == BACKEND_EGA_640X700)

extern float yarnball_sin30;
extern float yarnball_cos30;
extern float yarnball_aspect_ratio;
extern int16_t yarnball_radius_step;
extern int16_t yarnball_sparse_shift;
extern int16_t yarnball_full_ring;
extern int16_t yarnball_draw_flags;
extern int16_t yarnball_center_y;
extern int16_t yarnball_center_x;
extern int16_t yarnball_radius_y;
extern int16_t yarnball_radius_x;
extern int16_t yarnball_x_sin30;
extern int16_t yarnball_x_cos30;
extern int16_t yarnball_radius_sin30;
extern int16_t yarnball_radius_cos30;
extern int16_t wedges_step_y[3];
extern int16_t wedges_step_x[3];
extern int16_t wedges_vertex_y[3];
extern int16_t wedges_vertex_x[3];
extern int16_t tiles_y_extent;
extern int16_t tiles_fill_color_mode;
extern int16_t tiles_saved_y;
extern int16_t tiles_saved_x;
extern int16_t tiles_base_fill_color;
extern int16_t tiles_fill_rows_left;
extern int16_t tiles_edge_step_y;
extern int16_t tiles_edge_step_x;
extern int16_t tiles_move_y;
extern int16_t tiles_move_x;
extern int16_t tiles_edge_index;
extern int16_t tiles_edge_progress;
extern int16_t tiles_side_length;
extern int16_t gumballs_shading_choice;
extern int16_t gumballs_draw_flags;
extern int16_t gumballs_edge_color;
extern int16_t gumballs_step_y;
extern int16_t gumballs_step_x;
extern int16_t gumballs_radius_step;
extern int16_t gumballs_center_y;
extern int16_t gumballs_center_x;
extern int16_t gumballs_radius_y;
extern int16_t diamonds_height_limit;
extern int16_t diamonds_width_limit;
extern int16_t diamonds_origin_limit_y;
extern int16_t diamonds_origin_limit_x;
extern int16_t diamonds_height_step;
extern int16_t diamonds_width_step;
extern int16_t diamonds_step_y;
extern int16_t diamonds_step_x;
extern int16_t diamonds_height;
extern int16_t diamonds_width;
extern int16_t diamonds_closing_y;
extern int16_t diamonds_vertex3_y;
extern int16_t diamonds_vertex2_y;
extern int16_t diamonds_vertex1_y;
extern int16_t diamonds_origin_y;
extern int16_t diamonds_closing_x;
extern int16_t diamonds_vertex3_x;
extern int16_t diamonds_vertex2_x;
extern int16_t diamonds_vertex1_x;
extern int16_t diamonds_origin_x;
extern int16_t butterflies_restart;
extern int16_t butterflies_tip_offset;
extern int16_t butterflies_height_limit;
extern int16_t butterflies_draw_flags;
extern int16_t butterflies_edge_color;
extern int16_t butterflies_fill_color;
extern int16_t butterflies_center_y;
extern int16_t butterflies_center_x;
extern int16_t butterflies_wing_y[6];
extern int16_t butterflies_mirrored_x[6];
extern int16_t butterflies_wing_x[6];
extern int16_t butterflies_vertex_index;

/* Scalar views of the closed polygon vertex arrays. */
#define butterflies_wing_x1 butterflies_wing_x[1]
#define butterflies_wing_x2 butterflies_wing_x[2]
#define butterflies_wing_x3 butterflies_wing_x[3]
#define butterflies_wing_x4 butterflies_wing_x[4]
#define butterflies_wing_y1 butterflies_wing_y[1]
#define butterflies_wing_y2 butterflies_wing_y[2]
#define butterflies_wing_y3 butterflies_wing_y[3]
#define butterflies_wing_y4 butterflies_wing_y[4]

/* Borland's int product wrapped before the signed division. */
static int16_t multiply_word(int16_t left, int16_t right)
{
    uint16_t low_word = (uint16_t)((uint32_t)(uint16_t)left * (uint32_t)(uint16_t)right);
    return low_word <= INT16_MAX
        ? (int16_t)low_word
        : (int16_t)((int32_t)low_word - INT32_C(65536));
}

/* Native builds have no DOS code-segment value.
 * Original 0B84:0000, 3 bytes. */
uint16_t dzl2_seg(void) { return 0; }

/* Grows two mirrored butterfly wing polygons at a random center.
 * initialize or butterflies_restart seeds a new shape; later calls grow and draw it.
 * Original 0B84:0003, 748 bytes. */
void butterflies(int16_t initialize)
{
    if (initialize || butterflies_restart) {
        butterflies_center_x = dz_random(screen_half_width * 3 / 4) + screen_half_width / 8;
        butterflies_center_y = dz_random(screen_half_height * 3 / 4) + screen_half_height / 8;
        butterflies_wing_x1 = dz_random(screen_half_width / 8) + screen_half_width / 16;
        butterflies_wing_y1 = dz_random(screen_half_height / 16);
        butterflies_height_limit = dz_random(screen_half_height / 2) + screen_half_height / 8;
        butterflies_tip_offset = dz_random(screen_half_width / 4) + 8;
        butterflies_edge_color = dz_random(color_count);
        butterflies_draw_flags = dz_random(3) + 1;
        if (video_adapter == ADAPTER_VGA)
            butterflies_draw_flags += POLYGON_FILL_COLOR_PER_ROW;
        if (dz_random(2))
            butterflies_draw_flags += POLYGON_EDGE_COLOR_PER_POINT;
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
        butterflies_wing_y4 = (int16_t)((int32_t)multiply_word(butterflies_wing_x3,
            butterflies_wing_y1) / butterflies_wing_x1);
        for (butterflies_vertex_index = 1; butterflies_vertex_index <= 4; ++butterflies_vertex_index)
            butterflies_mirrored_x[butterflies_vertex_index] = -butterflies_wing_x[butterflies_vertex_index];
        if (butterflies_wing_y1 > butterflies_height_limit ||
            butterflies_wing_y1 - butterflies_wing_y4 >= screen_half_height - 1) {
            butterflies_restart = 1;
            return;
        } else {
            butterflies_fill_color = dz_random(color_count);
            draw_polygon(1, 5, butterflies_wing_x, butterflies_wing_y,
                         (uint16_t)butterflies_fill_color, (uint16_t)butterflies_edge_color,
                         (uint16_t)butterflies_draw_flags);
            draw_polygon(1, 5, butterflies_mirrored_x, butterflies_wing_y,
                         (uint16_t)butterflies_fill_color, (uint16_t)butterflies_edge_color,
                         (uint16_t)butterflies_draw_flags);
        }
    }
    draw_x = butterflies_center_x;
    draw_y = butterflies_center_y;
}

/* Moves and resizes a diamond using the persistent origin, dimensions and signed steps.
 * initialize seeds its bounds; each call chooses polygon fill, edge and shading colors.
 * Original 0B84:02EF, 1079 bytes. */
void diamonds(int16_t initialize)
{
    int16_t step_refresh_gate = 0;
    int16_t edge_color;
    int16_t draw_flags;
    int16_t shading_choice;

    if (initialize) {
        step_refresh_gate = 0;
        diamonds_width_limit = screen_half_width / 2;
        diamonds_height_limit = screen_half_height / 2;
        diamonds_origin_limit_x = screen_half_width - screen_half_width / 4;
        diamonds_origin_limit_y = screen_half_height - screen_half_height / 4;
        diamonds_origin_x = dz_random(diamonds_origin_limit_x);
        diamonds_origin_y = dz_random(diamonds_origin_limit_y);
        diamonds_width = dz_random(diamonds_width_limit - 8) + 8;
        diamonds_height = dz_random(diamonds_height_limit - 8) + 8;
    }

    /* The reconstructed binary read an indeterminate automatic gate here.
     * Native starts it at zero so the intended per-call step refresh is defined. */
    if (step_refresh_gate <= 0) {
        (void)dz_random(4);
        if (dz_random(2))
            diamonds_step_x = dz_random(screen_half_width / 16) + 4;
        else
            diamonds_step_x = -(dz_random(screen_half_width / 16) + 4);
        if (dz_random(2))
            diamonds_step_y = dz_random(screen_half_height / 16) + 4;
        else
            diamonds_step_y = -(dz_random(screen_half_height / 16) + 4);
        diamonds_width_step = dz_random(screen_half_width / 4) - screen_half_width / 8;
        diamonds_height_step = dz_random(screen_half_width / 4) - screen_half_width / 8;
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

    edge_color = dz_random(color_count);
    if (++plot_color >= color_count)
        plot_color = FIRST_CYCLING_COLOR;
    draw_flags = 0;
    if (dz_random(2))
        draw_flags |= SHAPE_FILL_AND_OUTLINE;
    else {
        draw_flags |= SHAPE_OUTLINE;
        if (video_adapter == ADAPTER_VGA && dz_random(2))
            draw_flags |= POLYGON_EDGE_COLOR_PER_POINT;
    }
    if (video_adapter == ADAPTER_VGA) {
        shading_choice = dz_random(3);
        switch (shading_choice) {
        case 0: plot_color = (uint16_t)dz_random(color_count); break;
        case 1: draw_flags |= POLYGON_FILL_COLOR_PER_PIXEL; break;
        case 2: draw_flags |= POLYGON_FILL_COLOR_PER_ROW; break;
        }
    }
    draw_polygon(1, 4, &diamonds_origin_x, &diamonds_origin_y, plot_color,
                 (uint16_t)edge_color, (uint16_t)draw_flags);
}

/* Draws a gumball ellipse after random changes to its center and radius, undoing steps
 * outside the bounds. initialize seeds the geometry; each call chooses fill/edge shading.
 * Original 0B84:0726, 764 bytes. */
void gumballs(int16_t initialize)
{
    if (initialize) {
        gumballs_radius_y = dz_random(screen_half_height / 4 - 8) + 8;
        gumballs_center_x = screen_half_width / 4 + dz_random(screen_half_width / 2);
        gumballs_center_y = screen_half_height / 4 + dz_random(screen_half_height / 2);
    }
    gumballs_radius_step = dz_random(screen_half_height / 4) - screen_half_height / 8;
    gumballs_step_x = dz_random(screen_half_width / 4) - screen_half_width / 8;
    gumballs_step_y = dz_random(screen_half_height / 4) - screen_half_height / 8;
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
    gumballs_edge_color = dz_random(color_count);
    if (++plot_color >= color_count) plot_color = FIRST_CYCLING_COLOR;
    gumballs_draw_flags = 0;
    if (dz_random(2))
        gumballs_draw_flags |= SHAPE_FILL;
    else {
        gumballs_draw_flags |= SHAPE_OUTLINE;
        if (video_adapter == ADAPTER_VGA && dz_random(2))
            gumballs_draw_flags |= ELLIPSE_EDGE_COLOR_PER_POINT;
    }
    if (video_adapter == ADAPTER_VGA) {
        gumballs_shading_choice = dz_random(4);
        switch (gumballs_shading_choice) {
        case 0: plot_color = (uint16_t)dz_random(color_count); break;
        case 1: gumballs_draw_flags |= ELLIPSE_FILL_COLOR_PER_PIXEL; break;
        case 2: gumballs_draw_flags |= ELLIPSE_FILL_MIRRORED_COLORS; break;
        case 3: gumballs_draw_flags |= ELLIPSE_FILL_COLOR_PER_ROW; break;
        }
    }
    draw_ellipse(1, gumballs_radius_y, 0, gumballs_center_x, gumballs_center_y,
                 plot_color, (uint16_t)gumballs_edge_color, (uint16_t)gumballs_draw_flags);
}

/* Moves the pen to a new tile, optionally fills it, then traces four rectangle edges.
 * initialize seeds the pen and base color; enhanced EGA uses doubled logical Y steps.
 * Original 0B84:0A22, 1017 bytes. */
void tiles(int16_t initialize)
{
    /* Work in physical rows until the tile is handed to the doubled-Y EGA plotter. */
    if (EGA_PLOT_CALLBACK)
        draw_y >>= 1;

    if (initialize) {
        if (EGA_PLOT_CALLBACK)
            tiles_y_extent = screen_half_height / 2;
        else
            tiles_y_extent = screen_half_height;
        tiles_base_fill_color = (int16_t)plot_color;
        draw_x = screen_half_width / 4 + dz_random(screen_half_width / 2);
        draw_y = tiles_y_extent / 4 + dz_random(tiles_y_extent / 2);
    }

    tiles_move_x = dz_random(screen_half_width / 4) - screen_half_width / 8;
    tiles_move_y = dz_random(tiles_y_extent / 4) - tiles_y_extent / 8;
    tiles_side_lengths[0] = dz_random(screen_half_width / 2);
    tiles_side_lengths[1] = dz_random(tiles_y_extent / 2);
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

    if (dz_random(2)) {
        if (video_adapter == ADAPTER_VGA)
            tiles_fill_color_mode = dz_random(2) + 1;
        else
            tiles_fill_color_mode = 0;
        if (tiles_fill_color_mode == 2)
            plot_color = (uint16_t)tiles_base_fill_color;
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
                plot_color = (uint16_t)tiles_base_fill_color;
            tiles_side_length = tiles_side_lengths[0];
            while (tiles_side_length >= 0) {
                plot_callback(draw_symmetry);
                --draw_x;
                --tiles_side_length;
                if (tiles_fill_color_mode == 1 && ++plot_color >= color_count)
                    plot_color = FIRST_CYCLING_COLOR;
                ++image_work_count;
            }
            draw_x = (int16_t)(draw_x + tiles_side_lengths[0] + 1);
            --draw_y;
            --tiles_fill_rows_left;
            if (tiles_fill_color_mode == 2 && ++plot_color >= color_count)
                plot_color = FIRST_CYCLING_COLOR;
            if (EGA_PLOT_CALLBACK) {
                --draw_y;
                --tiles_fill_rows_left;
            }
        }
        draw_x = tiles_saved_x;
        draw_y = tiles_saved_y;
        if (tiles_fill_color_mode == 0 && ++plot_color >= color_count)
            plot_color = FIRST_CYCLING_COLOR;
        tiles_base_fill_color = (int16_t)plot_color;
    }

    if (EGA_PLOT_CALLBACK) {
        draw_y <<= 1;
        tiles_side_lengths[1] <<= 1;
    }
    plot_color = (uint16_t)dz_random(color_count);
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

/* Moves three persistent triangle vertices, reflects steps at the drawing bounds,
 * and draws a wedge with random fill/edge shading. initialize seeds the vertices.
 * Original 0B84:0E1B, 804 bytes. */
void wedges(int16_t initialize)
{
    int16_t vertex_index, polygon_x[4], polygon_y[4], step_refresh_gate = 0;
    int16_t edge_color, shading_choice, draw_flags;

    if (initialize) {
        step_refresh_gate = 0;
        for (vertex_index = 0; vertex_index < 3; ++vertex_index) {
            wedges_vertex_x[vertex_index] = screen_half_width / 4 + dz_random(screen_half_width / 2);
            wedges_vertex_y[vertex_index] = screen_half_height / 4 + dz_random(screen_half_height / 2);
        }
    }
    /* The reconstructed binary read an indeterminate automatic gate here.
     * Native starts it at zero so the intended per-call step refresh is defined. */
    if (step_refresh_gate <= 0) {
        (void)dz_random(8);
        for (vertex_index = 0; vertex_index < 3; ++vertex_index) {
            wedges_step_x[vertex_index] = dz_random(screen_half_width / 4) - screen_half_width / 8;
            wedges_step_y[vertex_index] = dz_random(screen_half_height / 4) - screen_half_height / 8;
        }
    }

    for (vertex_index = 0; vertex_index < 3; ++vertex_index) {
        wedges_vertex_x[vertex_index] += wedges_step_x[vertex_index];
        wedges_vertex_y[vertex_index] += wedges_step_y[vertex_index];
        if (wedges_vertex_x[vertex_index] < 0 || wedges_vertex_x[vertex_index] >= screen_half_width) {
            wedges_step_x[vertex_index] = -wedges_step_x[vertex_index];
            wedges_vertex_x[vertex_index] += wedges_step_x[vertex_index];
        }
        if (wedges_vertex_y[vertex_index] < 0 || wedges_vertex_y[vertex_index] >= screen_half_height) {
            wedges_step_y[vertex_index] = -wedges_step_y[vertex_index];
            wedges_vertex_y[vertex_index] += wedges_step_y[vertex_index];
        }
    }

    /* Polygon element zero is the translation; later elements are relative vertices. */
    polygon_x[0] = wedges_vertex_x[0];
    polygon_y[0] = wedges_vertex_y[0];
    polygon_x[1] = wedges_vertex_x[1] - wedges_vertex_x[0];
    polygon_y[1] = wedges_vertex_y[1] - wedges_vertex_y[0];
    polygon_x[2] = wedges_vertex_x[2] - wedges_vertex_x[0];
    polygon_y[2] = wedges_vertex_y[2] - wedges_vertex_y[0];

    edge_color = dz_random(color_count);
    if (++plot_color >= color_count)
        plot_color = FIRST_CYCLING_COLOR;
    draw_flags = 0;
    if (dz_random(2))
        draw_flags |= SHAPE_FILL_AND_OUTLINE;
    else {
        draw_flags |= SHAPE_OUTLINE;
        if (video_adapter == ADAPTER_VGA && dz_random(2))
            draw_flags |= POLYGON_EDGE_COLOR_PER_POINT;
    }
    if (video_adapter == ADAPTER_VGA) {
        shading_choice = dz_random(3);
        switch (shading_choice) {
        case 0: plot_color = (uint16_t)dz_random(color_count); break;
        case 1: draw_flags |= POLYGON_FILL_COLOR_PER_PIXEL; break;
        case 2: draw_flags |= POLYGON_FILL_COLOR_PER_ROW; break;
        }
    }
    draw_polygon(1, 3, polygon_x, polygon_y, plot_color,
                 (uint16_t)edge_color, (uint16_t)draw_flags);
}

/* Draws growing ellipses around a base center and three or twelve surrounding centers,
 * with aspect-corrected radius projections. initialize chooses the layout and radii.
 * Original 0B84:113F, 859 bytes. */
void yarnball(int16_t initialize)
{
     int16_t circle_x = 0, circle_y = 0;
    int16_t center_index;

    if (initialize) {
        yarnball_aspect_ratio = (float)screen_half_width / (float)screen_half_height * yarnball_aspect_scale;
        yarnball_cos30 = 0.866f;
        yarnball_sin30 = 0.5f;
        yarnball_draw_flags = SHAPE_OUTLINE;
        yarnball_radius_step = 1;
        yarnball_full_ring = dz_random(3);
        if (yarnball_full_ring != 0) {
            yarnball_radius_y = screen_half_height / 8 + dz_random(screen_half_height / 8);
            yarnball_center_y = (int16_t)(yarnball_radius_y * 2 + dz_random(screen_half_height / 4));
            yarnball_center_x = (int16_t)((float)dz_random(screen_half_width / 4) +
                                     ((float)(yarnball_radius_y * 2) * yarnball_aspect_ratio));
        } else {
            yarnball_sparse_shift = 1;
            yarnball_radius_y = screen_half_height / 4 + dz_random(screen_half_height * 3 / 4);
            yarnball_center_x = 0;
            yarnball_center_y = 0;
            yarnball_draw_flags |= ELLIPSE_EDGE_COLOR_PER_POINT;
        }
        return;
    }

    draw_ellipse(1, yarnball_radius_y, 0, yarnball_center_x, yarnball_center_y,
                 0, plot_color, (uint16_t)yarnball_draw_flags);

    yarnball_radius_cos30 = (int16_t)((float)yarnball_radius_y * yarnball_cos30);
    yarnball_radius_sin30 = (int16_t)((float)yarnball_radius_y * yarnball_sin30);
    yarnball_x_cos30 = (int16_t)((float)yarnball_radius_cos30 * yarnball_aspect_ratio);
    yarnball_x_sin30 = (int16_t)((float)yarnball_radius_sin30 * yarnball_aspect_ratio);
    yarnball_radius_x = (int16_t)((float)yarnball_radius_y * yarnball_aspect_ratio);

    center_index = 0;
    do {
        switch (center_index) {
        case 0: circle_x = yarnball_center_x + yarnball_radius_x; circle_y = yarnball_center_y; break;
        case 1: circle_x = yarnball_center_x - yarnball_radius_x; break;
        case 2: circle_x = yarnball_center_x; circle_y = yarnball_center_y + yarnball_radius_y; break;
        case 3: circle_y = yarnball_center_y - yarnball_radius_y; break;
        case 4: circle_x = yarnball_center_x + yarnball_x_cos30;
                circle_y = yarnball_center_y + yarnball_radius_sin30; break;
        case 5: circle_x = yarnball_center_x - yarnball_x_cos30; break;
        case 6: circle_y = yarnball_center_y - yarnball_radius_sin30; break;
        case 7: circle_x = yarnball_center_x + yarnball_x_cos30; break;
        case 8: circle_x = yarnball_center_x + yarnball_x_sin30;
                circle_y = yarnball_center_y + yarnball_radius_cos30; break;
        case 9: circle_x = yarnball_center_x - yarnball_x_sin30; break;
        case 10: circle_y = yarnball_center_y - yarnball_radius_cos30; break;
        case 11: circle_x = yarnball_center_x + yarnball_x_sin30; break;
        }
        draw_ellipse(0, yarnball_radius_y, 0, circle_x, circle_y,
                     0, plot_color, (uint16_t)yarnball_draw_flags);
        if (yarnball_full_ring != 0)
            ++center_index;
        else {
            /* Sparse layout visits surrounding centers 0, 4 and 8. */
            center_index = 2 << yarnball_sparse_shift++;
        }
    } while (center_index < 12);

    yarnball_radius_y += yarnball_radius_step;
    if (yarnball_full_ring != 0)
        yarnball_draw_flags ^= ELLIPSE_EDGE_COLOR_PER_POINT;
    if (++plot_color >= color_count)
        plot_color = FIRST_CYCLING_COLOR;
    yarnball_sparse_shift = 1;
}

/* Uninitialized module state. BC++ 3.0 emits BSS in reverse declaration order.
 * Offsets below are the independently recovered DGROUP addresses. */
float yarnball_sin30; /* DS:60F4 */
float yarnball_cos30; /* DS:60F0 */
float yarnball_aspect_ratio; /* DS:60EC */
int16_t yarnball_radius_step; /* DS:60EA */
int16_t yarnball_sparse_shift; /* DS:60E8 */
int16_t yarnball_full_ring; /* DS:60E6 */
int16_t yarnball_draw_flags; /* DS:60E4 */
int16_t yarnball_center_y; /* DS:60E2 */
int16_t yarnball_center_x; /* DS:60E0 */
int16_t yarnball_radius_y; /* DS:60DE */
int16_t yarnball_radius_x; /* DS:60DC */
int16_t yarnball_x_sin30; /* DS:60DA */
int16_t yarnball_x_cos30; /* DS:60D8 */
int16_t yarnball_radius_sin30; /* DS:60D6 */
int16_t yarnball_radius_cos30; /* DS:60D4 */
int16_t wedges_step_y[3]; /* DS:60CE */
int16_t wedges_step_x[3]; /* DS:60C8 */
int16_t wedges_vertex_y[3]; /* DS:60C2 */
int16_t wedges_vertex_x[3]; /* DS:60BC */
int16_t tiles_y_extent; /* DS:60BA */
int16_t tiles_fill_color_mode; /* DS:60B8 */
int16_t tiles_saved_y; /* DS:60B6 */
int16_t tiles_saved_x; /* DS:60B4 */
int16_t tiles_base_fill_color; /* DS:60B2 */
int16_t tiles_fill_rows_left; /* DS:60B0 */
int16_t tiles_edge_step_y; /* DS:60AE */
int16_t tiles_edge_step_x; /* DS:60AC */
int16_t tiles_move_y; /* DS:60AA */
int16_t tiles_move_x; /* DS:60A8 */
int16_t tiles_edge_index; /* DS:60A6 */
int16_t tiles_edge_progress; /* DS:60A4 */
int16_t tiles_side_length; /* DS:60A2 */
int16_t gumballs_shading_choice; /* DS:60A0 */
int16_t gumballs_draw_flags; /* DS:609E */
int16_t gumballs_edge_color; /* DS:609C */
int16_t gumballs_step_y; /* DS:609A */
int16_t gumballs_step_x; /* DS:6098 */
int16_t gumballs_radius_step; /* DS:6096 */
int16_t gumballs_center_y; /* DS:6094 */
int16_t gumballs_center_x; /* DS:6092 */
int16_t gumballs_radius_y; /* DS:6090 */
int16_t diamonds_height_limit; /* DS:608E */
int16_t diamonds_width_limit; /* DS:608C */
int16_t diamonds_origin_limit_y; /* DS:608A */
int16_t diamonds_origin_limit_x; /* DS:6088 */
int16_t diamonds_height_step; /* DS:6086 */
int16_t diamonds_width_step; /* DS:6084 */
int16_t diamonds_step_y; /* DS:6082 */
int16_t diamonds_step_x; /* DS:6080 */
int16_t diamonds_height; /* DS:607E */
int16_t diamonds_width; /* DS:607C */
int16_t diamonds_closing_y; /* DS:607A */
int16_t diamonds_vertex3_y; /* DS:6078 */
int16_t diamonds_vertex2_y; /* DS:6076 */
int16_t diamonds_vertex1_y; /* DS:6074 */
int16_t diamonds_origin_y; /* DS:6072 */
int16_t diamonds_closing_x; /* DS:6070 */
int16_t diamonds_vertex3_x; /* DS:606E */
int16_t diamonds_vertex2_x; /* DS:606C */
int16_t diamonds_vertex1_x; /* DS:606A */
int16_t diamonds_origin_x; /* DS:6068 */
int16_t butterflies_restart; /* DS:6066 */
int16_t butterflies_tip_offset; /* DS:6064 */
int16_t butterflies_height_limit; /* DS:6062 */
int16_t butterflies_draw_flags; /* DS:6060 */
int16_t butterflies_edge_color; /* DS:605E */
int16_t butterflies_fill_color; /* DS:605C */
int16_t butterflies_center_y; /* DS:605A */
int16_t butterflies_center_x; /* DS:6058 */
int16_t butterflies_wing_y[6]; /* DS:604C */
int16_t butterflies_mirrored_x[6]; /* DS:6040 */
int16_t butterflies_wing_x[6]; /* DS:6034 */
int16_t butterflies_vertex_index; /* DS:6032 */

/* Control-image geometry tables. */
int16_t tiles_direction_x[4] = {
    -1, 0, 1, 0
};
int16_t tiles_direction_y[4] = {
    0, -1, 0, 1
};
int16_t tiles_side_lengths[2] = {
    0, 0
};
float yarnball_aspect_scale = 0.75f;
