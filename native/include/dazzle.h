/* Shared DAZZLE types, state and module interfaces. Native C17 port. */
#ifndef DAZZLE_H
#define DAZZLE_H

/* Native declarations derived from include/dazzle.h; DOS integer widths are explicit.
 * Segment getters, register/vector storage, Borland libc redeclarations and pointer
 * segment/offset aliases are deliberately absent. See native/README.md. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dzplat.h"

/* Explicit 8086 arithmetic right shift, including negative values. */
static inline int16_t dz_asr16(int16_t value, unsigned bits)
{
    int32_t divisor = (int32_t)(1u << bits);
    return (int16_t)(value < 0 ? -((-(int32_t)value + divisor - 1) / divisor) : value / divisor);
}

/* Portable conio state, retained for the menu port. */
struct text_info {
    uint8_t winleft, wintop, winright, winbottom, attribute, normattr;
    uint8_t currmode, screenheight, screenwidth, curx, cury;
};

typedef struct { int16_t x,y; } Point16;
typedef struct { int16_t dx[3],dy[3]; } ConeDirections;
typedef struct { int16_t x1,y1,x2,y2; } StarTracePoint;
typedef struct { float cosine,sine; } TrigPair;
typedef void (*ImageDrawRoutine)(int16_t);
typedef struct {
    ImageDrawRoutine draw;
    uint16_t size;
    char *name;
    uint16_t enabled, continuous;
    uint8_t key;
} ImageConfigRow;
typedef struct {
    uint8_t key;
    char *name;
    uint16_t enabled;
} FadeConfigRow;

/* Original DZLE program entry; native/src/main.c supplies the host entry. */
int dazzle_main(int argc, char **argv);

extern uint16_t video_adapter;
/* Native: the active drawing backend, set by configure_video_backend next to plot_callback.
 * Replaces the original's function-pointer identity tests (e.g. plot_callback == ega_plot_symmetric_640x700). */
enum video_backend_kind { BACKEND_VGA_320X400, BACKEND_VGA_320X200, BACKEND_EGA_640X700, BACKEND_EGA_640X350 };
extern enum video_backend_kind video_backend;
extern uint16_t plot_color;
extern void cycle_palette(void);
extern int16_t poll_graphics_keys(void);
extern void ega_plot_symmetric_640x700(uint16_t);
extern void restore_text_attributes(void);
extern void restore_runtime_callbacks(void);
extern void signal_diagnostic(int16_t signal_number, int16_t signal_type, uint16_t *registers);
extern void initialize_program_defaults(void);
int dazzle_main(int argc, char **argv);
extern void save_video_state(void);
extern void detect_video_adapter(void);
extern int16_t parse_command_line(int16_t argc, char * *argv, int16_t *option_position);
extern void print_startup_information(void);
extern uint16_t keyboard_led_flags(uint16_t mask, uint16_t mode);
extern void fatal_selfcheck_error(int16_t show_crc, const char *reason);
extern void check_executable_integrity(void);
extern void configure_video_backend(int16_t allocate);
extern int16_t resolve_startup_configuration(int16_t requested);
extern void install_runtime_callbacks(void);
extern void beep(uint16_t hz, uint16_t ms);
extern void park_disk_heads(void);
extern void prepare_logo_image_resources(void);
extern void semi_auto_loop(void);
extern void set_ega_raster_xor(int16_t mode);
extern void closing_graphic_and_music(void);
extern void restore_video_state(int16_t);
extern void print_closing_text_and_exit(int16_t status);

extern void butterflies(int16_t);
extern void diamonds(int16_t);
extern void gumballs(int16_t);
extern void tiles(int16_t);
extern void wedges(int16_t);
extern void yarnball(int16_t);

extern char *executable_name_pointer;
extern char *help_program_name;
extern char *program_revision;
extern char *registration_notice_pointer;
extern int16_t direction_x[4], direction_y[4];
extern ConeDirections cone_directions[4];
extern uint8_t grid_patterns[256];
extern int16_t diamond_x[4], diamond_y[4];
extern int16_t zigstream_dx[2], zigstream_dy[2];
extern ImageConfigRow alt_image_records[36];
extern ImageConfigRow ctrl_image_records[6];
extern FadeConfigRow fade_records[24];

