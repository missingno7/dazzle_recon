/* Shared DAZZLE types, state and module interfaces. BC++ 3.0 large model. */
#ifndef DAZZLE_H
#define DAZZLE_H

/* Menus deliberately uses far accesses to shared DGROUP objects. This
 * qualifies the access in that TU; it does not allocate duplicate storage. */
#ifdef DAZZLE_MENU
#define DAZZLE_SHARED far
#define _streams unused_near_streams
#define _osmajor unused_near_osmajor
#else
#define DAZZLE_SHARED
#endif

#include <dos.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <dir.h>
#include <signal.h>
#include <bios.h>
#ifdef DAZZLE_MENU
#undef _streams
#undef _osmajor
extern FILE far _streams[];
extern unsigned char far _osmajor;
#endif

typedef struct { int x,y; } Point16;
typedef struct { int dx[3],dy[3]; } ConeDirections;
typedef struct { int x1,y1,x2,y2; } StarTracePoint;
typedef struct { float cosine,sine; } TrigPair;
typedef void (far *ImageDrawRoutine)(int);
typedef struct {
    ImageDrawRoutine draw;
    unsigned size;
    char far *name;
    unsigned enabled, continuous;
    unsigned char key;
} ImageConfigRow;
typedef struct {
    unsigned char key;
    char far *name;
    unsigned enabled;
} FadeConfigRow;

extern unsigned video_adapter;
extern unsigned plot_color;
extern void cycle_palette(void);
extern int poll_graphics_keys(void);
extern void far ega_plot_symmetric_640x700(unsigned);
extern void restore_text_attributes(void);
extern void restore_runtime_callbacks(void);
extern void signal_diagnostic(int signal_number, int signal_type, unsigned int far *registers);
extern void initialize_program_defaults(void);
extern void save_video_state(void);
extern void detect_video_adapter(void);
extern int parse_command_line(int argc, char far * far *argv, int far *option_position);
extern void print_startup_information(void);
extern unsigned int keyboard_led_flags(unsigned int mask, unsigned int mode);
extern void fatal_selfcheck_error(int show_crc, const char far *reason);
extern void check_executable_integrity(void);
extern void configure_video_backend(int allocate);
extern int resolve_startup_configuration(int requested);
extern void install_runtime_callbacks(void);
extern void beep(unsigned hz, unsigned ms);
extern void park_disk_heads(void);
extern void prepare_logo_image_resources(void);
extern void semi_auto_loop(void);
extern void set_ega_raster_xor(int mode);
extern void closing_graphic_and_music(void);
extern void restore_video_state(int);
extern void print_closing_text_and_exit(int status);

extern void butterflies(int);
extern void diamonds(int);
extern void gumballs(int);
extern void tiles(int);
extern void wedges(int);
extern void yarnball(int);

extern unsigned _stklen;
extern char far *executable_name_pointer;
extern char far *help_program_name;
extern char far *program_revision;
extern char far *registration_notice_pointer;
extern int direction_x[4], direction_y[4];
extern ConeDirections cone_directions[4];
extern unsigned char grid_patterns[256];
extern int diamond_x[4], diamond_y[4];
extern int zigstream_dx[2], zigstream_dy[2];
extern ImageConfigRow DAZZLE_SHARED alt_image_records[36];
extern ImageConfigRow DAZZLE_SHARED ctrl_image_records[6];
extern FadeConfigRow DAZZLE_SHARED fade_records[24];

