#include <dir.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

typedef void (far *ImageDrawRoutine)(unsigned);

/* objcmp: {"function":"initialize_math_tables","address":"11c1:1d64","size":113,"symbols":{"_g_trig_table":"DS:5df2","_cos":"0000:0f6e","_sin":"0000:0e75"},"segments":{"_DATA":"DS:2007"},"absolute_symbols":{"FIDRQQ":"0x5c32","FIWRQQ":"0xa23d"}} */
extern float g_trig_table[128];
extern double far cos(double), far sin(double);

void initialize_math_tables(void) {
    float angle;
    register int i;

    angle = 0.0;
    for (i = 0; i < 64; ++i) {
        g_trig_table[i * 2] = cos(angle);
        g_trig_table[i * 2 + 1] = sin(angle);
        angle += 0.098174770421875;
    }
}

/* objcmp: {"function":"initialize_palette_mapping","address":"11c1:1dd5","size":340,"symbols":{"_g_5db6":"DS:5db6","_sqrt":"0000:0e33","F_FTOL@":"0000:0fbe"},"segments":{"_DATA":"DS:2007"},"absolute_symbols":{"FIDRQQ":"0x5c32","FIWRQQ":"0xa23d"}} */
typedef struct {
    unsigned width;
    unsigned height;
    unsigned char far *pixels;
} PaletteMapSpec;
extern double far sqrt(double);
extern unsigned g_5db6;

void far initialize_palette_mapping(PaletteMapSpec far *mapping) {
    int half_height, half_width;
    int limit_y, limit_x;
    unsigned int y_squared;
    unsigned int row_start;
    unsigned int radius_limit;
    float distance;
    register int y, x;
    register int dy, dx;
    register unsigned char color_index;

    half_height = mapping->height / 2;
    half_width = mapping->width / 2;
    limit_y = half_height + half_height / 4;
    limit_x = half_width + half_width / 4;
    radius_limit = half_height < half_width ? half_height - 1 : half_width - 1;
    radius_limit *= radius_limit;

    for (y = 0; y < mapping->height; ++y) {
        y_squared = abs(half_height - y);
        y_squared *= y_squared;
        row_start = mapping->width * y;
        for (x = 0; x < mapping->width; ++x) {
            dx = abs(half_width - x);
            dx *= dx;
            if (y_squared + dx > radius_limit) {
                mapping->pixels[row_start + x] = 0;
            } else {
                dx = abs(limit_y - y);
                dy = abs(limit_x - x);
                if (dx || dy) {
                    distance = sqrt((double)(unsigned int)(dx * dx + dy * dy));
                } else {
                    distance = 0.0;
                }
                color_index = (int)distance % 253;
                if (g_5db6 < 0x80)
                    color_index >>= 1;
                mapping->pixels[row_start + x] = (unsigned char)(color_index + 3);
            }
        }
    }
}

extern unsigned g_7488, g_5dc6, g_5caf;
extern ImageDrawRoutine g_01fe;
extern ImageDrawRoutine g_5c4b;
extern int g_5d90, g_5d92, g_5cb1;
extern unsigned g_5d80;
extern char far *g_5ffa;
extern void (*g_5c43)(void);
extern ImageDrawRoutine g_5c3f;
extern char g_5c5f[];
extern unsigned g_5cc5, g_5cc3, g_5da8, g_5da4, g_5de0;
extern void initialize_math_tables(void);
extern void delay(unsigned);
extern void far backend_1899_0aa5(void);
extern void far backend_1899_02e8(void);

/* objcmp: {"function":"initialize_program_defaults","address":"11c1:1f29","size":150,"symbols":{"_g_7488":"DS:7488","_g_5dc6":"DS:5dc6","_g_5caf":"DS:5caf","_g_01fe":"DS:01fe","_g_5c4b":"DS:5c4b","_g_5d90":"DS:5d90","_g_5d92":"DS:5d92","_g_5cb1":"DS:5cb1","_g_5d80":"DS:5d80","_initialize_math_tables":"11c1:1d64","_g_5ffa":"DS:5ffa","_g_5c43":"DS:5c43","_g_5c3f":"DS:5c3f","_g_5c5f":"DS:5c5f","_strcpy":"0000:59cc","_g_5cc5":"DS:5cc5","_g_5cc3":"DS:5cc3","_g_5da8":"DS:5da8","_g_5da4":"DS:5da4","_g_5de0":"DS:5de0","_delay":"0000:29a6","_backend_1899_0aa5":"1899:0aa5","_backend_1899_02e8":"1899:02e8"},"segments":{"_DATA":"DS:2007"}} */
void initialize_program_defaults(void) {
    g_7488 = 3;
    g_5dc6 = 1;
    g_5caf = 3;
    g_5c4b = g_01fe;
    g_5d92 = (g_5d90 = 112);
    g_5d80 = (g_5cb1 = 1);
    initialize_math_tables();
    g_5ffa = "DAZZLE.DZL\0\0";
    g_5c43 = backend_1899_0aa5;
    g_5c3f = backend_1899_02e8;
    strcpy(g_5c5f, g_5ffa);
    g_5cc5 = 3;
    g_5cc3 = 9;
    g_5da8 = 91;
    g_5da4 = 3;
    g_5de0 = 0;
    delay(1);
}

/* objcmp: {"function":"beep_float","address":"11c1:1fbf","size":48,"symbols":{"_sound":"0000:3d88","_delay":"0000:29a6","_nosound":"0000:3db4","F_FTOL@":"0000:0fbe"},"segments":{"_DATA":"DS:2007"},"absolute_symbols":{"FIDRQQ":"0x5c32"}} */
void sound(unsigned); void delay(unsigned); void nosound(void);
void beep_float(float hz, unsigned ms) {
    int f;
    f = hz + 0.5;
    sound(f);
    delay(ms);
    nosound();
}

/* objcmp: {"function":"play_tone_sequence","address":"11c1:1fef","size":370,"symbols":{"_g_5dbe":"DS:5dbe","_g_5d7c":"DS:5d7c","_g_0c14":"DS:0c14","_g_0c16":"DS:0c16","_g_0c0c":"DS:0c0c","_bioskey":"0000:2553","_beep_float":"11c1:1fbf"},"segments":{"_DATA":"DS:2007"},"absolute_symbols":{"FIDRQQ":"0x5c32","FIWRQQ":"0xa23d"}} */
extern unsigned g_5dbe, g_5d7c;
extern float g_0c14;
extern unsigned g_0c0c[12];
extern int bioskey(unsigned);
extern void beep_float(float, unsigned);
void play_tone_sequence(void) {
    float notes[8][12];
    int note;
    register int octave;
    register int melody;

    if (g_5dbe)
        return;
    for (octave = 0; octave < 8; ++octave) {
        for (note = 0; note < 12; ++note) {
            notes[octave][note] = g_0c14;
            g_0c14 *= 1.0594645;
        }
    }
    if (g_5d7c || bioskey(1))
        goto done;

    for (octave = 7; octave > 1; --octave) {
        if (g_5d7c || bioskey(1))
            goto done;
        for (note = 0; note < 4; ++note)
            beep_float(notes[octave][g_0c0c[note]], 80);
    }

    if (g_5d7c || bioskey(1))
        goto done;
    for (melody = 0; melody < 7; ++melody) {
        beep_float(notes[4][0], 80);
        beep_float(notes[4][2], 80);
    }
    if (!g_5d7c && !bioskey(1))
        beep_float(notes[4][0], 600);
done:
    nosound();
}

