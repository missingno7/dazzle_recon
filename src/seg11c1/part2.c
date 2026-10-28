#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* objcmp: {"function":"initialize_math_tables","address":"11c1:1d64","size":113,"level":"UNKNOWN","blocker":"fp-emulator-stream at 11c1:1d77; source expressions for the 64-entry trig initializer are unresolved"} */
/* Builds the program's sine/cosine lookup tables; the emitted INT 34h..3Dh
   software-floating-point stream has not been reconstructed as C. */
void initialize_math_tables(void) { }

/* objcmp: {"function":"initialize_palette_mapping","address":"11c1:1dd5","size":340,"level":"UNKNOWN","blocker":"unresolved floating-point and far-buffer transformation at 11c1:1eb3"} */
/* Builds the per-image mapping passed from configure_video_and_buffers. */
void far initialize_palette_mapping(unsigned far *mapping) { (void)mapping; }

extern unsigned g_7488, g_5dc6, g_5caf;
extern void (*g_01fe)(void);
extern void (*g_5c4b)(void);
extern int g_5d90, g_5d92, g_5cb1;
extern unsigned g_5d80;
extern char far *g_5ffa;
extern void (*g_5c43)(void);
extern void (*g_5c3f)(void);
extern char g_5c5f[];
extern unsigned g_5cc5, g_5cc3, g_5da8, g_5da4, g_5de0;
extern void initialize_math_tables(void);
extern void delay(unsigned);
extern void far backend_1899_0aa5(void);
extern void far backend_1899_02e8(void);

/* objcmp: {"function":"initialize_program_defaults","address":"11c1:1f29","size":150,"symbols":{"_g_7488":"DS:7488","_g_5dc6":"DS:5dc6","_g_5caf":"DS:5caf","_g_01fe":"DS:01fe","_g_5c4b":"DS:5c4b","_g_5d90":"DS:5d90","_g_5d92":"DS:5d92","_g_5cb1":"DS:5cb1","_g_5d80":"DS:5d80","_initialize_math_tables":"11c1:1d64","_g_5ffa":"DS:5ffa","_g_5c43":"DS:5c43","_g_5c3f":"DS:5c3f","_g_5c5f":"DS:5c5f","_strcpy":"0000:59cc","_g_5cc5":"DS:5cc5","_g_5cc3":"DS:5cc3","_g_5da8":"DS:5da8","_g_5da4":"DS:5da4","_g_5de0":"DS:5de0","_delay":"0000:29a6","_backend_1899_0aa5":"1899:0aa5","_backend_1899_02e8":"1899:02e8"},"segments":{"_DATA":"DS:200f"}} */
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

/* objcmp: {"function":"beep_float","address":"11c1:1fbf","size":48,"symbols":{"_sound":"0000:3d88","_delay":"0000:29a6","_nosound":"0000:3db4","F_FTOL@":"0000:0fbe"},"segments":{"_DATA":"DS:200f"},"absolute_symbols":{"FIDRQQ":"0x5c32"}} */
void sound(unsigned); void delay(unsigned); void nosound(void);
void beep_float(float hz, unsigned ms) {
    int f;
    f = hz + 0.5;
    sound(f);
    delay(ms);
    nosound();
}

/* objcmp: {"function":"play_tone_sequence","address":"11c1:1fef","size":370,"level":"UNKNOWN","blocker":"fp-emulator-stream at 11c1:2030; the generated tone matrix and note expression are unresolved"} */
/* Builds an eight-octave note matrix, plays the selected notes through
   beep_float, and stops early for keyboard input or the mouse timer. */