extern unsigned dzle_seg(void);
extern int line_step(int endx,int endy,int initialize);
extern void choose_color(int kind);
extern void square(int n,int symmetry);
extern void draw_polygon(int rebuild,int n,int far *px,int far *py,unsigned fill_color,unsigned edge_color,unsigned flags);
extern int ellipse_step(int radius_y,int radius_x,int initialize);
extern void draw_ellipse(int rebuild,int ry,int rx,int cx,int cy,unsigned fill,unsigned edge,unsigned flags);
extern void swap_pixel_rectangles(int flip,int x1,int y1,int x2,int y2,int width,int height);
extern void arches(int rebuild);
extern void atom(int rebuild);
extern void balls(int rebuild);
extern void bars(int rebuild);
extern void bigstar(int rebuild);
extern void boxes(int rebuild);
extern void cones(int rebuild);
extern void corkscrew(int rebuild);
extern void curves(int rebuild);
extern void cycloids(int rebuild);
extern void daisies(int rebuild);
extern void diag1(int size);
extern void diag2(int size);
extern void ellipse(int size);
extern void formula1(int size);
extern void grid(int size);
extern void hexagon(int size);
extern void hilbert(int size);
extern void lines(int size);
extern void littleStars(void);
extern void lotsadots(int size);
extern void pedals(int size);
extern void puffs(int size);
extern void ribbons(int size);
extern void sierpinski(int size);
extern void sine(int rebuild);
extern void squares(int size);
extern void squirals(int size);
extern void spiral(int size);
extern void spiral1(int size);
extern void sweep(int size);
extern void sweep2(int size);
extern void triangles(int size);
extern void tunnel(int size);
extern int wandering(int size);
extern void zigstream(int size);
extern void main(int argc, char far * far *argv);

/* DOSI shared state and interfaces. */
typedef struct {
    unsigned width, height;
    unsigned char far *pixels;
} PaletteMapSpec;
typedef void (far *VideoCallback)(void);
typedef unsigned (far *ReadPixelCallback)(int, int);
typedef void (far *PaletteCallback)(unsigned char far *);

extern unsigned int far *bios_keyboard_flags;
extern unsigned long crc_polynomial;
extern unsigned int display_freeze_active;
extern unsigned int display_freeze_cycle_requested;
extern unsigned int menu_active;
extern unsigned int palette_callback_active;
extern float tone_base_frequency;
extern unsigned tone_chord[4];
extern unsigned force_ega;
extern unsigned force_vga;
extern unsigned int far dzl2_seg(void);
extern unsigned int far dzfd_seg(void);
extern unsigned int far dvdt_seg(void);
extern unsigned int far dhuf_seg(void);
extern unsigned int far dlogo_seg(void);
extern unsigned int far dazpcx_seg(void);
extern void far menu_error_message(char far *);
extern void far upload_reserved_dac_entries(unsigned int);
extern void far help_screen(void);
extern void far configuration_screen(void);
extern void far save_pcx(void);
extern void far ega_set_mode(unsigned);
extern void far ega_plot_symmetric_640x350(void);
extern void far draw_closing_graphic(void);
extern void far set_display_start(unsigned, unsigned);
extern void far vga_set_mode_320x200(unsigned);
extern void far vga_set_mode_320x400(unsigned);
extern void far vga_plot_symmetric_320x200(unsigned);
extern void far vga_plot_symmetric_320x400(unsigned);
extern unsigned far vga_read_pixel_320x200(int, int);
extern unsigned far vga_read_pixel_320x400(int, int);
extern unsigned far ega_read_pixel_640x350(int, int);
extern unsigned far ega_read_pixel_640x700(int, int);
extern void far vga_upload_palette_bios_split(unsigned, unsigned);
extern void far vga_upload_palette_8086_split(unsigned, unsigned);
extern void far vga_upload_palette_rep_split(unsigned, unsigned);
extern void far vga_upload_palette_bios(unsigned, unsigned);
extern void far vga_upload_palette_8086(unsigned, unsigned);
extern void far vga_upload_palette_rep(unsigned, unsigned);
extern void far ega_upload_palette_bios(unsigned, unsigned);
extern void far ega_upload_palette_ports(unsigned, unsigned);
extern void far extract_manual(void);
extern int far cpu_has_post_8086_shifts(void);
extern void far display_boundary_fade_and_pan(void);
extern void mode_ready_chirp(void);
extern int default_signal_result(void);
extern int interrupt_signal_result(void);
struct exception;
extern int matherr(struct exception far *frame);
extern int probe_mouse(unsigned int mask);
extern void mouse_position(int buttons, int x, int y);
extern void read_mz_resource_base(FILE far *input);
extern void *allocate(unsigned *amount, unsigned minimum);
extern unsigned long genCRC(FILE far *input, long length);
extern int deRead(void far *metadata, unsigned int size);
extern void print_order_info(int show_order_form);
extern void command_line_help(void);
extern int reject_exec_filename(char far *filename);
extern void generate_palette(void);
extern void randomize_palette_state(unsigned int replace_base_colors);
extern void change_background_palette(void);
extern void build_ega_palette(int update_cycle);
extern int freeze_display(unsigned int operation);
extern void initialize_math_tables(void);
extern void far initialize_palette_mapping(PaletteMapSpec far *mapping);
extern void beep_float(float hz, unsigned ms);
extern void play_tone_sequence(void);
extern int parse_config_version(char far *line);
extern void parse_alt_image_size(char far *line);
extern void parse_ctrl_image_size(char far *line);
extern void parse_fade_enabled(char far *line);
extern void parse_auto_configuration(char far *line);
extern int load_configuration_file(char far *path);
extern int save_configuration_file(char far *path);
extern void auto_loop(void);
extern void interrupt far graphics_timer_tick_isr(void);
extern void interrupt far keyboard_pending_isr(void);
extern void far mouse_pending_callback(void);