/* objcmp: {"function":"closing_graphic_and_music","address":"11c1:2161","size":69,"symbols":{"_closing_logo":"17f5:051c","_g_5d7c":"DS:5d7c","_g_5d7e":"DS:5d7e","_g_5d78":"DS:5d78","_g_5dbc":"DS:5dbc","_play_tone_sequence":"11c1:1fef","_delay":"0000:29a6","_bioskey":"0000:2553"}} */
extern void far closing_logo(void);
extern unsigned g_5d7e, g_5d78, g_5dbc;
extern void delay(unsigned);
extern int bioskey(unsigned);

void closing_graphic_and_music(void) {
    closing_logo();
    g_5d7e = (g_5d7c = 0);
    g_5d78 = 182;
    if (!g_5dbc) {
        play_tone_sequence();
        delay(1000);
    }
    while (!bioskey(1) && g_5d78 > 0 && !g_5d7c)
        ;
}

/* objcmp: {"function":"configure_video_backend","address":"11c1:21a6","size":879,"symbols":{"_g_5d82":"DS:5d82","_g_5c33":"DS:5c33","_g_743c":"DS:743c","_g_743a":"DS:743a","_g_7438":"DS:7438","_g_7484":"DS:7484","_g_5da2":"DS:5da2","_g_5dd4":"DS:5dd4","_g_5da6":"DS:5da6","_g_5dba":"DS:5dba","_g_5db6":"DS:5db6","_g_5dc0":"DS:5dc0","_g_5db0":"DS:5db0","_g_5db2":"DS:5db2","_g_5de4":"DS:5de4","_g_5c43":"DS:5c43","_g_5c3f":"DS:5c3f","_g_5c3b":"DS:5c3b","_g_5db4":"DS:5db4","_g_5dae":"DS:5dae","_g_5c47":"DS:5c47","_g_5dd8":"DS:5dd8","_g_5c37":"DS:5c37","_g_5c49":"DS:5c49","_g_5d88":"DS:5d88","_g_5db8":"DS:5db8","_g_5d94":"DS:5d94","_g_5d96":"DS:5d96","_g_5de8":"DS:5de8","_g_5d80":"DS:5d80","_g_5c57":"DS:5c57","_g_5c59":"DS:5c59","_g_5c4f":"DS:5c4f","_g_5c51":"DS:5c51","_g_5c53":"DS:5c53","_g_5c55":"DS:5c55","_g_441c":"DS:441c","_g_2028":"DS:2028","_g_2047":"DS:2047","_g_2068":"DS:2068","_set_display_start":"1899:0ebf","_vga_mode_tall":"1899:0b9e","_vga_mode_standard":"1899:0afc","_vga_plot_tall":"1899:0622","_vga_plot_standard":"1899:048b","_vga_read_tall":"1899:08f1","_vga_read_standard":"1899:088b","_vga_palette_builder":"11c1:13ef","_vga_palette_writer_bios_split":"1899:0d27","_vga_palette_writer_split":"1899:0dee","_vga_palette_writer_fast":"1899:0d74","_vga_palette_writer_bios":"1899:0d14","_vga_palette_writer_x":"1899:0dbf","_vga_palette_writer":"1899:0d48","_ega_mode":"1899:0aa5","_ega_plot_tall":"1899:02e8","_ega_plot_standard":"1899:012d","_ega_read_tall":"1899:07f5","_ega_read_standard":"1899:075d","_ega_palette_writer_bios":"1899:0c13","_ega_palette_writer":"1899:0c7c","_ega_palette_builder":"11c1:0e45","_malloc":"0000:2c51","_rand":"0000:1694","_fprintf":"0000:45d4","_video_error":"11c1:0d46","_initialize_palette_mapping":"11c1:1dd5","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7"}} */
typedef void (far *VideoCallback)(void);
typedef unsigned (far *ReadPixelCallback)(int, int);
typedef void (far *PaletteCallback)(unsigned char far *);
extern void (*g_5c33)(unsigned, unsigned);
extern VideoCallback g_5c41, g_5c43, g_5c47;
extern ReadPixelCallback g_5c3b;
extern PaletteCallback g_5c37;
extern unsigned g_5d82, g_7438, g_743a, g_743c;
extern unsigned g_5da2, g_5dd4, g_5da6, g_5dba, g_5db6, g_5dc0;
extern int g_5db0, g_5db2;
extern unsigned g_5de4, g_5db4, g_5dd8;
extern int g_5dae;
extern unsigned g_5d88, g_5db8, g_5d80, g_5c49;
extern int g_5c4f, g_5c51;
extern unsigned g_5de8;
extern unsigned g_7484;
extern unsigned char far *g_5d94;
extern void far *g_5c57, *g_5c53;
extern FILE g_441c;
extern char g_2028[], g_2047[], g_2068[];
extern void far set_display_start(unsigned, unsigned);
extern void far vga_mode_tall(unsigned), vga_mode_standard(unsigned);
extern void far ega_mode(unsigned);
extern void far vga_plot_tall(unsigned), vga_plot_standard(unsigned);
extern void far ega_plot_tall(unsigned), ega_plot_standard(unsigned);
extern unsigned far vga_read_tall(int, int), vga_read_standard(int, int);
extern unsigned far ega_read_tall(int, int), ega_read_standard(int, int);
extern void far vga_palette_builder(unsigned), ega_palette_builder(unsigned);
extern void far vga_palette_writer_bios_split(unsigned, unsigned);
extern void far vga_palette_writer_split(unsigned, unsigned);
extern void far vga_palette_writer_fast(unsigned, unsigned);
extern void far vga_palette_writer_bios(unsigned, unsigned);
extern void far vga_palette_writer_x(unsigned, unsigned);
extern void far vga_palette_writer(unsigned, unsigned);
extern void far ega_palette_writer_bios(unsigned, unsigned);
extern void far ega_palette_writer(unsigned, unsigned);
extern void far video_error(unsigned);
extern void far initialize_palette_mapping(PaletteMapSpec far *);