extern int16_t line_step(int16_t endx,int16_t endy,int16_t initialize);
extern void choose_color(int16_t kind);
extern void square(int16_t n,int16_t symmetry);
extern void draw_polygon(int16_t rebuild,int16_t n,int16_t *px,int16_t *py,uint16_t fill_color,uint16_t edge_color,uint16_t flags);
extern int16_t ellipse_step(int16_t radius_y,int16_t radius_x,int16_t initialize);
extern void draw_ellipse(int16_t rebuild,int16_t ry,int16_t rx,int16_t cx,int16_t cy,uint16_t fill,uint16_t edge,uint16_t flags);
extern void swap_pixel_rectangles(int16_t flip,int16_t x1,int16_t y1,int16_t x2,int16_t y2,int16_t width,int16_t height);
extern void arches(int16_t rebuild);
extern void atom(int16_t rebuild);
extern void balls(int16_t rebuild);
extern void bars(int16_t rebuild);
extern void bigstar(int16_t rebuild);
extern void boxes(int16_t rebuild);
extern void cones(int16_t rebuild);
extern void corkscrew(int16_t rebuild);
extern void curves(int16_t rebuild);
extern void cycloids(int16_t rebuild);
extern void daisies(int16_t rebuild);
extern void diag1(int16_t size);
extern void diag2(int16_t size);
extern void ellipse(int16_t size);
extern void formula1(int16_t size);
extern void grid(int16_t size);
extern void hexagon(int16_t size);
extern void hilbert(int16_t size);
extern void lines(int16_t size);
extern void littleStars(void);
extern void lotsadots(int16_t size);
extern void pedals(int16_t size);
extern void puffs(int16_t size);
extern void ribbons(int16_t size);
extern void sierpinski(int16_t size);
extern void sine(int16_t rebuild);
extern void squares(int16_t size);
extern void squirals(int16_t size);
extern void spiral(int16_t size);
extern void spiral1(int16_t size);
extern void sweep(int16_t size);
extern void sweep2(int16_t size);
extern void triangles(int16_t size);
extern void tunnel(int16_t size);
extern int16_t wandering(int16_t size);
extern void zigstream(int16_t size);

/* DOSI shared state and interfaces. */
typedef struct {
    uint16_t width, height;
    uint8_t *pixels;
} PaletteMapSpec;
typedef void (*VideoCallback)(void);
typedef uint16_t (*ReadPixelCallback)(int16_t, int16_t);
typedef void (*PaletteCallback)(uint8_t *);

extern uint32_t crc_polynomial;
extern uint16_t display_freeze_active;
extern uint16_t display_freeze_cycle_requested;
extern uint16_t menu_active;
extern uint16_t palette_callback_active;
extern float tone_base_frequency;
extern uint16_t tone_chord[4];
extern uint16_t force_ega;
extern uint16_t force_vga;
extern void menu_error_message(char *);
extern void upload_reserved_dac_entries(uint16_t);
extern void help_screen(void);
extern void configuration_screen(void);
extern void save_pcx(void);
extern void ega_set_mode(uint16_t);
extern void ega_plot_symmetric_640x350(uint16_t);
extern void draw_closing_graphic(void);
extern void set_display_start(uint16_t, uint16_t);
extern void vga_set_mode_320x200(uint16_t);
extern void vga_set_mode_320x400(uint16_t);
extern void vga_plot_symmetric_320x200(uint16_t);
extern void vga_plot_symmetric_320x400(uint16_t);
extern uint16_t vga_read_pixel_320x200(int16_t, int16_t);
extern uint16_t vga_read_pixel_320x400(int16_t, int16_t);
extern uint16_t ega_read_pixel_640x350(int16_t, int16_t);
extern uint16_t ega_read_pixel_640x700(int16_t, int16_t);
extern void vga_upload_palette_bios_split(uint8_t *);
extern void vga_upload_palette_8086_split(uint8_t *);
extern void vga_upload_palette_rep_split(uint8_t *);
extern void vga_upload_palette_bios(uint8_t *);
extern void vga_upload_palette_8086(uint8_t *);
extern void vga_upload_palette_rep(uint8_t *);
extern void ega_upload_palette_bios(uint8_t *);
extern void ega_upload_palette_ports(uint8_t *);
extern void extract_manual(void);
extern int16_t cpu_has_post_8086_shifts(void);
extern void display_boundary_fade_and_pan(void);
extern void mode_ready_chirp(void);
extern int16_t default_signal_result(void);
extern int16_t _signal_result(void);
/* Borland matherr payload, represented without DOS ABI aliasing. */
struct exception { int16_t type; char *name; double arg1, arg2, retval; };
extern int16_t interrupt_signal_result(void);
extern int16_t matherr(struct exception *frame);
extern int16_t probe_mouse(uint16_t mask);
extern void mouse_position(int16_t buttons, int16_t x, int16_t y);
extern void read_mz_resource_base(FILE *input);
extern void *allocate(uint16_t *amount, uint16_t minimum);
extern uint32_t genCRC(FILE *input, int32_t length);
extern int16_t deRead(void *metadata, uint16_t size);
extern void print_order_info(int16_t show_order_form);
extern void command_line_help(void);
extern int16_t reject_exec_filename(char *filename);
extern void generate_palette(void);
extern void randomize_palette_state(uint16_t replace_base_colors);
extern void change_background_palette(void);
extern void build_ega_palette(int16_t update_cycle);
extern int16_t freeze_display(uint16_t operation);
extern void initialize_math_tables(void);
extern void initialize_palette_mapping(PaletteMapSpec *mapping);
extern void beep_float(float hz, uint16_t ms);
extern void play_tone_sequence(void);
extern int16_t parse_config_version(char *line);
extern void parse_alt_image_size(char *line);
extern void parse_ctrl_image_size(char *line);
extern void parse_fade_enabled(char *line);
extern void parse_auto_configuration(char *line);
extern int16_t load_configuration_file(char *path);
extern int16_t save_configuration_file(char *path);
extern void auto_loop(void);
extern void graphics_timer_tick_isr(void);
extern void keyboard_pending_isr(void);
extern void mouse_pending_callback(void);