/* SEG0CCD interfaces. */
typedef struct {
    int head_x, head_y;
    int tail_x, tail_y;
    int velocity_x, velocity_y;
    int tail_start, tail_end;
    int unknown_10; /* Not accessed by this routine. */
} transition_particle_t;

typedef union {
    long value;
    struct { unsigned int fraction; int whole; } part;
} transition_point_t;

typedef union {
    unsigned long value;
    struct { unsigned int fraction; int whole; } part;
} spiral_point_t;

struct LineWipePoint {
    unsigned int x0, y0, x1, y1;
};

union StarburstFixed {
    long value;
    struct {
        unsigned int fraction;
        int integer;
    } word;
};

struct StarburstState {
    union StarburstFixed x;
    union StarburstFixed y;
    union StarburstFixed dx;
    union StarburstFixed dy;
    unsigned int color;
};

extern unsigned int page_paragraph;
extern int fireworks_unused_active_view[4];
extern int moving_bars_patterns[8][7];
extern unsigned int far dzfd_seg(void);
extern void wait_fade_tick(int, int);
extern int countdown_digit_segments[5][6];
extern int countdown_segment_bounds[8][4];
extern int ziti_sweep_patterns[8][8];
extern void ega_set_line_compare(unsigned int);
extern void vga_set_line_compare(unsigned int);
/* High byte of background_fill_word (DS:5D8A). */
extern void randomize_transition(void);
extern void drip_transition(void);
extern void firefly_transition(int mode);
extern void gas_cloud(void);
extern void spiral_transition(void);
extern void random_delay_transition(void);
extern void mesh_transition(void);
extern void cleanup_transition(void);
extern void quilt_transition(void);
extern int dispatch_fade_key(void);
extern void animate_vertical_display_start(int, int);
extern void animate_line_compare_split(int reverse);
extern void clear_vram_words(unsigned int, unsigned int, unsigned int);
extern void seed_plasma(void);
extern void randomize_helper(int);
/* fade_timer_ticks is the volatile view of countdown_ticks (DS:5D78).
 * The shared declaration needs reconciliation by its owner. */

/* SEG17F5 interfaces. */
extern unsigned far strlen(const char far *text);
extern char near *default_open_path;
extern char near *default_open_mode;

extern unsigned logo_input_buffer_size;
struct image_header {
    char reserved[0x42];
    int output_size;
};
extern unsigned logo_pending_output_offset;
extern unsigned video_adapter;
extern unsigned logo_restore_video_adapter;
extern void far *malloc(unsigned size);
extern void far free(void far *block);
extern void far vga_set_mode_320x200(unsigned mode);
extern void far ega_set_mode(unsigned mode);
extern void far ega_init_row_table_and_mode(void);
extern void far ega_write_planar_row(unsigned char far *source, unsigned width,
                                     unsigned frame);
