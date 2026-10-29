/* DAZZLE DZLE: first native image slice. Original function order retained. */
#include "dazzle.h"
#include "dzconst.h"

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
            if(error>=0) {error=(int16_t)(error-dx); draw_y=(int16_t)(draw_y+sy);}
            draw_x=(int16_t)(draw_x+sx); error=(int16_t)(error+dy);
            if(draw_x==end_x) return 1; else return 0;
        } else {
            if(error>=0) {error=(int16_t)(error-dy); draw_x=(int16_t)(draw_x+sx);}
            draw_y=(int16_t)(draw_y+sy); error=(int16_t)(error+dx);
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
    case 1: plot_color=(uint16_t)dz_random((uint16_t)color_count); break;
    case 2: plot_color=abs(draw_x); break;
    case 3: plot_color=abs(draw_y); break;
    case 4: plot_color=abs(draw_x)+abs(draw_y); break;
    case 5: color_term=(uint32_t)(int32_t)(int16_t)(draw_x*draw_x); y_term=(uint32_t)(int32_t)(int16_t)(draw_y*draw_y); color_term+=y_term; plot_color=(uint16_t)((color_term>>5)&(uint16_t)(color_count-1)); break;
    case 6: color_term=(uint32_t)(int32_t)(int16_t)(draw_x*draw_x); y_term=(uint32_t)(int32_t)(int16_t)(draw_y*draw_y); color_term-=y_term; plot_color=(uint16_t)((color_term>>5)&(uint16_t)(color_count-1)); break;
    case 7: y_term=(uint32_t)(int32_t)dz_asr16((int16_t)(draw_y*draw_y),5); color_term=(uint32_t)(int32_t)(int16_t)abs(draw_x); plot_color=(uint16_t)(y_term-color_term); break;
    case 8: color_term=(uint32_t)(int32_t)dz_asr16((int16_t)abs((int16_t)(draw_x*draw_y)),5); plot_color=(uint16_t)color_term; break;
    }
    plot_color&=color_count-1;
    if(plot_color<FIRST_CYCLING_COLOR) plot_color=FIRST_CYCLING_COLOR;
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
        plot_color = (uint16_t)dz_random(4) & color_index_mask;
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
            draw_x = screen_extent;
        }
        if (lines_base_y > screen_extent) {
            lines_step_y = 0 - (dz_random(4) + 1);
            lines_base_y = screen_extent;
        }
        draw_y = lines_base_y;
    } else {
        ++draw_y;
    }
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
        draw_x = draw_y = vertex_extent;
        sierpinski_vertices[1].x = sierpinski_vertices[1].y = sierpinski_vertices[2].y = vertex_extent;
        if (dz_random(2))
            sierpinski_vertices[0].x = sierpinski_vertices[0].y = sierpinski_vertices[2].x = vertex_extent / 2;
        else
            sierpinski_vertices[0].x = sierpinski_vertices[0].y = sierpinski_vertices[2].x = 0;
        sierpinski_fixed_color = dz_random(2);
        plot_color = (uint16_t)dz_random(4) & color_index_mask;
    }
    vertex_index = dz_random(3);
    if (vertex_index == 0 && sierpinski_fixed_color == 0 && ++plot_color >= color_count)
        plot_color = FIRST_CYCLING_COLOR;
    /* Divide the displacement to retain the original rounding toward zero. */
    draw_x = (int16_t)(draw_x + (int16_t)(sierpinski_vertices[vertex_index].x - draw_x) / 2);
    draw_y = (int16_t)(draw_y + (int16_t)(sierpinski_vertices[vertex_index].y - draw_y) / 2);
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