void configure_video_backend(int allocate) {
    register int divisor;

    if (allocate) g_5d82 = 0;
    g_5c33 = set_display_start;
    g_743c = 0x2f;
    g_743a = 0x3f;
    g_7438 = 0x1f;
    if (g_7484 == 8) {
        g_5da2 = 1;
        if (allocate) g_5da6 = g_5dd4 ? g_5da2 << 2 : g_5da2;
        g_5dba = 576;
        g_5db6 = 256;
        g_5dc0 = g_5db6 - 1;
        g_5db0 = 160;
        if (g_5de4) {
            g_5db2 = 100;
            g_5c43 = (VideoCallback)vga_mode_tall;
            g_5c3f = (VideoCallback)vga_plot_tall;
            g_5c3b = vga_read_tall;
            g_5dae = g_5db0 + 1;
        } else {
            g_5db2 = 200;
            g_5c43 = (VideoCallback)vga_mode_standard;
            g_5c3f = (VideoCallback)vga_plot_standard;
            g_5c3b = vga_read_standard;
            if (g_5db4 || g_5dd8 || g_5dd4)
                g_5dae = g_5db2 - 28;
            else
                g_5dae = g_5db2 + 1;
        }
        g_5c47 = vga_palette_builder;
        if (g_5dd8) {
            if (g_5db4)
                g_5c37 = vga_palette_writer_bios_split;
            else if (g_5dd4)
                g_5c37 = vga_palette_writer_split;
            else
                g_5c37 = vga_palette_writer_fast;
        } else if (g_5db4) {
            g_5c37 = vga_palette_writer_bios;
        } else if (g_5dd4) {
            g_5c37 = vga_palette_writer_x;
        } else {
            g_5c37 = vga_palette_writer;
        }
    }
    if (g_7484 == 4) {
        g_5da2 = 2;
        if (allocate) g_5da6 = g_5da2;
        g_5dba = 64;
        g_5db6 = 16;
        g_5dc0 = g_5db6 - 1;
        g_5db0 = 320;
        g_5c43 = ega_mode;
        if (g_5de4) {
            g_5c3f = (VideoCallback)ega_plot_tall;
            g_5c3b = ega_read_tall;
            g_5db2 = 175;
            g_5dae = g_5db0 + 1;
        } else {
            g_5c3f = (VideoCallback)ega_plot_standard;
            g_5c3b = ega_read_standard;
            g_5db2 = 350;
            g_5dae = g_5db2 + 1;
        }
        if (g_5db4)
            g_5c37 = ega_palette_writer_bios;
        else
            g_5c37 = ega_palette_writer;
        g_5c47 = ega_palette_builder;
    }
    g_5d88 = 0xffff / (g_5dae + (g_5dae >> 2)) - 1;
    g_5db8 = g_5dba * 3;
    if (!allocate) return;

    if (g_7484 == 4)
        g_5de8 = g_5db6 + g_5dba;
    else
        g_5de8 = (g_5db6 + g_5dba) * 3;
    g_5d94 = (unsigned char far *)malloc(g_5de8 + 3);
    if (!g_5d94) {
        g_5d80 = 1;
        fprintf(&g_441c, g_2028);
        video_error(0x3c);
    }
    g_5c57 = malloc(0x7d8);
    if (!g_5c57) {
        g_5d80 = 1;
        fprintf(&g_441c, g_2047);
        video_error(0x46);
    }
    divisor = (int)((long)rand() * 5L / 32768L) + 2;
    g_5c4f = g_5db2 / divisor;
    g_5c51 = g_5db0 / divisor;
    g_5c53 = malloc(g_5c4f * g_5c51);
    if (!g_5c53) {
        g_5d80 = 1;
        fprintf(&g_441c, g_2068);
        video_error(0x47);
    }
    initialize_palette_mapping((PaletteMapSpec far *)&g_5c4f);
}

/* objcmp: {"function":"cycle_palette","address":"11c1:2515","size":89,"symbols":{"_g_0bfe":"DS:0bfe","_g_0c00":"DS:0c00","_g_7484":"DS:7484","_g_5dec":"DS:5dec","_g_5dba":"DS:5dba","_g_5da4":"DS:5da4","_g_5db8":"DS:5db8","_g_5d94":"DS:5d94","_g_5d96":"DS:5d96","_g_5c37":"DS:5c37","_g_5d7a":"DS:5d7a"}} */
extern unsigned g_0bfe, g_0c00, g_7484;
extern unsigned g_5dec, g_5dba, g_5da4, g_5db8;
extern unsigned g_5d7a;
extern unsigned char far *g_5d94;
extern PaletteCallback g_5c37;

void cycle_palette(void) {
    if (!g_0bfe || g_0c00) {
        if (g_7484 == 4) {
            ++g_5dec;
            if (g_5dec >= g_5dba) g_5dec = 3;
        } else {
            g_5dec = g_5dec + g_5da4;
            if (g_5dec >= g_5db8) g_5dec = 9;
        }
        g_5c37(g_5d94 + g_5dec);
    }
    g_5d7a = 0;
}

/* objcmp: {"function":"parse_config_version","address":"11c1:256e","size":104,"symbols":{"_g_version_format":"DS:2087","_g_version_corrupt":"DS:208a","_g_version_warning":"DS:20b3","_strchr":"0000:598f","_sscanf":"0000:5909","_report_config_error":"158d:2492"}} */
extern char g_version_format[];
extern char g_version_corrupt[];
extern char g_version_warning[];
extern char far *strchr(const char far *, int);
extern int sscanf(const char far *, const char far *, ...);
extern void far report_config_error(char far *);

int parse_config_version(char far *line) {
    char far *mark;
    unsigned version;

    mark = strchr(line, '#');
    ++mark;
    if (sscanf(mark, g_version_format, &version) == -1) {
        report_config_error(g_version_corrupt);
        return 1;
    }
    if (version < 10410)
        report_config_error(g_version_warning);
    return 0;
}

/* objcmp: {"function":"parse_alt_image_size","address":"11c1:25d6","size":244,"symbols":{"_g_format_char":"DS:20da","_g_format_decimal":"DS:20dd","_strchr":"0000:598f","_sscanf":"0000:5909","_g_5d88":"DS:5d88","_g_5d9c":"DS:5d9c","_g_020c":"DS:020c","_g_0202":"DS:0202"}} */
extern char g_format_char[];
extern char g_format_decimal[];
extern unsigned g_5d88, g_5d9c;
extern char g_020c[];
extern char g_0202[];

void parse_alt_image_size(char far *line) {
    int size;
    char far *p;
    char key;
    unsigned i;

    if (!(p = strchr(line, '~'))) return;
    ++p;
    while (*p && (*p == ' ' || *p == '\t')) ++p;
    if (*p == 0) return;
    if (sscanf(p, g_format_char, &key) == -1)
        return;
    if (!(p = strchr(p, '['))) return;
    ++p;
    if (sscanf(p, g_format_decimal, &size) == -1)
        return;
    if (size > g_5d88) size = g_5d88;
    if (size < 0) size = 0;
    for (i = 0; i < g_5d9c; ++i) {
        if (g_020c[i * 15] == key) {
            *(unsigned *)&g_0202[i * 15] = size;
            break;
        }
    }
}