extern void far configure_logo_dac(void);
extern void far rgb8_to_ega_palette(unsigned char far *destination,
                               unsigned char far *source, unsigned mask);

extern void far setmem(void far *destination, unsigned count, char value);

/* BSS first declarations, in reverse link-address order. */
extern unsigned image_data_offset;
extern unsigned int serial_r2;
extern unsigned int serial_r1;
extern unsigned long expected_crc;
extern int effect_parameter;
extern unsigned long computed_crc;
extern struct text_info saved_text_info;
extern union REGS DAZZLE_SHARED keyboard_regs;
extern FILE far *selfcheck_stream;
extern FILE far *configuration_file_stream;
extern char far *DAZZLE_SHARED default_config_name;
extern void (interrupt far *saved_timer_vector)(void);
extern void (interrupt far *saved_keyboard_vector)(void);
extern TrigPair trig_table[64];
extern int draw_x;
extern int draw_y;
extern unsigned DAZZLE_SHARED palette_cycle_offset;
extern unsigned int original_ctrlbreak_setting;
extern unsigned palette_buffer_bytes;
extern unsigned display_pan_state;
extern unsigned bios_resolution_mode;
extern unsigned int palette_fixed_mode;
extern unsigned int palette_alternate_mode;
extern unsigned hide_ega_border;
extern unsigned option_y_requested;
extern unsigned int palette_random_mode;
extern unsigned split_vga_palette_updates;
extern unsigned unused_palette_state;
extern unsigned use_8086_palette_io;
extern unsigned keep_typematic;
extern unsigned int palette_four_ega_colors;
extern unsigned show_mode;
extern unsigned no_disk_park;
extern unsigned bashful_mode;
extern unsigned insecure_argument_index;
extern unsigned draw_delay_count;
extern unsigned targa64_mode;
extern unsigned option_plus_requested;
extern unsigned color_index_mask;
extern int muted;
extern unsigned window_mode;
extern unsigned int palette_entries;
extern unsigned palette_cycle_wrap_bytes;
extern unsigned color_count;
extern unsigned use_bios_palette;
extern int screen_half_height;
extern int screen_half_width;
extern int screen_extent;
extern int rectangles_angle_index;
extern int ega_color_substep;
extern unsigned DAZZLE_SHARED auto_fade_pause_ticks;
extern unsigned palette_cycle_ticks;
extern unsigned int palette_step;
extern unsigned default_palette_cycle_ticks;
extern unsigned semi_auto_mode;
extern unsigned int semi_auto_continuous;
extern unsigned DAZZLE_SHARED alt_image_count;
extern unsigned DAZZLE_SHARED ctrl_image_count;
extern unsigned DAZZLE_SHARED fade_count;
extern unsigned char far *DAZZLE_SHARED palette_buffer;
extern unsigned int DAZZLE_SHARED current_graphics_key;
extern unsigned int DAZZLE_SHARED previous_graphics_key;
extern unsigned startup_active;
extern int ega_border_color;
extern unsigned background_fill_word;
extern unsigned DAZZLE_SHARED maximum_image_size;
extern unsigned show_duration_ticks;
extern unsigned startup_config_failure;
extern unsigned int DAZZLE_SHARED freeze_led_state;
extern unsigned DAZZLE_SHARED exit_requested;
extern unsigned DAZZLE_SHARED input_pending;
extern unsigned DAZZLE_SHARED mouse_abort_pending;
extern unsigned palette_elapsed_ticks;
extern unsigned countdown_ticks;
extern unsigned unused_input_state;
extern unsigned int timer_hook_installed;
extern unsigned int keyboard_hook_installed;
extern unsigned int skip_ctrlbreak_control;
extern unsigned palette_base_blue;
extern unsigned palette_base_green;
extern long resource_base;
extern unsigned char unused_path_state;
extern char near executable_path[80];
extern char startup_directory[80];
extern unsigned DAZZLE_SHARED minimum_images_per_display;
extern unsigned DAZZLE_SHARED maximum_images_per_display;
extern unsigned current_image_index;
extern unsigned diagonal_symmetry_selected;
extern unsigned image_work_limit;
extern int image_work_count;
extern unsigned int mouse_callback_installed;
extern unsigned option_bang_requested;
extern unsigned int mouse_available;
extern unsigned image_xor_mode;
extern int DAZZLE_SHARED display_cleared;
extern unsigned int draw_symmetry;
extern char DAZZLE_SHARED configuration_filename[80];
extern unsigned int far *current_image_header;
extern void far *linewipe_history;
extern unsigned char far *ball_bitmap_pixels;
extern unsigned int ball_bitmap_width;
extern unsigned int ball_bitmap_height;
extern ImageDrawRoutine current_image_draw;
extern void (far *palette_build_callback)(int);
extern void (far *DAZZLE_SHARED mode_setup_callback)(int);
extern void (far *plot_callback)(unsigned);
extern unsigned (far *read_pixel_callback)(int,int);
extern void (far *DAZZLE_SHARED palette_upload_callback)(unsigned char far *);
extern void (*display_start_callback)(unsigned, unsigned);
extern int zigstream_step_y;
extern int zigstream_step_x;
extern int zigstream_direction;
extern int zigstream_color_phase;
extern int zigstream_run_length;
extern int zigstream_run_counter;
extern int wandering_line_done;
extern int wandering_end_dy;
extern int wandering_start_dy;
extern int wandering_end_dx;
extern int wandering_start_dx;
extern int wandering_segments_left;
extern int wandering_end_y;
extern int wandering_end_x;
extern int wandering_start_y;
extern int wandering_start_x;
extern int far *tunnel_right_edge_ptr;
extern int tunnel_right_limit;
extern int tunnel_rows_left;
extern int tunnel_row_left;
extern int triangles_expand_left;
extern int triangles_right_limit;
extern int triangles_active;
extern int triangles_row_left;
extern int triangles_row_right;
extern int sweep2_line_done;
extern int sweep2_end_x_step;
extern int sweep2_end_x;
extern int sweep2_end_y;
extern int sweep_rise_interval;
extern int sweep_rise_counter;
extern int spiral1_side_limit;
extern int spiral1_step_y;
extern int spiral1_step_x;
extern int spiral1_direction;
extern int spiral1_side_length;
extern int spiral1_side_step;
extern int spiral_step_y;
extern int spiral_step_x;
extern int spiral_direction;
extern int spiral_color_phase;
extern int spiral_leg_length;
extern int spiral_leg_step;
extern int squares_side_limit;
extern int squares_step_y;
extern int squares_step_x;
extern int squares_direction;
extern int squares_side_length;
extern int squares_side_step;
extern float step_y;
extern float step_x;
extern float mod_y;
extern float mod_x;
extern float sine_phase;
extern float amplitude_y;
extern float amplitude_x;
extern int sierpinski_fixed_color;
extern Point16 sierpinski_vertices[3];
extern int ribbons_run_limit;
extern int ribbons_run_length;
extern int ribbons_run_step;
extern char puffs_drift_step;
extern int puffs_line_done;
extern int puffs_spread;
extern int puffs_end_y;
extern int puffs_end_x;
extern int puffs_origin_y;
extern int puffs_origin_x;
extern int pedals_stroke_index;
extern int pedals_phase;
extern int pedals_start_radius;
extern int pedals_radius;
extern int lines_base_y;
extern int lines_step_y;
extern int lines_step_x;
extern int lines_stroke_index;
extern int hilbert_restart;
extern int hilbert_stroke_width;
extern int hilbert_color_per_point;
extern int hilbert_step_y;
extern int hilbert_step_x;
extern int hilbert_depth;
extern int hilbert_target_y;
extern int hilbert_target_x;
extern int hilbert_previous_y;
extern int hilbert_previous_x;
extern int hexagon_start_x;
extern int hexagon_vertical_limit;
extern unsigned int grid_next_y;
extern unsigned int grid_next_x;
extern int grid_pattern_index;
extern int grid_tile_left;
extern int grid_tile_right;
extern int formula1_color_shift;
extern int formula1_y_square_factor;
extern int formula1_x_shift;
extern long g_long5b56;
extern long g_long5b52;
extern long g_long5b4e;
extern long g_long5b4a;
extern long g_long5b46;
extern long g_long5b42;
extern long g_long5b3e;
extern long g_long5b3a;
extern long g_long5b36;
extern int ellipse_center_y;
extern int ellipse_center_x;
extern int ellipse_offset_y;
extern int ellipse_offset_x;
extern int ellipse_draw_mode;
extern int ellipse_region;
extern int ellipse_line_done;
extern int diag2_color_period;
extern int diag2_color_counter;
extern long daisies_sine_table[10];
extern long daisies_cosine_table[10];
extern long daisies_aspect_scale;
extern int daisies_sample_limit;
extern int daisies_sample_start;
extern Point16 daisies_edge_points[66];
extern int daisies_base_color;
extern int daisies_color_per_ray;
extern int daisies_sample_step;
extern int daisies_radius_step;
extern int daisies_radius;
extern int daisies_arc_variant;
extern int daisies_trig_index;
extern int daisies_work_index;
extern int daisies_edge_count;
extern int daisies_line_done;
extern int daisies_sample_index;
extern int daisies_center_y;
extern int daisies_center_x;
extern int daisies_edge_end_y;
extern int daisies_edge_end_x;
extern int daisies_saved_y;
extern int daisies_saved_x;
extern long cycloids_aspect_scale;
extern int cycloids_next_curve;
extern int cycloids_curve_index;
extern int cycloids_line_done;
extern int cycloids_pen_y;
extern int cycloids_pen_x;
extern int cycloids_center_y;
extern int cycloids_center_x;
extern int cycloids_spin_step;
extern int cycloids_orbit_step;
extern int cycloids_phase_advance;
extern int cycloids_pen_radius_step;
extern int cycloids_color_per_point;
extern int cycloids_curve_limit;
extern int cycloids_orbit_radius;
extern int cycloids_pen_radius;
extern int cycloids_rolling_radius;
extern int cycloids_outer_radius;
extern int cycloids_pen_phase;
extern int cycloids_spin_phase;
extern int cycloids_orbit_phase;
extern int cycloids_start_phase;
extern int curves_amplitude;
extern int curves_phase_step_y;
extern int curves_phase_step_x;
extern int curves_phase_y;
extern int curves_phase_x;
extern int curves_stroke_index;
extern int corkscrew_color_counter;
extern int corkscrew_origin_y;
extern int corkscrew_origin_x;
extern float corkscrew_phase;
extern float corkscrew_growth_step;
extern float corkscrew_distance;
extern float corkscrew_across;
extern float corkscrew_along;
extern int boxes_restart;
extern unsigned int boxes_edge_progress;
extern int boxes_edge_state;
extern int boxes_line_done;
extern int boxes_target_y;
extern int boxes_target_x;
extern int boxes_height;
extern int boxes_width;
extern int boxes_origin_y;
extern int boxes_origin_x;
extern float bigstar_inner_radius;
extern StarTracePoint bigstar_traces[175];
extern int bigstar_base_color;
extern int bigstar_use_steep_trace;
extern int bigstar_trace_index;
extern int bigstar_shallow_count;
extern int bigstar_steep_count;
extern int bigstar_color_mode;
extern int bigstar_spacing_shift;
extern int bigstar_trace_step;
extern int bigstar_resize_pending;
extern int bigstar_segment_state;
extern int bigstar_restart;
extern int bigstar_line_done;
extern int bigstar_target_y;
extern int bigstar_target_x;
extern int bigstar_diagonal_y;
extern int bigstar_diagonal_x;
extern int bigstar_axis_y;
extern int bigstar_axis_x;
extern unsigned int bars_line_done;
extern unsigned int bars_span_y;
extern unsigned int bars_span_x;
extern unsigned int bars_end_y;
extern unsigned int bars_end_x;
extern unsigned int bars_start_y;
extern unsigned int bars_start_x;
extern unsigned int balls_direction;
extern int balls_step_y;
extern int balls_step_x;
extern int balls_run_length;
extern int balls_run_progress;
extern int balls_origin_y;
extern int balls_origin_x;
extern unsigned int balls_sample_x;
extern unsigned int balls_sample_y;
extern int atom_stroke_index;
extern int atom_phase;
extern int atom_radius;
extern int arches_second_radius_x;
extern int arches_second_radius_y;
extern int arches_second_center_x;
extern int arches_first_radius_x;
extern int arches_first_radius_y;
extern int arches_first_center_y;
extern int ellipse_trace_limit;
extern int ellipse_fill_color;
extern int ellipse_fill_mode;
extern int ellipse_fill_min_x;
extern int ellipse_fill_x;
extern int ellipse_trace_done;
extern int ellipse_direction_steps[700];
extern int ellipse_trace_y;
extern int ellipse_trace_x;
extern int ellipse_trace_index;
extern int ellipse_first_region;
extern long ellipse_step_gradient_y;
extern long ellipse_step_gradient_x;
extern long ellipse_step_error;
extern long ellipse_step_twice_y_sq;
extern long ellipse_step_twice_x_sq;
extern long ellipse_step_radius_y_sq;
extern long ellipse_step_radius_x_sq;
extern long ellipse_step_radius_y;
extern long ellipse_step_radius_x;
extern int polygon_row_limit;
extern int polygon_origin_y;
extern int polygon_origin_x;
extern int polygon_min_y;
extern int polygon_max_y;
extern int polygon_row_right[350];
extern int polygon_row_left[350];
extern int error;
extern int sy;
extern int sx;
extern int dy;
extern int dx;
extern int saved_pan_draw_delay;
extern int tetris_source_y[21];
extern unsigned int tetris_block_colors[21];
extern int tetris_block_y[21];
extern unsigned int tetris_column_active[21];
extern int firefly_path_state[17];
extern int firefly_y_history[5];
extern int firefly_x_history[5];
extern int quilt_tile_height;
extern int quilt_tile_width;
extern int quilt_second_y;
extern int quilt_second_x;
extern int quilt_first_y;
extern int quilt_first_x;
extern int quilt_flip_mode;
extern int quilt_grid_divisions;
extern float fireworks_aspect_ratio;
extern transition_particle_t fireworks_particles[4][64];
extern int fireworks_displaced_origins;
extern int fireworks_background_x_mask;
extern int fireworks_trail_mode;
extern int fireworks_background_b;
extern int fireworks_background_a;
extern int fireworks_acceleration;
extern int fireworks_gravity[4];
extern int fireworks_active[4];
extern int fireworks_launch_delay[4];
extern int fireworks_colors[4];
extern int fireworks_finished_particles[4];
extern int fireworks_age[4];
extern int fireworks_origin_y[4];
extern int fireworks_origin_x[4];
/* Unreferenced allocation between the mouse REGPACK and logo state.
 * Four bytes are independently bounded; original name/type/owner are unknown. */