extern unsigned g_5dbe, g_5d7c;
void play_tone_sequence(void) {
    if (!g_5dbe) {
        /* Floating-point emulator expansion unresolved. */
    }
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

/* objcmp: {"function":"configure_video_backend","address":"11c1:21a6","size":879,"level":"PROVISIONAL","blocker":"extent 814 vs 879; allocation and error-path shape is not byte matched","symbols":{"_g_5d82":"DS:5d82","_g_5c33":"DS:5c33","_g_743c":"DS:743c","_g_743a":"DS:743a","_g_7438":"DS:7438","_g_7484":"DS:7484","_g_5da2":"DS:5da2","_g_5dd4":"DS:5dd4","_g_5da6":"DS:5da6","_g_5dba":"DS:5dba","_g_5db6":"DS:5db6","_g_5dc0":"DS:5dc0","_g_5db0":"DS:5db0","_g_5de4":"DS:5de4","_g_5c43":"DS:5c43","_g_5c3f":"DS:5c3f","_g_5c3b":"DS:5c3b","_g_5db4":"DS:5db4","_g_5dae":"DS:5dae","_g_5c47":"DS:5c47","_g_5dd8":"DS:5dd8","_g_5c37":"DS:5c37","_g_5c49":"DS:5c49","_g_5d88":"DS:5d88","_g_5db8":"DS:5db8","_g_5d94":"DS:5d94","_g_5d96":"DS:5d96","_g_5d80":"DS:5d80","_g_5c57":"DS:5c57","_g_5c59":"DS:5c59","_g_5c4f":"DS:5c4f","_g_5c51":"DS:5c51","_g_5c53":"DS:5c53","_g_5c55":"DS:5c55","_set_display_start":"1899:0ebf","_vga_mode_tall":"1899:0b9e","_vga_mode_standard":"1899:0afc","_vga_plot_tall":"1899:0622","_vga_plot_standard":"1899:048b","_vga_read_tall":"1899:08f1","_vga_read_standard":"1899:088b","_vga_palette_builder":"11c1:13ef","_vga_palette_writer_bios_split":"1899:0d27","_vga_palette_writer_split":"1899:0dee","_vga_palette_writer_fast":"1899:0d74","_vga_palette_writer_bios":"1899:0d14","_vga_palette_writer_x":"1899:0dbf","_vga_palette_writer":"1899:0d48","_ega_mode":"1899:0aa5","_ega_plot_tall":"1899:02e8","_ega_plot_standard":"1899:012d","_ega_read_tall":"1899:07f5","_ega_read_standard":"1899:075d","_ega_palette_writer_bios":"1899:0c13","_ega_palette_writer":"1899:0c7c","_ega_palette_builder":"11c1:0e45","_malloc":"0000:2c51","_rand":"0000:1694","_video_error":"11c1:0d46","_initialize_palette_mapping":"11c1:1dd5"}} */
typedef void (far *VideoCallback)(void);
typedef unsigned (far *ReadPixelCallback)(int, int);
typedef void (far *PaletteCallback)(unsigned, unsigned);
extern void (*g_5c33)(unsigned, unsigned);
extern VideoCallback g_5c41, g_5c43, g_5c47;
extern ReadPixelCallback g_5c3b;
extern PaletteCallback g_5c37;
extern unsigned g_5d82, g_7438, g_743a, g_743c;
extern unsigned g_5da2, g_5dd4, g_5da6, g_5dba, g_5db6, g_5dc0;
extern unsigned g_5db0, g_5db2, g_5de4, g_5db4, g_5dae, g_5dd8;
extern unsigned g_5d88, g_5db8, g_5d80, g_5c49;
extern unsigned g_5c57, g_5c59, g_5c4f, g_5c51, g_5c53, g_5c55;
extern unsigned g_7484, g_5d94, g_5d96;
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
extern void far initialize_palette_mapping(unsigned far *);

union FarAddress {
    void far *pointer;
    unsigned word[2];
};

void configure_video_backend(int allocate) {
    unsigned palette_bytes;
    unsigned divisor;
    union FarAddress allocation;

    if (allocate) g_5d82 = 0;
    g_5c33 = set_display_start;
    g_743c = 0x2f;
    g_743a = 0x3f;
    g_7438 = 0x1f;
    if (g_7484 == 8) {
        g_5da2 = 1;
        if (allocate) g_5da6 = g_5dd4 ? 4 : 1;
        g_5dba = 576;
        g_5db6 = 256;
        g_5dc0 = g_5db6 - 1;
        g_5db0 = 160;
        if (g_5de4) {
            g_5db2 = 100;
            g_5c43 = (VideoCallback)vga_mode_tall;
            g_5c3f = (VideoCallback)vga_plot_tall;
            g_5c3b = vga_read_tall;
        } else {
            g_5db2 = 200;
            g_5c43 = (VideoCallback)vga_mode_standard;
            g_5c3f = (VideoCallback)vga_plot_standard;
            g_5c3b = vga_read_standard;
        }
        if (g_5db4 || g_5dd8 || g_5dd4)
            g_5dae = g_5db2 - 28;
        else
            g_5dae = g_5db2 + 1;
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
    } else {
        g_5da2 = 2;
        if (allocate) g_5da6 = 2;
        g_5dba = 64;
        g_5db6 = 16;
        g_5dc0 = g_5db6 - 1;
        g_5db0 = 320;
        if (g_5de4) {
            g_5db2 = 175;
            g_5c3f = (VideoCallback)ega_plot_tall;
            g_5c3b = ega_read_tall;
        } else {
            g_5db2 = 350;
            g_5c3f = (VideoCallback)ega_plot_standard;
            g_5c3b = ega_read_standard;
        }
        g_5dae = g_5db2 + 1;
        g_5c43 = ega_mode;
        g_5c47 = ega_palette_builder;
        g_5c37 = g_5db4 ? ega_palette_writer_bios : ega_palette_writer;
    }
    g_5d88 = 0xffff / (g_5dae + g_5dae / 4) - 1;
    g_5db8 = g_5dba * 3;
    if (!allocate) return;

    palette_bytes = g_5db6 + g_5dba;
    if (g_7484 != 4) palette_bytes *= 3;
    allocation.pointer = malloc(palette_bytes + 3);
    g_5d94 = allocation.word[0];
    g_5d96 = allocation.word[1];
    if (!allocation.pointer) {
        g_5d80 = 1;
        video_error(0x3c);
    }
    allocation.pointer = malloc(0x7d8);
    g_5c57 = allocation.word[0];
    g_5c59 = allocation.word[1];
    if (!allocation.pointer) {
        g_5d80 = 1;
        video_error(0x46);
    }
    divisor = rand() % 5 + 2;
    g_5c4f = g_5db2 / divisor;
    g_5c51 = g_5db0 / divisor;
    allocation.pointer = malloc(g_5c4f * g_5c51);
    g_5c53 = allocation.word[0];
    g_5c55 = allocation.word[1];
    if (!allocation.pointer) {
        g_5d80 = 1;
        video_error(0x47);
    }
    initialize_palette_mapping((unsigned far *)&g_5c4f);
}

/* objcmp: {"function":"cycle_palette","address":"11c1:2515","size":89,"level":"PROVISIONAL","blocker":"callback argument order at +0x40; compiler places the offset in DX and emits one extra byte","symbols":{"_g_0bfe":"DS:0bfe","_g_0c00":"DS:0c00","_g_7484":"DS:7484","_g_5dec":"DS:5dec","_g_5dba":"DS:5dba","_g_5da4":"DS:5da4","_g_5db8":"DS:5db8","_g_5d94":"DS:5d94","_g_5d96":"DS:5d96","_g_5c37":"DS:5c37","_g_5d7a":"DS:5d7a"}} */
extern unsigned g_0bfe, g_0c00, g_7484;
extern unsigned g_5dec, g_5dba, g_5da4, g_5db8;
extern unsigned g_5d94, g_5d96, g_5d7a;
extern void (*g_5c37)(unsigned, unsigned);

void cycle_palette(void) {
    register unsigned offset;
    if (!g_0bfe || g_0c00) {
        if (g_7484 == 4) {
            ++g_5dec;
            if (g_5dec >= g_5dba) g_5dec = 3;
        } else {
            g_5dec = g_5dec + g_5da4;
            if (g_5dec >= g_5db8) g_5dec = 9;
        }
        offset = g_5d94 + g_5dec;
        g_5c37(offset, g_5d96);
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

/* objcmp: {"function":"save_configuration_file","address":"11c1:2b44","size":482,"level":"PROVISIONAL","blocker":"extent 489 vs 482; final error/return code layout differs","symbols":{"_g_config_write_mode":"DS:20fc","_g_config_save_error":"DS:20ff","_g_5ffe":"DS:5ffe","_g_config_header":"DS:211d","_g_alt_write_format":"DS:223d","_g_ctrl_write_format":"DS:2259","_g_fade_write_format":"DS:2275","_g_min_write_format":"DS:228f","_g_max_write_format":"DS:22bd","_g_fade_delay_write_format":"DS:22eb","_g_alt_records":"DS:0202","_g_ctrl_records":"DS:041e","_g_fade_records":"DS:0474","_g_5d9c":"DS:5d9c","_g_5d9a":"DS:5d9a","_g_5d98":"DS:5d98","_g_5cc5":"DS:5cc5","_g_5cc3":"DS:5cc3","_g_5da8":"DS:5da8","_fopen":"0000:459f","_fprintf":"0000:45d4","_fclose":"0000:407f","_report_config_error":"158d:2492"}} */
extern char g_config_write_mode[], g_config_save_error[];
extern char g_config_header[], g_alt_write_format[], g_ctrl_write_format[];
extern char g_fade_write_format[], g_min_write_format[], g_max_write_format[];
extern char g_fade_delay_write_format[];
extern unsigned g_5d9c, g_5d9a, g_5d98;
extern unsigned g_5cc5, g_5cc3, g_5da8;
extern void far report_config_error(char far *);

typedef struct {
    unsigned size;
    unsigned name_offset;
    unsigned name_segment;
    unsigned char gap[4];
    char key;
    unsigned char tail[4];
} ImageConfigRow;

typedef struct {
    char key;
    unsigned char name_offset[2];
    unsigned char name_segment[2];
    unsigned char enabled[2];
} FadeConfigRow;

extern ImageConfigRow g_alt_records[];
extern ImageConfigRow g_ctrl_records[];
extern FadeConfigRow g_fade_records[];

int save_configuration_file(char far *path) {
    unsigned i;

    if (!(g_5ffe = fopen(path, g_config_write_mode))) {
        report_config_error(g_config_save_error);
        goto error_return;
    }
    if (fprintf(g_5ffe, g_config_header) == -1) goto write_error;
    for (i = 0; i < g_5d9c; ++i) {
        if (fprintf(g_5ffe, g_alt_write_format, g_alt_records[i].key,
                    g_alt_records[i].size,
                    *(char far * far *)&g_alt_records[i].name_offset) == -1)
            goto write_error;
    }
    for (i = 0; i < g_5d9a; ++i) {
        if (fprintf(g_5ffe, g_ctrl_write_format, g_ctrl_records[i].key,
                    g_ctrl_records[i].size,
                    *(char far * far *)&g_ctrl_records[i].name_offset) == -1)
            goto write_error;
    }
    for (i = 0; i < g_5d98; ++i) {
        if (fprintf(g_5ffe, g_fade_write_format, g_fade_records[i].key,
                    *(unsigned *)&g_fade_records[i].enabled[0] ? 'y' : 'n',
                    *(char far * far *)&g_fade_records[i].name_offset[0]) == -1)
            goto write_error;
    }
    if (fprintf(g_5ffe, g_min_write_format, g_5cc5) == -1) goto write_error;
    if (fprintf(g_5ffe, g_max_write_format, g_5cc3) == -1) goto write_error;
    if (fprintf(g_5ffe, g_fade_delay_write_format, g_5da8 / 18) == -1)
        goto write_error;
    fclose(g_5ffe);
    return 0;

write_error:
    fclose(g_5ffe);
error_return:
    return 1;
}

/* objcmp: {"function":"resolve_startup_configuration","address":"11c1:2d26","size":455,"level":"UNKNOWN","blocker":"far-path split/merge and filename-validation flow at 11c1:2d88 is not source reconstructed","symbols":{"_g_5c5f":"DS:5c5f","_g_5ffa":"DS:5ffa","_g_5ffc":"DS:5ffc","_g_5d17":"DS:5d17","_load_configuration_file":"11c1:29ae","_getcwd":"0000:4b1f","_fnsplit":"0000:439f","_strcat":"0000:5950","_strcpy":"0000:59cc","_setdisk":"0000:128e","_report_config_error":"158d:2492"}} */
/* Resolves the command-line/configuration filename, fills in its directory and
   extension, validates it, and calls load_configuration_file. */

/* objcmp: {"function":"parse_command_line","address":"11c1:2eed","size":1202,"level":"UNKNOWN","blocker":"27-case dispatch and CS tables at 11c1:3333/3369 are not source reconstructed","symbols":{"_getswitchar":"0000:12ad","_tolower":"0000:1f66","_cprintf":"0000:277a","_exit":"0000:11a8","_g_5c5f":"DS:5c5f","_g_5dc8":"DS:5dc8","_g_5dd8":"DS:5dd8","_g_5dd0":"DS:5dd0","_g_5dd4":"DS:5dd4","_g_5de0":"DS:5de0","_g_5dce":"DS:5dce","_g_5d86":"DS:5d86","_g_7480":"DS:7480","_g_7482":"DS:7482"}} */
/* Walks far argv strings, recognizes concatenated DOS switch characters, and
   dispatches the 27 option keys through the adjacent CS-relative tables. */

/* objcmp: {"function":"print_startup_information","address":"11c1:339f","size":410,"level":"UNKNOWN","blocker":"conditional cprintf argument/string layout at 11c1:339f is not source reconstructed","symbols":{"_cprintf":"0000:277a","_g_7484":"DS:7484","_g_5dbc":"DS:5dbc","_g_5dca":"DS:5dca"}} */
/* Prints the program version, selected video mode, driver and mouse status,
   subject to the Show, Window, and Bashful flags. */

/* objcmp: {"function":"semi_auto_loop","address":"11c1:3539","size":882,"level":"UNKNOWN","blocker":"mode state machine and six-entry CS dispatch table at 11c1:38ab are not source reconstructed","symbols":{"_g_5d9a":"DS:5d9a","_g_5d9c":"DS:5d9c","_g_5d92":"DS:5d92","_g_5d9e":"DS:5d9e","_g_5da0":"DS:5da0","_g_5cc1":"DS:5cc1","_g_5c4b":"DS:5c4b","_g_5c3f":"DS:5c3f","_cycle_palette":"11c1:2515","_bioskey":"0000:2553","_rand":"0000:1694","_input_dispatcher":"11c1:18e7"}} */
/* Semi-Auto selects normalized Alt/Ctrl image records, services F8/F9/F10,
   handles the mode command table, and returns on mode change or exit. */

/* objcmp: {"function":"auto_loop","address":"11c1:38c3","size":1025,"level":"UNKNOWN","blocker":"random pattern selection and draw-state loop at 11c1:391e is not source reconstructed","symbols":{"_g_5cc1":"DS:5cc1","_g_5cbb":"DS:5cbb","_g_5cc3":"DS:5cc3","_g_5cc5":"DS:5cc5","_g_5c4b":"DS:5c4b","_g_5c3f":"DS:5c3f","_g_5caf":"DS:5caf","_g_5d80":"DS:5d80","_g_5da0":"DS:5da0","_g_5d9c":"DS:5d9c","_g_5da8":"DS:5da8","_cycle_palette":"11c1:2515","_rand":"0000:1694","_input_dispatcher":"11c1:18e7"}} */
/* Auto clamps the requested image count, chooses enabled image records and
   overlays, draws through DS:5C4B, and services palette/input state. */
