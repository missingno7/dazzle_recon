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
extern int g_612a[4];
extern int g_0b4c[8][7];
extern unsigned int far dzfd_seg(void);
extern void wait_fade_tick(int, int);
extern int g_0a50[5][6];
extern int g_0a8c[8][4];
extern int g_0acc[8][8];
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
extern int g_6025;
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
extern unsigned g_5de8;
extern unsigned display_pan_state;
extern unsigned bios_resolution_mode;
extern unsigned int palette_fixed_mode;
extern unsigned int palette_alternate_mode;
extern unsigned hide_ega_border;
extern unsigned g_5ddc;
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
extern unsigned g_5dc4;
extern unsigned g_5dc2;
extern unsigned color_index_mask;
extern int muted;
extern unsigned window_mode;
extern unsigned int palette_entries;
extern unsigned g_5db8;
extern unsigned color_count;
extern unsigned use_bios_palette;
extern int screen_half_height;
extern int screen_half_width;
extern int g_5dae;
extern int g_5dac;
extern int ega_color_substep;
extern unsigned DAZZLE_SHARED auto_fade_pause_ticks;
extern unsigned palette_cycle_ticks;
extern unsigned int palette_step;
extern unsigned g_5da2;
extern unsigned semi_auto_mode;
extern unsigned int g_5d9e;
extern unsigned DAZZLE_SHARED alt_image_count;
extern unsigned DAZZLE_SHARED ctrl_image_count;
extern unsigned DAZZLE_SHARED fade_count;
extern unsigned char far *DAZZLE_SHARED palette_buffer;
extern unsigned int DAZZLE_SHARED current_graphics_key;
extern unsigned int DAZZLE_SHARED previous_graphics_key;
extern unsigned startup_active;
extern int g_5d8c;
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
extern unsigned g_5cbf;
extern unsigned g_5cbd;
extern int g_5cbb;
extern unsigned int mouse_callback_installed;
extern unsigned g_5cb7;
extern unsigned int mouse_available;
extern unsigned g_5cb3;
extern int DAZZLE_SHARED g_5cb1;
extern unsigned int draw_symmetry;
extern char DAZZLE_SHARED configuration_filename[80];
extern unsigned int far *current_image_header;
extern void far *g_5c57;
extern unsigned char far *g_5c53;
extern unsigned int g_5c51;
extern unsigned int g_5c4f;
extern ImageDrawRoutine current_image_draw;
extern void (far *palette_build_callback)(int);
extern void (far *DAZZLE_SHARED mode_setup_callback)(int);
extern void (far *plot_callback)(unsigned);
extern unsigned (far *read_pixel_callback)(int,int);
extern void (far *DAZZLE_SHARED palette_upload_callback)(unsigned char far *);
extern void (*display_start_callback)(unsigned, unsigned);
extern int g_5c31;
extern int g_5c2f;
extern int g_5c2d;
extern int g_5c2b;
extern int g_5c29;
extern int g_5c27;
extern int g_5c25;
extern int g_5c23;
extern int g_5c21;
extern int g_5c1f;
extern int g_5c1d;
extern int g_5c1b;
extern int g_5c19;
extern int g_5c17;
extern int g_5c15;
extern int g_5c13;
extern int far *g_5c0f;
extern int g_5c0d;
extern int g_5c0b;
extern int g_5c09;
extern int g_5c07;
extern int g_5c05;
extern int g_5c03;
extern int g_5c01;
extern int g_5bff;
extern int g_5bfd;
extern int g_5bfb;
extern int g_5bf9;
extern int g_5bf7;
extern int g_5bf5;
extern int g_5bf3;
extern int g_5bf1;
extern int g_5bef;
extern int g_5bed;
extern int g_5beb;
extern int g_5be9;
extern int g_5be7;
extern int g_5be5;
extern int g_5be3;
extern int g_5be1;
extern int g_5bdf;
extern int g_5bdd;
extern int g_5bdb;
extern int g_5bd9;
extern int g_5bd7;
extern int g_5bd5;
extern int g_5bd3;
extern int g_5bd1;
extern int g_5bcf;
extern float step_y;
extern float step_x;
extern float mod_y;
extern float mod_x;
extern float sine_phase;
extern float amplitude_y;
extern float amplitude_x;
extern int g_5bb1;
extern Point16 g_5ba5[3];
extern int g_5ba3;
extern int g_5ba1;
extern int g_5b9f;
extern char g_5b9e;
extern int g_5b9c;
extern int g_5b9a;
extern int g_5b98;
extern int g_5b96;
extern int g_5b94;
extern int g_5b92;
extern int g_5b90;
extern int g_5b8e;
extern int g_5b8c;
extern int g_5b8a;
extern int g_5b88;
extern int g_5b86;
extern int g_5b84;
extern int g_5b82;
extern int g_5b80;
extern int g_5b7e;
extern int g_5b7c;
extern int g_5b7a;
extern int g_5b78;
extern int g_5b76;
extern int g_5b74;
extern int g_5b72;
extern int g_5b70;
extern int g_5b6e;
extern int g_5b6c;
extern int g_5b6a;
extern unsigned int g_5b68;
extern unsigned int g_5b66;
extern int g_5b64;
extern int g_5b62;
extern int g_5b60;
extern int g_5b5e;
extern int g_5b5c;
extern int g_5b5a;
extern long g_long5b56;
extern long g_long5b52;
extern long g_long5b4e;
extern long g_long5b4a;
extern long g_long5b46;
extern long g_long5b42;
extern long g_long5b3e;
extern long g_long5b3a;
extern long g_long5b36;
extern int g_5b34;
extern int g_5b32;
extern int g_5b30;
extern int g_5b2e;
extern int g_5b2c;
extern int g_5b2a;
extern int g_5b28;
extern int g_5b26;
extern int g_5b24;
extern long g_5afc[10];
extern long g_5ad4[10];
extern long g_5ad0;
extern int g_5ace;
extern int g_5acc;
extern Point16 g_59c4[66];
extern int g_59c2;
extern int g_59c0;
extern int g_59be;
extern int g_59bc;
extern int g_59ba;
extern int g_59b8;
extern int g_59b6;
extern int g_59b4;
extern int g_59b2;
extern int g_59b0;
extern int g_59ae;
extern int g_59ac;
extern int g_59aa;
extern int g_59a8;
extern int g_59a6;
extern int g_59a4;
extern int g_59a2;
extern long g_599e;
extern int g_599c;
extern int g_599a;
extern int g_5998;
extern int g_5996;
extern int g_5994;
extern int g_5992;
extern int g_5990;
extern int g_598e;
extern int g_598c;
extern int g_598a;
extern int g_5988;
extern int g_5986;
extern int g_5984;
extern int g_5982;
extern int g_5980;
extern int g_597e;
extern int g_597c;
extern int g_597a;
extern int g_5978;
extern int g_5976;
extern int g_5974;
extern int g_5972;
extern int g_5970;
extern int g_596e;
extern int g_596c;
extern int g_596a;
extern int g_5968;
extern int g_5966;
extern int g_5964;
extern int g_5962;
extern float g_595e;
extern float g_595a;
extern float g_5956;
extern float g_5952;
extern float g_594e;
extern int g_594c;
extern unsigned int g_594a;
extern int g_5948;
extern int g_5946;
extern int g_5944;
extern int g_5942;
extern int g_5940;
extern int g_593e;
extern int g_593c;
extern int g_593a;
extern float g_5936;
extern StarTracePoint g_53be[175];
extern int g_53bc;
extern int g_53ba;
extern int g_53b8;
extern int g_53b6;
extern int g_53b4;
extern int g_53b2;
extern int g_53b0;
extern int g_53ae;
extern int g_53ac;
extern int g_53aa;
extern int g_53a8;
extern int g_53a6;
extern int g_53a4;
extern int g_53a2;
extern int g_53a0;
extern int g_539e;
extern int g_539c;
extern int g_539a;
extern unsigned int g_5398;
extern unsigned int g_5396;
extern unsigned int g_5394;
extern unsigned int g_5392;
extern unsigned int g_5390;
extern unsigned int g_538e;
extern unsigned int g_538c;
extern unsigned int g_538a;
extern int g_5388;
extern int g_5386;
extern int g_5384;
extern int g_5382;
extern int g_5380;
extern int g_537e;
extern unsigned int g_537c;
extern unsigned int g_537a;
extern int g_5378;
extern int g_5376;
extern int g_5374;
extern int g_5372;
extern int g_5370;
extern int g_536e;
extern int g_536c;
extern int g_536a;
extern int g_5368;
extern int g_5366;
extern int g_5364;
extern int g_5362;
extern int g_5360;
extern int g_535e;
extern int g_535c;
extern int ellipse_direction_steps[700];
extern int g_4de2;
extern int g_4de0;
extern int g_4dde;
extern int ellipse_first_region;
extern long g_4dd8;
extern long g_4dd4;
extern long g_4dd0;
extern long g_4dcc;
extern long g_4dc8;
extern long g_4dc4;
extern long g_4dc0;
extern long g_4dbc;
extern long g_4db8;
extern int g_4db6;
extern int g_4db4;
extern int g_4db2;
extern int g_4db0;
extern int g_4dae;
extern int polygon_row_right[350];
extern int polygon_row_left[350];
extern int error;
extern int sy;
extern int sx;
extern int dy;
extern int dx;
extern int g_7436;
extern int g_740c[21];
extern unsigned int g_73e2[21];
extern int g_73b8[21];
extern unsigned int g_738e[21];
extern int g_736c[17];
extern int g_7362[5];
extern int g_7358[5];
extern int g_7356;
extern int g_7354;
extern int g_7352;
extern int g_7350;
extern int g_734e;
extern int g_734c;
extern int g_734a;
extern int g_7348;
extern float g_7344;
extern transition_particle_t g_6144[4][64];
extern int g_6142;
extern int g_6140;
extern int g_613e;
extern int g_613c;
extern int g_613a;
extern int g_6138;
extern int g_6130[4];
extern int g_6128[4];
extern int g_6120[4];
extern int g_6118[4];
extern int g_6110[4];
extern int g_6108[4];
extern int g_6100[4];
extern int g_60f8[4];
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
#define g_5d8b (((unsigned char near *)&background_fill_word)[1])
#define bios_key_scan_code (((unsigned char near *)&current_graphics_key)[1])
#define g_5d94 (((unsigned near *)&palette_buffer)[0])
#define g_5d96 (((unsigned near *)&palette_buffer)[1])
#define g_7456 (((unsigned near *)&image_source_bytes)[0])
#define image_source_offset (((unsigned near *)&image_source_bytes)[0])
#define image_file_position (*(unsigned long near *)&resource_base)
#define image_stream (*(void far * near *)&selfcheck_stream)
#define far_callback (*(void (far * near *)(unsigned))&mode_setup_callback)
#define original_text_attribute (saved_text_info.attribute)
#define standard_error_stream (_streams[2])
#define standard_output_stream (_streams[1])
#define g_0479 (*(int near *)&fade_records[0].enabled)
#define g_0480 (*(int near *)&fade_records[1].enabled)
#define g_0487 (*(int near *)&fade_records[2].enabled)
#define g_048e (*(int near *)&fade_records[3].enabled)
#define g_0495 (*(int near *)&fade_records[4].enabled)
#define g_049c (*(int near *)&fade_records[5].enabled)
#define g_04a3 (*(int near *)&fade_records[6].enabled)
#define g_04aa (*(int near *)&fade_records[7].enabled)
#define g_04b1 (*(int near *)&fade_records[8].enabled)
#define g_04b8 (*(int near *)&fade_records[9].enabled)
#define g_04bf (*(int near *)&fade_records[10].enabled)
#define g_04c6 (*(int near *)&fade_records[11].enabled)
#define g_04cd (*(int near *)&fade_records[12].enabled)
#define g_04d4 (*(int near *)&fade_records[13].enabled)
#define g_04db (*(int near *)&fade_records[14].enabled)
#define g_04e2 (*(int near *)&fade_records[15].enabled)
#define g_04e9 (*(int near *)&fade_records[16].enabled)
#define g_04f0 (*(int near *)&fade_records[17].enabled)
#define g_04f7 (*(int near *)&fade_records[18].enabled)
#define g_04fe (*(int near *)&fade_records[19].enabled)
#define g_0505 (*(int near *)&fade_records[20].enabled)
#define g_050c (*(int near *)&fade_records[21].enabled)
#define g_0513 (*(int near *)&fade_records[22].enabled)
#define g_051a (*(int near *)&fade_records[23].enabled)

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