extern unsigned char reserved_program_state[4];
extern struct REGPACK mouse_regs;
extern unsigned int ega_red_limit;
extern unsigned int ega_blue_limit;
extern unsigned int ega_green_limit;
extern struct image_header far *logo_image_header;
extern unsigned logo_image_height;
extern unsigned logo_previous_aspect;
extern unsigned logo_previous_video_adapter;
extern unsigned logo_frame_state[2];
extern unsigned image_frame_count;
extern unsigned logo_input_buffer_index;
extern unsigned logo_input_buffer_offset;
extern unsigned logo_decode_state;
extern unsigned image_start_offset;
extern unsigned image_frame_size;
extern unsigned logo_plane_state[2];
extern char far *logo_input_buffers[2];
extern unsigned long image_source_bytes;

/* Typed views of shared fields; no duplicate storage. */
#define background_alt_color (((unsigned char near *)&background_fill_word)[1])
#define bios_key_scan_code (((unsigned char near *)&current_graphics_key)[1])
#define palette_pointer_offset (((unsigned near *)&palette_buffer)[0])
#define palette_pointer_segment (((unsigned near *)&palette_buffer)[1])
#define image_source_bytes_low (((unsigned near *)&image_source_bytes)[0])
#define image_source_offset (((unsigned near *)&image_source_bytes)[0])
#define image_file_position (*(unsigned long near *)&resource_base)
#define image_stream (*(void far * near *)&selfcheck_stream)
#define far_callback (*(void (far * near *)(unsigned))&mode_setup_callback)
#define original_text_attribute (saved_text_info.attribute)
#define standard_error_stream (_streams[2])
#define standard_output_stream (_streams[1])
#define fade_randomize_enabled (*(int near *)&fade_records[0].enabled)
#define fade_blister_enabled (*(int near *)&fade_records[1].enabled)
#define fade_countdown_enabled (*(int near *)&fade_records[2].enabled)
#define fade_drip_enabled (*(int near *)&fade_records[3].enabled)
#define fade_exclusive_or_enabled (*(int near *)&fade_records[4].enabled)
#define fade_firefly_enabled (*(int near *)&fade_records[5].enabled)
#define fade_gas_cloud_enabled (*(int near *)&fade_records[6].enabled)
#define fade_spiral_enabled (*(int near *)&fade_records[7].enabled)
#define fade_juggle_palette_enabled (*(int near *)&fade_records[8].enabled)
#define fade_kleenup_enabled (*(int near *)&fade_records[9].enabled)
#define fade_linewipe_enabled (*(int near *)&fade_records[10].enabled)
#define fade_mesh_enabled (*(int near *)&fade_records[11].enabled)
#define fade_fireworks_enabled (*(int near *)&fade_records[12].enabled)
#define fade_pan_scroll_enabled (*(int near *)&fade_records[13].enabled)
#define fade_quilt_enabled (*(int near *)&fade_records[14].enabled)
#define fade_rectangles_enabled (*(int near *)&fade_records[15].enabled)
#define fade_starburst_enabled (*(int near *)&fade_records[16].enabled)
#define fade_twist_enabled (*(int near *)&fade_records[17].enabled)
#define fade_umbra_enabled (*(int near *)&fade_records[18].enabled)
#define fade_moving_bars_enabled (*(int near *)&fade_records[19].enabled)
#define fade_wipe_enabled (*(int near *)&fade_records[20].enabled)
#define fade_hexagon_enabled (*(int near *)&fade_records[21].enabled)
#define fade_tetris_enabled (*(int near *)&fade_records[22].enabled)
#define fade_ziti_enabled (*(int near *)&fade_records[23].enabled)