/* objcmp: {"function":"parse_ctrl_image_size","address":"11c1:26ca","size":244,"symbols":{"_g_format_char":"DS:20da","_g_format_decimal":"DS:20dd","_strchr":"0000:598f","_sscanf":"0000:5909","_g_5d88":"DS:5d88","_g_5d9a":"DS:5d9a","_g_0428":"DS:0428","_g_041e":"DS:041e"}} */
extern unsigned g_5d9a;
extern char g_0428[];
extern char g_041e[];

void parse_ctrl_image_size(char far *line) {
    int size;
    char far *p;
    char key;
    unsigned i;

    if (!(p = strchr(line, '^'))) return;
    ++p;
    while (*p && (*p == ' ' || *p == '\t')) ++p;
    if (*p == 0) return;
    if (sscanf(p, g_format_char, &key) == -1)
        return;
    if (!(p = strchr(p, '['))) return;
    ++p;
    if (sscanf(p, g_format_decimal, &size) == -1)
        return;
    if (size > g_5d88) size = g_5d88;
    if (size < 0) size = 0;
    for (i = 0; i < g_5d9a; ++i) {
        if (g_0428[i * 15] == key) {
            *(unsigned *)&g_041e[i * 15] = size;
            break;
        }
    }
}

/* objcmp: {"function":"parse_fade_enabled","address":"11c1:27be","size":256,"symbols":{"_g_format_char":"DS:20da","_strchr":"0000:598f","_sscanf":"0000:5909","_g_5d98":"DS:5d98","_g_0474":"DS:0474","_g_0479":"DS:0479"}} */
extern unsigned g_5d98;
extern char g_0474[], g_0479[];

void parse_fade_enabled(char far *line) {
    char far *p;
    char key;
    char enabled;
    unsigned i;

    p = line;
    while (*p && (*p == ' ' || *p == '\t')) ++p;
    if (*p == 0) return;
    if (sscanf(p, g_format_char, &key) == -1)
        return;
    if (!(p = strchr(p, '['))) return;
    ++p;
    while (*p && (*p == ' ' || *p == '\t')) ++p;
    if (*p == 0) return;
    if (sscanf(p, g_format_char, &enabled) == -1)
        return;
    if (enabled != 'n' && enabled != 'y') return;
    for (i = 0; i < g_5d98; ++i) {
        if (g_0474[i * 7] == key) {
            *(unsigned *)&g_0479[i * 7] = (enabled == 'y');
            break;
        }
    }
}

/* objcmp: {"function":"parse_auto_configuration","address":"11c1:28be","size":240,"symbols":{"_g_format_char":"DS:20da","_g_format_decimal":"DS:20dd","_strchr":"0000:598f","_sscanf":"0000:5909","_g_5cc5":"DS:5cc5","_g_5cc3":"DS:5cc3","_g_5da8":"DS:5da8"}} */
extern unsigned g_5cc5, g_5cc3, g_5da8;

void parse_auto_configuration(char far *line) {
    int value;
    char far *p;
    char key;

    if (!(p = strchr(line, '$'))) return;
    ++p;
    while (*p && (*p == ' ' || *p == '\t')) ++p;
    if (*p == 0) return;
    if (sscanf(p, g_format_char, &key) == -1)
        return;
    if (!(p = strchr(p, '['))) return;
    ++p;
    if (sscanf(p, g_format_decimal, &value) == -1)
        return;
    if (value > 256) value = 256;
    if (value < 0) value = 1;
    switch (key) {
    case 'i':
        g_5cc5 = value;
        break;
    case 'x':
        g_5cc3 = value;
        break;
    case 'f':
        g_5da8 = value * 18;
        break;
    }
}

/* objcmp: {"function":"load_configuration_file","address":"11c1:29ae","size":406,"symbols":{"_g_mode_rt":"DS:20e0","_g_config_close_error":"DS:20e3","_g_5ffe":"DS:5ffe","_fopen":"0000:459f","_fgets":"0000:4236","_clearerr":"0000:102f","_fclose":"0000:407f","_strlen":"0000:5a36","_strchr":"0000:598f","_strlwr":"0000:5a55","_report_config_error":"158d:2492","_parse_config_version":"11c1:256e","_parse_alt_image_size":"11c1:25d6","_parse_ctrl_image_size":"11c1:26ca","_parse_fade_enabled":"11c1:27be","_parse_auto_configuration":"11c1:28be"}} */
extern char g_mode_rt[], g_config_close_error[];
extern FILE far *g_5ffe;
extern FILE far *fopen(const char far *, const char far *);
extern char far *fgets(char far *, int, FILE far *);
extern void clearerr(FILE far *);
extern int fclose(FILE far *);
extern unsigned strlen(const char far *);
extern char far *strlwr(char far *);

int load_configuration_file(char far *path) {
    char line[80];
    char far *p;

    if (!(g_5ffe = fopen(path, g_mode_rt)))
        return 1;
    for (;;) {
        if (!fgets(line, 80, g_5ffe)) {
            clearerr(g_5ffe);
            if (fclose(g_5ffe) != 0)
                report_config_error(g_config_close_error);
            return 0;
        }
        if (strlen(line) < 1)
            continue;
        if ((p = strchr(line, ';')) != 0)
            *p = 0;
        if (strlen(line) < 1)
            continue;
        p = line;
        while (*p && (*p == ' ' || *p == '\t'))
            ++p;
        if (*p == 0)
            continue;
        if (!strlwr(p))
            continue;
        switch (*p) {
        case '#':
            if (parse_config_version(p)) {
                fclose(g_5ffe);
                return 1;
            }
            break;
        case '~':
            parse_alt_image_size(p);
            break;
        case '^':
            parse_ctrl_image_size(p);
            break;
        case '`':
            break;
        case '$':
            parse_auto_configuration(p);
            break;
        default:
            parse_fade_enabled(p);
            break;
        }
    }
}

/* objcmp: {"function":"save_configuration_file","address":"11c1:2b44","size":482,"symbols":{"_g_config_write_mode":"DS:20fc","_g_config_save_error":"DS:20ff","_g_5ffe":"DS:5ffe","_g_config_header":"DS:211d","_g_alt_write_format":"DS:223d","_g_ctrl_write_format":"DS:2259","_g_fade_write_format":"DS:2275","_g_min_write_format":"DS:228f","_g_max_write_format":"DS:22bd","_g_fade_delay_write_format":"DS:22eb","_g_alt_records":"DS:01fe","_g_ctrl_records":"DS:041a","_g_fade_records":"DS:0474","_g_5d9c":"DS:5d9c","_g_5d9a":"DS:5d9a","_g_5d98":"DS:5d98","_g_5cc5":"DS:5cc5","_g_5cc3":"DS:5cc3","_g_5da8":"DS:5da8","_fopen":"0000:459f","_fprintf":"0000:45d4","_fclose":"0000:407f","_report_config_error":"158d:2492"}} */
extern char g_config_write_mode[], g_config_save_error[];
extern char g_config_header[], g_alt_write_format[], g_ctrl_write_format[];
extern char g_fade_write_format[], g_min_write_format[], g_max_write_format[];
extern char g_fade_delay_write_format[];
extern unsigned g_5d9c, g_5d9a, g_5d98;
extern unsigned g_5cc5, g_5cc3, g_5da8;
extern void far report_config_error(char far *);