/* SEG0CCD interfaces. */
extern uint16_t dzfd_seg(void); /* Native compatibility getter returns zero. */
typedef struct {
    int16_t head_x, head_y;
    int16_t tail_x, tail_y;
    int16_t velocity_x, velocity_y;
    int16_t tail_start, tail_end;
    int16_t unknown_10; /* Not accessed by this routine. */
} transition_particle_t;

typedef union {
    int32_t value;
    struct { uint16_t fraction; int16_t whole; } part;
} transition_point_t;

typedef union {
    uint32_t value;
    struct { uint16_t fraction; int16_t whole; } part;
} spiral_point_t;

struct LineWipePoint {
    uint16_t x0, y0, x1, y1;
};

union StarburstFixed {
    int32_t value;
    struct {
        uint16_t fraction;
        int16_t integer;
    } word;
};

struct StarburstState {
    union StarburstFixed x;
    union StarburstFixed y;
    union StarburstFixed dx;
    union StarburstFixed dy;
    uint16_t color;
};

extern uint16_t page_paragraph;
extern int16_t fireworks_unused_active_view[4];
extern int16_t moving_bars_patterns[8][7];
extern void wait_fade_tick(int16_t, int16_t);
extern int16_t countdown_digit_segments[5][6];
extern int16_t countdown_segment_bounds[8][4];
extern int16_t ziti_sweep_patterns[8][8];
extern void ega_set_line_compare(uint16_t);
extern void vga_set_line_compare(uint16_t);
/* High byte of background_fill_word (DS:5D8A). */
extern void fade_countdown(void);
extern void fade_ziti(void);
extern void fade_umbra(void);
extern void fade_moving_bars(void);
extern void fade_tetris(void);
extern void fade_exclusive_or(void);
extern void fade_twist(void);
extern void swap_rectangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2);
extern void fade_rectangles(void);
extern void fade_linewipe(void);
extern void fade_starburst(void);
extern void fade_wipe(int16_t paint_columns);
extern void hexagon_transition(void);
extern void fade_blister(void);
extern void fade_dac_to_black(int16_t mask_high_bits);
extern void fade_palette_to_background(void);
extern void spiral_midpoint(int16_t first, int16_t last, uint32_t profile[]);
extern void randomize_transition(void);
extern void drip_transition(void);
extern void firefly_transition(int16_t mode);
extern void gas_cloud(void);
extern void spiral_transition(void);
extern void random_delay_transition(void);
extern void mesh_transition(void);
extern void cleanup_transition(void);
extern void quilt_transition(void);
extern int16_t dispatch_fade_key(void);
extern void animate_vertical_display_start(int16_t, int16_t);
extern void animate_line_compare_split(int16_t reverse);
extern void clear_vram_words(uint16_t, uint16_t, uint16_t);
extern void seed_plasma(void);
extern void randomize_helper(int16_t);
/* fade_timer_ticks is the volatile view of countdown_ticks (DS:5D78).
 * The shared declaration needs reconciliation by its owner. */