#define fade_timer_ticks (*(volatile unsigned near *)&countdown_ticks)

/* Menu descriptors and directory workspace. */
struct node { struct node far *next; char path[14]; };
struct dos_find_block {
    unsigned char reserved[0x15], attribute, reserved2[8];
    char filename[13];
};
struct menu_descriptor { unsigned height, width; char attribute; char far *text; };
struct config_panel { unsigned x, y; unsigned char attributes; char far *title; };
struct image_size_record { unsigned size; unsigned char reserved[13]; };
struct file_field { char x, y, width; };
typedef void (far *configuration_editor)(unsigned x, unsigned y);
extern struct node far *head;
extern char near readonly_search_spec[14];
extern char near file_dialog_mode;
extern struct menu_descriptor help_page_data[5];
extern unsigned help_menu_active, configuration_active;
extern struct config_panel configuration_panels[5];
extern unsigned char near image_size_layout[7], fade_layout[6];
extern unsigned char near auto_input_width, auto_field_positions[3];
extern struct file_field file_fields[3];
/* Shared far objects: physical layout is declared in asm/dvdt.asm. */
extern unsigned far editable_image_count;
extern char far directory_path[80];
extern ImageConfigRow far * far editable_images;
extern struct dos_find_block far directory_find_data;
extern struct node far * far directory_tail;
extern unsigned far error_screen_backup[3][78];
#ifdef DAZZLE_MENU
extern int far tolower(int);
#endif

#endif /* DAZZLE_H */