typedef struct {
    ImageDrawRoutine draw;
    unsigned size;
    char far *name;
    unsigned multicolor;
    unsigned independent_colors;
    char key;
} ImageConfigRow;

typedef struct {
    char key;
    char far *name;
    unsigned enabled;
} FadeConfigRow;

extern ImageConfigRow g_alt_records[];
extern ImageConfigRow g_ctrl_records[];
extern FadeConfigRow g_fade_records[];

int save_configuration_file(char far *path) {
    unsigned i;
    register char enabled;

    if (!(g_5ffe = fopen(path, g_config_write_mode))) {
        report_config_error(g_config_save_error);
        return 1;
    }
    fprintf(g_5ffe, g_config_header);
    for (i = 0; i < g_5d9c; ++i) {
        if (fprintf(g_5ffe, g_alt_write_format, g_alt_records[i].key,
                    g_alt_records[i].size,
                    g_alt_records[i].name) == -1)
            goto write_error;
    }
    for (i = 0; i < g_5d9a; ++i) {
        if (fprintf(g_5ffe, g_ctrl_write_format, g_ctrl_records[i].key,
                    g_ctrl_records[i].size,
                    g_ctrl_records[i].name) == -1)
            goto write_error;
    }
    for (i = 0; i < g_5d98; ++i) {
        if (g_fade_records[i].enabled)
            enabled = 'y';
        else
            enabled = 'n';
        if (fprintf(g_5ffe, g_fade_write_format, g_fade_records[i].key,
                    enabled,
                    g_fade_records[i].name) == -1)
            goto write_error;
    }
    if (fprintf(g_5ffe, g_min_write_format, g_5cc5) == -1) goto write_error;
    if (fprintf(g_5ffe, g_max_write_format, g_5cc3) == -1) goto write_error;
    if (fprintf(g_5ffe, g_fade_delay_write_format, (int)g_5da8 / 18) == -1) {
write_error:
        fclose(g_5ffe);
        return 1;
    }
    fclose(g_5ffe);
    return 0;
}

/* objcmp: {"function":"resolve_startup_configuration","address":"11c1:2d26","size":455,"symbols":{"_g_5c5f":"DS:5c5f","_g_5ffa":"DS:5ffa","_g_5ffc":"DS:5ffc","_g_5d17":"DS:5d17","_g_5dbc":"DS:5dbc","_g_5dca":"DS:5dca","_g_startup_load_error":"DS:0c18","_g_illegal_config_name":"DS:2315","_g_loaded_config":"DS:2341","_g_441c":"DS:441c","_load_configuration_file":"11c1:29ae","_findfirst":"0000:42d2","_fnsplit":"0000:439f","_fnmerge":"0000:4374","_searchpath":"0000:1ea4","_tolower":"0000:1f66","_setdisk":"0000:128e","_strcpy":"0000:59cc","_strcat":"0000:5950","_fprintf":"0000:45d4","_delay":"0000:29a6","_textcolor":"0000:25fc","_filename_is_legal":"11c1:0d85","_cprintf":"0000:277a"}} */
extern unsigned g_5dbc, g_5dca;
extern char g_5d17[];
extern char g_startup_load_error[], g_illegal_config_name[], g_loaded_config[];
extern FILE g_441c;
extern int tolower(int);
extern int setdisk(int);
extern int far filename_is_legal(char far *);
extern int far fprintf(FILE far *, const char far *, ...);
extern int far cprintf(const char far *, ...);
extern void far textcolor(int);

int resolve_startup_configuration(int requested) {
    char far *found_path;
    int find_result;
    char drive[MAXDRIVE];
    char filename[MAXFILE];
    char extension[MAXEXT];
    struct ffblk file_info;
    char directory[MAXDIR];

    if (requested < 0)
        return 0;

    if ((find_result = findfirst(g_5c5f, &file_info, 0)) && requested) {
        fprintf(&g_441c, g_startup_load_error, g_5c5f);
        delay(1250);
        return 1;
    }

    if (find_result) {
        fnsplit(g_5ffa, 0, 0, filename, extension);
        fnsplit(g_5d17, drive, directory, 0, 0);
        fnmerge(g_5c5f, drive, directory, filename, extension);
        if (findfirst(g_5c5f, &file_info, 0)) {
            if (found_path = searchpath(g_5ffa))
                strcpy(g_5c5f, found_path);
            else
                return 0;
        }
    } else {
        fnsplit(g_5c5f, drive, 0, 0, 0);
        /* The original tests the buffer pointer, rather than its first byte. */
        if (drive)
            setdisk(tolower(drive[0]) - 'a');
    }

    if (filename_is_legal(g_5c5f)) {
        fprintf(&g_441c, g_illegal_config_name);
        strcpy(g_5c5f, g_5ffa);
        delay(1250);
        return 1;
    }

    if (load_configuration_file(g_5c5f) && requested > 0) {
        fprintf(&g_441c, g_startup_load_error, g_5c5f);
        delay(1250);
        return 1;
    }

    textcolor(9);
    if (!g_5dbc && !g_5dca)
        cprintf(g_loaded_config, g_5c5f);
    return 0;
}

/* objcmp: {"function":"parse_command_line","address":"11c1:2eed","size":1202,"symbols":{"_getswitchar":"0000:12ad","_tolower":"0000:1f66","_cprintf":"0000:277a","_exit":"0000:11a8","_strchr":"0000:598f","_sscanf":"0000:5909","_strcpy":"0000:59cc","_strcat":"0000:5950","_g_5c5f":"DS:5c5f","_g_5ffa":"DS:5ffa","_g_5ffc":"DS:5ffc","_g_5cb7":"DS:5cb7","_g_5dc8":"DS:5dc8","_g_5dc2":"DS:5dc2","_g_5dd8":"DS:5dd8","_g_5dd0":"DS:5dd0","_g_5dd4":"DS:5dd4","_g_5de0":"DS:5de0","_g_5dca":"DS:5dca","_g_5db4":"DS:5db4","_g_5dc6":"DS:5dc6","_g_5de2":"DS:5de2","_g_5dde":"DS:5dde","_g_5dd2":"DS:5dd2","_g_5dda":"DS:5dda","_g_5dcc":"DS:5dcc","_g_5dbe":"DS:5dbe","_g_5dce":"DS:5dce","_g_5ddc":"DS:5ddc","_g_5d86":"DS:5d86","_g_5de4":"DS:5de4","_g_5dbc":"DS:5dbc","_g_7480":"DS:7480","_g_7482":"DS:7482","_g_unknown_option":"DS:234f","_g_unsigned_format":"DS:2366","_g_format_parse_error":"DS:236a","_g_delay_limit":"DS:2380","_g_showtime_limit":"DS:23a6","_g_force_xt_message":"DS:23cc","_g_dzl_suffix":"DS:2017","_show_option_help":"11c1:0bed","_prepare_exit":"11c1:0949","_show_registration":"11c1:0ab6","_targa64_exit":"1a72:11bb","_check_x86_mode":"1899:0ff5"}} */
/* Walks far argv strings, accepts concatenated switch characters, and handles
   optional numeric values, one input path, and the documented application flags. */