/* SEG17F5 interfaces. */
extern char *default_open_path;
extern char *default_open_mode;

extern uint16_t logo_input_buffer_size;
struct image_header {
    char reserved[0x42];
    int16_t output_size;
};
extern uint16_t logo_pending_output_offset;
extern uint16_t video_adapter;
extern uint16_t logo_restore_video_adapter;
extern void vga_set_mode_320x200(uint16_t mode);
extern void ega_set_mode(uint16_t mode);
extern void ega_init_row_table_and_mode(void);
extern void ega_write_planar_row(uint8_t *source, uint16_t width,
                                     uint16_t frame);
extern void configure_logo_dac(void);
extern void rgb8_to_ega_palette(uint8_t *destination,
                               uint8_t *source, uint16_t mask);

extern void setmem(void *destination, uint16_t count, char value);

/* BSS first declarations, in reverse link-address order. */
extern uint16_t image_data_offset;
extern uint16_t serial_r2;
extern uint16_t serial_r1;
extern uint32_t expected_crc;
extern int16_t effect_parameter;
extern uint32_t computed_crc;
extern struct text_info saved_text_info;
extern FILE *selfcheck_stream;
extern FILE *configuration_file_stream;
extern char *default_config_name;
extern TrigPair trig_table[64];
extern int16_t draw_x;
extern int16_t draw_y;
extern uint16_t palette_cycle_offset;
extern uint16_t original_ctrlbreak_setting;
extern uint16_t palette_buffer_bytes;
extern uint16_t display_pan_state;
extern uint16_t bios_resolution_mode;
extern uint16_t palette_fixed_mode;
extern uint16_t palette_alternate_mode;
extern uint16_t hide_ega_border;
extern uint16_t option_y_requested;
extern uint16_t palette_random_mode;
extern uint16_t split_vga_palette_updates;
extern uint16_t unused_palette_state;
extern uint16_t use_8086_palette_io;
extern uint16_t keep_typematic;
extern uint16_t palette_four_ega_colors;
extern uint16_t show_mode;
extern uint16_t no_disk_park;
extern uint16_t bashful_mode;
extern uint16_t insecure_argument_index;
extern uint16_t draw_delay_count;
extern uint16_t targa64_mode;
extern uint16_t option_plus_requested;
extern uint16_t color_index_mask;
extern int16_t muted;
extern uint16_t window_mode;
extern uint16_t palette_entries;
extern uint16_t palette_cycle_wrap_bytes;
extern uint16_t color_count;
extern uint16_t use_bios_palette;
extern int16_t screen_half_height;
extern int16_t screen_half_width;
extern int16_t screen_extent;
extern int16_t rectangles_angle_index;
extern int16_t ega_color_substep;
extern uint16_t auto_fade_pause_ticks;
extern uint16_t palette_cycle_ticks;
extern uint16_t palette_step;
extern uint16_t default_palette_cycle_ticks;
extern uint16_t semi_auto_mode;
extern uint16_t semi_auto_continuous;
extern uint16_t alt_image_count;
extern uint16_t ctrl_image_count;
extern uint16_t fade_count;
extern uint8_t *palette_buffer;
extern uint16_t current_graphics_key;
extern uint16_t previous_graphics_key;
extern uint16_t startup_active;
extern int16_t ega_border_color;
extern uint16_t background_fill_word;
extern uint16_t maximum_image_size;
extern uint16_t show_duration_ticks;
extern uint16_t startup_config_failure;
extern uint16_t freeze_led_state;
extern uint16_t exit_requested;
extern uint16_t input_pending;
extern uint16_t mouse_abort_pending;
extern uint16_t palette_elapsed_ticks;
extern uint16_t countdown_ticks;
extern uint16_t unused_input_state;
extern uint16_t timer_hook_installed;
extern uint16_t keyboard_hook_installed;
extern uint16_t skip_ctrlbreak_control;
extern uint16_t palette_base_blue;
extern uint16_t palette_base_green;
extern int32_t resource_base;
extern uint8_t unused_path_state;
extern char executable_path[80];
extern char startup_directory[80];
extern uint16_t minimum_images_per_display;
extern uint16_t maximum_images_per_display;
extern uint16_t current_image_index;
extern uint16_t diagonal_symmetry_selected;
extern uint16_t image_work_limit;
extern int16_t image_work_count;
extern uint16_t mouse_callback_installed;
extern uint16_t option_bang_requested;
extern uint16_t mouse_available;
extern uint16_t image_xor_mode;
extern int16_t display_cleared;
extern uint16_t draw_symmetry;
extern char configuration_filename[80];
extern uint16_t *current_image_header;
extern void *linewipe_history;
extern uint8_t *ball_bitmap_pixels;
extern uint16_t ball_bitmap_width;
extern uint16_t ball_bitmap_height;
extern ImageDrawRoutine current_image_draw;
extern void (*palette_build_callback)(int16_t);
extern void (*mode_setup_callback)(uint16_t);
extern void (*plot_callback)(uint16_t);
extern uint16_t (*read_pixel_callback)(int16_t,int16_t);
extern void (*palette_upload_callback)(uint8_t *);
extern void (*display_start_callback)(uint16_t, uint16_t);
extern int16_t zigstream_step_y;
extern int16_t zigstream_step_x;
extern int16_t zigstream_direction;
extern int16_t zigstream_color_phase;
extern int16_t zigstream_run_length;
extern int16_t zigstream_run_counter;
extern int16_t wandering_line_done;
extern int16_t wandering_end_dy;
extern int16_t wandering_start_dy;
extern int16_t wandering_end_dx;
extern int16_t wandering_start_dx;
extern int16_t wandering_segments_left;
extern int16_t wandering_end_y;
extern int16_t wandering_end_x;
extern int16_t wandering_start_y;
extern int16_t wandering_start_x;
extern int16_t *tunnel_right_edge_ptr;
extern int16_t tunnel_right_limit;
extern int16_t tunnel_rows_left;
extern int16_t tunnel_row_left;
extern int16_t triangles_expand_left;
extern int16_t triangles_right_limit;
extern int16_t triangles_active;
extern int16_t triangles_row_left;
extern int16_t triangles_row_right;
extern int16_t sweep2_line_done;
extern int16_t sweep2_end_x_step;
extern int16_t sweep2_end_x;
extern int16_t sweep2_end_y;
extern int16_t sweep_rise_interval;
extern int16_t sweep_rise_counter;
extern int16_t spiral1_side_limit;
extern int16_t spiral1_step_y;
extern int16_t spiral1_step_x;
extern int16_t spiral1_direction;
extern int16_t spiral1_side_length;
extern int16_t spiral1_side_step;
extern int16_t spiral_step_y;
extern int16_t spiral_step_x;
extern int16_t spiral_direction;
extern int16_t spiral_color_phase;
extern int16_t spiral_leg_length;
extern int16_t spiral_leg_step;
extern int16_t squares_side_limit;
extern int16_t squares_step_y;
extern int16_t squares_step_x;
extern int16_t squares_direction;
extern int16_t squares_side_length;
extern int16_t squares_side_step;
extern float step_y;
extern float step_x;
extern float mod_y;
extern float mod_x;
extern float sine_phase;
extern float amplitude_y;
extern float amplitude_x;
extern int16_t sierpinski_fixed_color;
extern Point16 sierpinski_vertices[3];
extern int16_t ribbons_run_limit;
extern int16_t ribbons_run_length;
extern int16_t ribbons_run_step;
extern char puffs_drift_step;
extern int16_t puffs_line_done;
extern int16_t puffs_spread;
extern int16_t puffs_end_y;
extern int16_t puffs_end_x;
extern int16_t puffs_origin_y;
extern int16_t puffs_origin_x;
extern int16_t pedals_stroke_index;
extern int16_t pedals_phase;
extern int16_t pedals_start_radius;
extern int16_t pedals_radius;
extern int16_t lines_base_y;
extern int16_t lines_step_y;
extern int16_t lines_step_x;
extern int16_t lines_stroke_index;
extern int16_t hilbert_restart;
extern int16_t hilbert_stroke_width;
extern int16_t hilbert_color_per_point;
extern int16_t hilbert_step_y;
extern int16_t hilbert_step_x;
extern int16_t hilbert_depth;
extern int16_t hilbert_target_y;
extern int16_t hilbert_target_x;
extern int16_t hilbert_previous_y;
extern int16_t hilbert_previous_x;
extern int16_t hexagon_start_x;
extern int16_t hexagon_vertical_limit;
extern uint16_t grid_next_y;
extern uint16_t grid_next_x;
extern int16_t grid_pattern_index;
extern int16_t grid_tile_left;
extern int16_t grid_tile_right;
extern int16_t formula1_color_shift;
extern int16_t formula1_y_square_factor;
extern int16_t formula1_x_shift;
extern int32_t g_long5b56;
extern int32_t g_long5b52;
extern int32_t g_long5b4e;
extern int32_t g_long5b4a;
extern int32_t g_long5b46;
extern int32_t g_long5b42;
extern int32_t g_long5b3e;
extern int32_t g_long5b3a;
extern int32_t g_long5b36;
extern int16_t ellipse_center_y;
extern int16_t ellipse_center_x;
extern int16_t ellipse_offset_y;
extern int16_t ellipse_offset_x;
extern int16_t ellipse_draw_mode;
extern int16_t ellipse_region;
extern int16_t ellipse_line_done;
extern int16_t diag2_color_period;
extern int16_t diag2_color_counter;
extern int32_t daisies_sine_table[10];
extern int32_t daisies_cosine_table[10];
extern int32_t daisies_aspect_scale;
extern int16_t daisies_sample_limit;
extern int16_t daisies_sample_start;
extern Point16 daisies_edge_points[66];
extern int16_t daisies_base_color;
extern int16_t daisies_color_per_ray;
extern int16_t daisies_sample_step;
extern int16_t daisies_radius_step;
extern int16_t daisies_radius;
extern int16_t daisies_arc_variant;
extern int16_t daisies_trig_index;
extern int16_t daisies_work_index;
extern int16_t daisies_edge_count;
extern int16_t daisies_line_done;
extern int16_t daisies_sample_index;
extern int16_t daisies_center_y;
extern int16_t daisies_center_x;
extern int16_t daisies_edge_end_y;
extern int16_t daisies_edge_end_x;
extern int16_t daisies_saved_y;
extern int16_t daisies_saved_x;
extern int32_t cycloids_aspect_scale;
extern int16_t cycloids_next_curve;
extern int16_t cycloids_curve_index;
extern int16_t cycloids_line_done;
extern int16_t cycloids_pen_y;
extern int16_t cycloids_pen_x;
extern int16_t cycloids_center_y;
extern int16_t cycloids_center_x;
extern int16_t cycloids_spin_step;
extern int16_t cycloids_orbit_step;
extern int16_t cycloids_phase_advance;
extern int16_t cycloids_pen_radius_step;
extern int16_t cycloids_color_per_point;
extern int16_t cycloids_curve_limit;
extern int16_t cycloids_orbit_radius;
extern int16_t cycloids_pen_radius;
extern int16_t cycloids_rolling_radius;
extern int16_t cycloids_outer_radius;
extern int16_t cycloids_pen_phase;
extern int16_t cycloids_spin_phase;
extern int16_t cycloids_orbit_phase;
extern int16_t cycloids_start_phase;
extern int16_t curves_amplitude;
extern int16_t curves_phase_step_y;
extern int16_t curves_phase_step_x;
extern int16_t curves_phase_y;
extern int16_t curves_phase_x;
extern int16_t curves_stroke_index;
extern int16_t corkscrew_color_counter;
extern int16_t corkscrew_origin_y;
extern int16_t corkscrew_origin_x;
extern float corkscrew_phase;
extern float corkscrew_growth_step;
extern float corkscrew_distance;
extern float corkscrew_across;
extern float corkscrew_along;
extern int16_t boxes_restart;
extern uint16_t boxes_edge_progress;
extern int16_t boxes_edge_state;
extern int16_t boxes_line_done;
extern int16_t boxes_target_y;
extern int16_t boxes_target_x;
extern int16_t boxes_height;
extern int16_t boxes_width;
extern int16_t boxes_origin_y;
extern int16_t boxes_origin_x;
extern float bigstar_inner_radius;
extern StarTracePoint bigstar_traces[175];
extern int16_t bigstar_base_color;
extern int16_t bigstar_use_steep_trace;
extern int16_t bigstar_trace_index;
extern int16_t bigstar_shallow_count;
extern int16_t bigstar_steep_count;
extern int16_t bigstar_color_mode;
extern int16_t bigstar_spacing_shift;
extern int16_t bigstar_trace_step;
extern int16_t bigstar_resize_pending;
extern int16_t bigstar_segment_state;
extern int16_t bigstar_restart;
extern int16_t bigstar_line_done;
extern int16_t bigstar_target_y;
extern int16_t bigstar_target_x;
extern int16_t bigstar_diagonal_y;
extern int16_t bigstar_diagonal_x;
extern int16_t bigstar_axis_y;
extern int16_t bigstar_axis_x;
extern uint16_t bars_line_done;
extern uint16_t bars_span_y;
extern uint16_t bars_span_x;
extern uint16_t bars_end_y;
extern uint16_t bars_end_x;
extern uint16_t bars_start_y;
extern uint16_t bars_start_x;
extern uint16_t balls_direction;
extern int16_t balls_step_y;
extern int16_t balls_step_x;
extern int16_t balls_run_length;
extern int16_t balls_run_progress;
extern int16_t balls_origin_y;
extern int16_t balls_origin_x;
extern uint16_t balls_sample_x;
extern uint16_t balls_sample_y;
extern int16_t atom_stroke_index;
extern int16_t atom_phase;
extern int16_t atom_radius;
extern int16_t arches_second_radius_x;
extern int16_t arches_second_radius_y;
extern int16_t arches_second_center_x;
extern int16_t arches_first_radius_x;
extern int16_t arches_first_radius_y;
extern int16_t arches_first_center_y;
extern int16_t ellipse_trace_limit;
extern int16_t ellipse_fill_color;
extern int16_t ellipse_fill_mode;
extern int16_t ellipse_fill_min_x;
extern int16_t ellipse_fill_x;
extern int16_t ellipse_trace_done;
extern int16_t ellipse_direction_steps[700];
extern int16_t ellipse_trace_y;
extern int16_t ellipse_trace_x;
extern int16_t ellipse_trace_index;
extern int16_t ellipse_first_region;
extern int32_t ellipse_step_gradient_y;
extern int32_t ellipse_step_gradient_x;
extern int32_t ellipse_step_error;
extern int32_t ellipse_step_twice_y_sq;
extern int32_t ellipse_step_twice_x_sq;
extern int32_t ellipse_step_radius_y_sq;
extern int32_t ellipse_step_radius_x_sq;
extern int32_t ellipse_step_radius_y;
extern int32_t ellipse_step_radius_x;
extern int16_t polygon_row_limit;
extern int16_t polygon_origin_y;
extern int16_t polygon_origin_x;
extern int16_t polygon_min_y;
extern int16_t polygon_max_y;
extern int16_t polygon_row_right[350];
extern int16_t polygon_row_left[350];
extern int16_t error;
extern int16_t sy;
extern int16_t sx;
extern int16_t dy;
extern int16_t dx;
extern int16_t saved_pan_draw_delay;
extern int16_t tetris_source_y[21];
extern uint16_t tetris_block_colors[21];
extern int16_t tetris_block_y[21];
extern uint16_t tetris_column_active[21];
extern int16_t firefly_path_state[17];
extern int16_t firefly_y_history[5];
extern int16_t firefly_x_history[5];
extern int16_t quilt_tile_height;
extern int16_t quilt_tile_width;
extern int16_t quilt_second_y;
extern int16_t quilt_second_x;
extern int16_t quilt_first_y;
extern int16_t quilt_first_x;
extern int16_t quilt_flip_mode;
extern int16_t quilt_grid_divisions;
extern float fireworks_aspect_ratio;
extern transition_particle_t fireworks_particles[4][64];
extern int16_t fireworks_displaced_origins;
extern int16_t fireworks_background_x_mask;
extern int16_t fireworks_trail_mode;
extern int16_t fireworks_background_b;
extern int16_t fireworks_background_a;
extern int16_t fireworks_acceleration;
extern int16_t fireworks_gravity[4];
extern int16_t fireworks_active[4];
extern int16_t fireworks_launch_delay[4];
extern int16_t fireworks_colors[4];
extern int16_t fireworks_finished_particles[4];
extern int16_t fireworks_age[4];
extern int16_t fireworks_origin_y[4];
extern int16_t fireworks_origin_x[4];
/* Unreferenced allocation between the mouse REGPACK and logo state.
 * Four bytes are independently bounded; original name/type/owner are unknown. */