extern unsigned g_5dc8, g_5dc2, g_5dd8, g_5dd0, g_5dd4, g_5de0;
extern unsigned g_5dca, g_5db4, g_5dc6, g_5de2, g_5dde, g_5dd2;
extern unsigned g_5dda, g_5dcc, g_5dbe, g_5dce, g_5ddc, g_5d86, g_5de4;
extern unsigned g_5cb7, g_5dbc, g_7480, g_7482;
extern char g_unknown_option[], g_unsigned_format[], g_format_parse_error[];
extern char g_delay_limit[];
extern char g_showtime_limit[], g_force_xt_message[], g_dzl_suffix[];
extern char far *strchr(const char far *, int);
extern char far *strcat(char far *, const char far *);
extern int sscanf(const char far *, const char far *, ...);
extern int getswitchar(void);
extern void far show_option_help(void);
extern void far prepare_exit(void);
extern void far show_registration(int);
extern void far targa64_exit(void);
extern void far exit(int);
extern int far check_x86_mode(void);

int parse_command_line(int argc, char far * far *argv,
                       unsigned far *option_position) {
    register int argument;
    int result;
    int machine_mode;
    char previous;
    char switch_character;
    register int string_position;

    result = 0;
    *option_position = 0xffff;
    strcpy(g_5c5f, g_5ffa);
    switch_character = (char)getswitchar();
    if (argc >= 1) {
        for (argument = 1; argument < argc; ++argument) {
            if (*argv[argument] == '-' || *argv[argument] == switch_character) {
                argv[argument][0] = '-';
                string_position = 1;
                while ((previous = argv[argument][string_position]) != 0) {
                    if (previous == switch_character)
                        previous = '-';
                    switch (tolower(previous)) {
                    default:
                        fprintf(&g_441c, g_unknown_option, previous);
                    case '?':
                        show_option_help();
                        break;
                    case '$':
                        g_5dc8 = 1;
                        prepare_exit();
                        show_registration(1);
                        exit(0);
                        break;
                    case '!':
                        g_5cb7 = 1;
                        break;
                    case '+':
                        g_5dc2 = 1;
                        break;
                    case '-':
                        break;
                    case '2':
                        g_5dd8 = 1;
                        break;
                    case '4':
                        g_5dd0 = 1;
                        break;
                    case 'a':
                        g_5de0 = 1;
                        break;
                    case 'b':
                        g_5dca = 1;
                        break;
                    case 'c':
                        g_5db4 = 1;
                        break;
                    case 'd':
                        g_5dc6 = 0x100;
                        if (argv[argument][string_position + 1] == '=') {
                            string_position += 2;
                            if (sscanf(argv[argument] + string_position, g_unsigned_format, &g_5dc6) != 1) {
                                fprintf(&g_441c, g_format_parse_error, argv[argument][string_position - 2]);
                                show_option_help();
                            }
                            if (g_5dc6 < 1)
                                g_5dc6 = 1;
                            if (g_5dc6 > 0x1fff) {
                                fprintf(&g_441c, g_delay_limit);
                                show_option_help();
                            }
                            while ((previous = argv[argument][string_position + 1]) >= '0' && previous <= '9')
                                ++string_position;
                        }
                        break;
                    case 'e':
                        g_7480 = 1;
                        break;
                    case 'f':
                        g_5de2 = 1;
                        break;
                    case 'h':
                        g_5dde = 1;
                        break;
                    case 'i':
                        *option_position = string_position;
                        g_5dc8 = argument;
                        break;
                    case 'k':
                        g_5dd2 = 1;
                        break;
                    case 'm':
                        g_5de0 = 0;
                        g_5dda = 1;
                        break;
                    case 'n':
                        g_5dcc = 1;
                        break;
                    case 'q':
                        g_5dbe = 1;
                        break;
                    case 's':
                        g_5dce = 1;
                        if (argv[argument][string_position + 1] == '=') {
                            string_position += 2;
                            if (sscanf(argv[argument] + string_position, g_unsigned_format, &g_5d86) != 1) {
                                fprintf(&g_441c, g_format_parse_error, argv[argument][string_position - 2]);
                                show_option_help();
                            }
                            if (g_5d86 > 3600) {
                                fprintf(&g_441c, g_showtime_limit);
                                show_option_help();
                            }
                            g_5d86 *= 18;
                            while ((previous = argv[argument][string_position + 1]) >= '0' && previous <= '9')
                                ++string_position;
                        }
                        break;
                    case 't':
                        g_5de4 = 1;
                        break;
                    case 'v':
                        g_7482 = 1;
                        break;
                    case 'w':
                        g_5dbc = 1;
                        break;
                    case 'x':
                        g_5dd4 = 1;
                        break;
                    case 'y':
                        g_5ddc = 1;
                        break;
                    case 'z':
                        targa64_exit();
                        exit(3);
                    case '*':
                        result = -1;
                        break;
                    }

                    ++string_position;
                }
            } else {
                strcpy(g_5c5f, argv[argument]);
                if (strchr(g_5c5f, '?'))
                    show_option_help();
                if (!strchr(g_5c5f, '*')) {
                    if (!strchr(g_5c5f, '.'))
                        strcat(g_5c5f, g_dzl_suffix);
                    result = 1;
                } else {
                    strcpy(g_5c5f, g_5ffa);
                    result = -1;
                }
            }
        }
    }
    if (g_5dd4) {
        return result;
    } else {
        machine_mode = check_x86_mode();
        if (machine_mode == 0) {
            fprintf(&g_441c, g_force_xt_message);
            g_5dd4 = 1;
        }
    }
    return result;
}

/* objcmp: {"function":"print_startup_information","address":"11c1:339f","size":410,"symbols":{"_cprintf":"0000:277a","_textcolor":"0000:25fc","_strlen":"0000:5a36","_highvideo":"0000:2635","_lowvideo":"0000:263b","_g_7484":"DS:7484","_g_5dbc":"DS:5dbc","_g_5dca":"DS:5dca","_g_5dcc":"DS:5dcc","_g_5dce":"DS:5dce","_g_5dc4":"DS:5dc4","_g_5cb9":"DS:5cb9","_g_5cb5":"DS:5cb5","_g_5c5b":"DS:5c5b","_g_009a":"DS:009a","_g_009e":"DS:009e","_g_00a0":"DS:00a0","_g_23fa":"DS:23fa","_g_2400":"DS:2400","_g_2405":"DS:2405","_g_0c34":"DS:0c34","_g_0c3b":"DS:0c3b","_g_0cba":"DS:0cba","_g_16a1":"DS:16a1","_g_2415":"DS:2415","_g_241a":"DS:241a","_g_2424":"DS:2424","_g_2429":"DS:2429","_g_2433":"DS:2433","_g_243f":"DS:243f","_g_245e":"DS:245e","_g_246f":"DS:246f"}} */
extern unsigned g_5dc4, g_5cb9, g_5cb5;
extern unsigned far *g_5c5b;
extern char far *g_009a, *g_009e;
extern char g_23fa[], g_2400[], g_2405[], g_0c34[], g_0c3b[], g_0cba[];
extern char g_16a1[];
extern char g_2415[], g_241a[], g_2424[], g_2429[], g_2433[];
extern char g_243f[], g_245e[], g_246f[];
extern void far textcolor(int);
extern void far highvideo(void);
extern void far lowvideo(void);
extern unsigned strlen(const char far *);

void print_startup_information(void) {
    int width;

    if (g_5dce || g_5dbc || (g_5dca && g_5dcc))
        return;
    textcolor(13);
    cprintf(g_23fa, g_009a);
    width = strlen(g_009a) + 2;
    textcolor(15);
    cprintf(g_2400, g_0c34, g_009e);
    width += strlen(g_0c34) + strlen(g_009e);
    width = width + strlen(g_0c3b);
    while (width < 0x4f) {
        cprintf(g_0cba);
        ++width;
    }
    cprintf(g_16a1, g_0c3b);
    textcolor(7);
    cprintf(g_2405);
    if (g_7484 == 4)
        cprintf(g_2415);
    if (g_5dc4)
        cprintf(g_241a);
    else if (g_7484 == 8)
        cprintf(g_2424);
    if (g_5cb9) {
        textcolor(6);
        highvideo();
        if (g_5c5b[5] & 0x8000)
            cprintf(g_2429);
        else
            cprintf(g_2433);
        lowvideo();
        cprintf(g_243f);
        if (g_5cb5)
            cprintf(g_245e);
    }
    textcolor(7);
    cprintf(g_246f, g_009a);
}

/* objcmp: {"function":"semi_auto_loop","address":"11c1:3539","size":906,"symbols":{"_g_5d9a":"DS:5d9a","_g_5d9c":"DS:5d9c","_g_5d92":"DS:5d92","_g_5d90":"DS:5d90","_g_5d9e":"DS:5d9e","_g_5da0":"DS:5da0","_g_5d7e":"DS:5d7e","_g_5d7a":"DS:5d7a","_g_5da6":"DS:5da6","_g_5d80":"DS:5d80","_g_5cc1":"DS:5cc1","_g_5cb1":"DS:5cb1","_g_5cb3":"DS:5cb3","_g_5caf":"DS:5caf","_g_5cbf":"DS:5cbf","_g_5cbd":"DS:5cbd","_g_5de6":"DS:5de6","_g_5c4b":"DS:5c4b","_g_5c4d":"DS:5c4d","_g_5c3f":"DS:5c3f","_rand":"0000:1694","_input_dispatcher":"11c1:18e7","_cycle_palette":"11c1:2515","_update_draw_bounds":"11c1:0e2a","_draw_interval":"11c1:0e07","_reset_view":"11c1:006f","_reset_video_state":"0ccd:4a82","_g_alt_records":"DS:01fe","_g_ctrl_records":"DS:041a","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7"},"level":"SEMANTIC","blocker":"control-flow layout: 904 vs 906 bytes; first differing instruction 11c1:3592 JMP rel16; draw_interval call suffix merges backward and empty H case is folded to command cleanup"} */
extern unsigned g_5d9a, g_5d9c, g_5d9e, g_5da0;
extern unsigned g_5d7e, g_5d7a, g_5da6, g_5d80, g_5cc1;
extern unsigned g_5cb3, g_5caf, g_5cbf, g_5cbd, g_5de6;
extern unsigned g_5c4d;
extern ImageConfigRow g_alt_records[], g_ctrl_records[];
extern void far update_draw_bounds(unsigned);
extern void far draw_interval(unsigned, unsigned);
extern void far reset_view(void);
extern int far input_dispatcher(void);
extern void far reset_video_state(void);
extern int rand(void);

void semi_auto_loop(void) {
    register unsigned old_index;

    old_index = 0xffff;
    g_5cc1 = (long)rand() * (g_5d9c + g_5d9a + 1) / 32768L;
    if (!g_5cb1) {
        g_5d90 = g_5d92 = 110;
        reset_video_state();
    }
    g_5d9e = 0;
    while (g_5d7e) {
        if (input_dispatcher())
            return;
    }
    g_5d92 = g_5d90 = 0;
    if (g_5da0)
        reset_view();

    while (g_5da0 && !g_5d80) {
        while (!g_5d7e && !g_5d9e) {
            if (g_5d7a >= g_5da6)
                cycle_palette();
        }
        if (input_dispatcher())
            return;

        if (g_5d92 && (unsigned)g_5d92 <= g_5d9a) {
            g_5cc1 = g_5d92 + g_5d9c;
            g_5c4b = g_ctrl_records[((unsigned)g_5d92 - 1)].draw;
            g_5d92 = 0x7100;
        } else if (((unsigned char)g_5d92 & 0x7f) < g_5d9c) {
            if (g_5d92 & 0x80) {
                g_5cc1 = (unsigned char)g_5d92 & 0x7f;
                g_5c4b = g_alt_records[g_5cc1].draw;
                g_5d92 = 0x7100;
            }
        }
        if (!g_5d92 && g_5d9e)
            g_5d92 = 0x4200;

        switch (g_5d92) {
        case 0x4300:
            g_5cc1 = (long)rand() * (g_5d9c + g_5d9a + 1) / 32768L;
            if (g_5cc1 < g_5d9c)
                g_5c4b = g_alt_records[g_5cc1].draw;
            else
                g_5c4b = g_ctrl_records[(g_5cc1 - g_5d9c - 1)].draw;
            /* Fall through to the common new-image setup. */

        case 0x7100:
            g_5cbf = (long)rand() * 2L / 32768L;
            if (g_5cc1 < g_5d9c) {
                g_5cb3 = g_alt_records[g_5cc1].independent_colors &&
                    (int)((long)rand() * 5L / 32768L) == 0 ? 1 : 0;
            } else {
                g_5cb3 = g_ctrl_records[g_5cc1 - g_5d9c - 1].independent_colors &&
                    (int)((long)rand() * 5L / 32768L) == 0 ? 1 : 0;
            }
            update_draw_bounds(g_5cb3);
            g_5c4b(1);
            old_index = g_5cc1;
            g_5cb1 = 0;
            if (g_5d92 == 0x4300)
                draw_interval(1200, 10);
            else
                draw_interval(2000, 3);
            break;

        case 0x4200:
        case 0x4400:
            g_5cb1 = 0;
            if (g_5cc1 < g_5d9c) {
                g_5caf = g_alt_records[g_5cc1].multicolor &&
                         g_5cbf ? 3 : 1;
            } else {
                g_5caf = g_ctrl_records[g_5cc1 - g_5d9c - 1].multicolor &&
                         g_5cbf ? 3 : 1;
            }
            if (old_index != g_5cc1) {
                if (g_5cc1 < g_5d9c) {
                    g_5c4b = g_alt_records[g_5cc1].draw;
                } else {
                    g_5c4b = g_ctrl_records[(g_5cc1 - g_5d9c - 1)].draw;
                }
                g_5c4b(1);
                old_index = g_5cc1;
            }
            while (!g_5d7e) {
                g_5c4b(0);
                g_5c3f(g_5caf);
                if (g_5d7a >= g_5da6)
                    cycle_palette();
            }
            break;

        case 0x68:
            break;

        case 0:
            continue;

        default:
            reset_video_state();
            if (g_5da0)
                reset_view();
            break;
        }
        g_5d90 = g_5d92 = 0;
    }
}