extern uint8_t reserved_program_state[4];
extern uint16_t ega_red_limit;
extern uint16_t ega_blue_limit;
extern uint16_t ega_green_limit;
extern struct image_header *logo_image_header;
extern uint16_t logo_image_height;
extern uint16_t logo_previous_aspect;
extern uint16_t logo_previous_video_adapter;
extern uint16_t logo_frame_state[2];
extern uint16_t image_frame_count;
extern uint16_t logo_input_buffer_index;
extern uint16_t logo_input_buffer_offset;
extern uint16_t logo_decode_state;
extern uint16_t image_start_offset;
extern uint16_t image_frame_size;
extern uint16_t logo_plane_state[2];
extern char *logo_input_buffers[2];
extern uint32_t image_source_bytes;

/* Typed views of shared fields; no duplicate storage. */
#define background_alt_color (((uint8_t *)&background_fill_word)[1])
#define bios_key_scan_code (((uint8_t *)&current_graphics_key)[1])
#define image_source_bytes_low (((uint16_t *)&image_source_bytes)[0])
#define image_source_offset (((uint16_t *)&image_source_bytes)[0])
#define image_file_position (*(uint32_t *)&resource_base)
#define image_stream (*(void * *)&selfcheck_stream)
#define original_text_attribute (saved_text_info.attribute)
#define fade_randomize_enabled (*(int16_t *)&fade_records[0].enabled)
#define fade_blister_enabled (*(int16_t *)&fade_records[1].enabled)
#define fade_countdown_enabled (*(int16_t *)&fade_records[2].enabled)
#define fade_drip_enabled (*(int16_t *)&fade_records[3].enabled)
#define fade_exclusive_or_enabled (*(int16_t *)&fade_records[4].enabled)
#define fade_firefly_enabled (*(int16_t *)&fade_records[5].enabled)
#define fade_gas_cloud_enabled (*(int16_t *)&fade_records[6].enabled)
#define fade_spiral_enabled (*(int16_t *)&fade_records[7].enabled)
#define fade_juggle_palette_enabled (*(int16_t *)&fade_records[8].enabled)
#define fade_kleenup_enabled (*(int16_t *)&fade_records[9].enabled)
#define fade_linewipe_enabled (*(int16_t *)&fade_records[10].enabled)
#define fade_mesh_enabled (*(int16_t *)&fade_records[11].enabled)
#define fade_fireworks_enabled (*(int16_t *)&fade_records[12].enabled)
#define fade_pan_scroll_enabled (*(int16_t *)&fade_records[13].enabled)
#define fade_quilt_enabled (*(int16_t *)&fade_records[14].enabled)
#define fade_rectangles_enabled (*(int16_t *)&fade_records[15].enabled)
#define fade_starburst_enabled (*(int16_t *)&fade_records[16].enabled)
#define fade_twist_enabled (*(int16_t *)&fade_records[17].enabled)
#define fade_umbra_enabled (*(int16_t *)&fade_records[18].enabled)
#define fade_moving_bars_enabled (*(int16_t *)&fade_records[19].enabled)
#define fade_wipe_enabled (*(int16_t *)&fade_records[20].enabled)
#define fade_hexagon_enabled (*(int16_t *)&fade_records[21].enabled)
#define fade_tetris_enabled (*(int16_t *)&fade_records[22].enabled)
#define fade_ziti_enabled (*(int16_t *)&fade_records[23].enabled)