/* objcmp: {"function":"auto_loop","address":"11c1:38c3","size":1025,"symbols":{"_g_5cc1":"DS:5cc1","_g_5cbb":"DS:5cbb","_g_5cc3":"DS:5cc3","_g_5cc5":"DS:5cc5","_g_5c4b":"DS:5c4b","_g_5c3f":"DS:5c3f","_g_5caf":"DS:5caf","_g_5d80":"DS:5d80","_g_5d84":"DS:5d84","_g_5da0":"DS:5da0","_g_5d92":"DS:5d92","_g_5d7e":"DS:5d7e","_g_5d7a":"DS:5d7a","_g_5da6":"DS:5da6","_g_5d9a":"DS:5d9a","_g_5d9c":"DS:5d9c","_g_5dae":"DS:5dae","_g_5cb1":"DS:5cb1","_g_5cb3":"DS:5cb3","_g_5cbf":"DS:5cbf","_g_5cbd":"DS:5cbd","_g_5de6":"DS:5de6","_g_5de0":"DS:5de0","_g_5dda":"DS:5dda","_g_4408":"DS:4408","_g_0f80":"DS:0f80","_rand":"0000:1694","_fprintf":"0000:45d4","_input_dispatcher":"11c1:18e7","_cycle_palette":"11c1:2515","_update_draw_bounds":"11c1:0e2a","_reset_view":"11c1:006f","_reset_video_state":"0ccd:4a82","_g_alt_records":"DS:01fe","_g_ctrl_records":"DS:041a","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7"}} */
extern unsigned g_5cbb, g_5cc3, g_5cc5, g_5d84, g_5da8;
extern int g_5dae;
extern unsigned g_5cbd, g_5de6, g_5dda, g_5de0;
extern FILE g_4408;
extern char g_0f80[];
extern ImageConfigRow g_alt_records[], g_ctrl_records[];

void auto_loop(void) {
    register int overlay_count;
    register unsigned attempts;

    g_5cc1 = 0;
    g_5cbb = 1;
    if (g_5cc5 < 1)
        g_5cc5 = 1;
    if (g_5cc3 < g_5cc5)
        g_5cc3 = g_5cc5;
    overlay_count = g_5cc5 + (long)rand() *
        (g_5cc3 + 1 - g_5cc5) / 32768L;
    while (!g_5d80 && !g_5da0) {
        if (g_5cbb++ > g_5cbd || g_5d92 == 110) {
            g_5cbb = 0;

            g_5cc1 = (long)rand() * (g_5d9c + g_5d9a + 1) / 32768L;
            attempts = 0;
            for (;;) {
                if (g_5cc1 < g_5d9c) {
                    if (g_alt_records[g_5cc1].size > 0)
                        break;
                } else if (g_ctrl_records[g_5cc1 - g_5d9c - 1].size > 0) {
                    break;
                }
                ++g_5cc1;
                g_5cc1 %= g_5d9c + g_5d9a + 1;
                if (attempts++ > g_5d9c + g_5d9a + 2) {
                    g_5d84 = g_5d80 = 1;
                    return;
                }
            }

            g_5cbf = (long)rand() * 2L / 32768L;
            if (g_5cc1 < g_5d9c) {
                g_5cbd = (long)rand() *
                        (g_alt_records[g_5cc1].size * g_5dae) / 32768L +
                    ((unsigned)(g_alt_records[g_5cc1].size * g_5dae) >> 2);
            } else {
                g_5cbd = (long)rand() *
                        (g_ctrl_records[g_5cc1 - g_5d9c - 1].size * g_5dae) /
                        32768L +
                    ((unsigned)(g_ctrl_records[g_5cc1 - g_5d9c - 1].size *
                                g_5dae) >> 2);
            }
            if (--overlay_count <= 0 || g_5d92 == 110) {
                reset_video_state();
                overlay_count = g_5cc5 + (long)rand() *
                    (g_5cc3 + 1 - g_5cc5) / 32768L;
                if (g_5de0)
                    overlay_count += (int)((long)rand() * g_5cc5 / 32768L);
                if (g_5dda)
                    overlay_count += (int)((long)rand() * g_5cc3 / 32768L);
                if (g_5de6 == 2 && overlay_count > 8)
                    overlay_count = 9;
            }

            if (g_5cc1 < g_5d9c) {
                g_5cb3 = g_alt_records[g_5cc1].independent_colors &&
                    (int)((long)rand() * (overlay_count + 1) / 32768L) == 0 ? 1 : 0;
            } else {
                g_5cb3 = g_ctrl_records[g_5cc1 - g_5d9c - 1].independent_colors &&
                    (int)((long)rand() * (overlay_count + 1) / 32768L) == 0 ? 1 : 0;
            }
            if (g_5d7e && (input_dispatcher() || g_5da0))
                return;
            update_draw_bounds(g_5cb3);
            if (g_5cc1 < g_5d9c) {
                g_5c4b = g_alt_records[g_5cc1].draw;
                g_5caf = g_alt_records[g_5cc1].multicolor &&
                         g_5cbf ? 3 : 1;
            } else {
                g_5c4b = g_ctrl_records[(g_5cc1 - g_5d9c - 1)].draw;
                /* The original also indexes the Alt table in this Ctrl branch. */
                g_5caf = g_alt_records[g_5cc1].multicolor &&
                         g_5cbf ? 3 : 1;
            }
            g_5c4b(1);
            g_5c3f(g_5caf);
            g_5cb1 = 0;
            fprintf(&g_4408, g_0f80);

        }
        if (g_5d7e && (input_dispatcher() || g_5da0))
            return;
        g_5c4b(0);
        g_5c3f(g_5caf);
        if (g_5d7a >= g_5da6)
            cycle_palette();
    }
}