#define fade_timer_ticks (*(volatile uint16_t *)&countdown_ticks)

/* Menu descriptors and directory workspace. */
struct node { struct node *next; char path[14]; };
struct dos_find_block {
    uint8_t reserved[0x15], attribute, reserved2[8];
    char filename[13];
};
struct menu_descriptor { uint16_t height, width; char attribute; char *text; };
struct config_panel { uint16_t x, y; uint8_t attributes; char *title; };
struct image_size_record { uint16_t size; uint8_t reserved[13]; };
struct file_field { char x, y, width; };
typedef void (*configuration_editor)(uint16_t x, uint16_t y);
extern struct node *head;
extern char readonly_search_spec[14];
extern char file_dialog_mode;
extern struct menu_descriptor help_page_data[5];
extern uint16_t help_menu_active, configuration_active;
extern struct config_panel configuration_panels[5];
extern uint8_t image_size_layout[7], fade_layout[6];
extern uint8_t auto_input_width, auto_field_positions[3];
extern struct file_field file_fields[3];
/* Shared objects: physical layout is declared in asm/dvdt.asm. */
extern uint16_t editable_image_count;
extern char directory_path[80];
extern ImageConfigRow * editable_images;
extern struct dos_find_block directory_find_data;
extern struct node * directory_tail;
extern uint16_t error_screen_backup[3][78];

#endif /* DAZZLE_H */
