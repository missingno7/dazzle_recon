/* Constants shared by the DAZZLE modules: palette, adapters, shape flags, BIOS/DOS
 * services, keyboard codes, text attributes, video hardware and the PCX format.
 * Defines only; including this header changes no compiled byte. */
#ifndef DAZZLE_DZCONST_H
#define DAZZLE_DZCONST_H

/* Palette indices 0..2 are reserved; generated colors cycle from index 3. */
#define FIRST_CYCLING_COLOR 3
/* DAZZLE's internal video adapter IDs (video_adapter). */
#define ADAPTER_EGA 4
#define ADAPTER_VGA 8

/* Shape drawing flags (polygon and ellipse images); the two color-mode layouts differ. */
#define SHAPE_OUTLINE 1
#define SHAPE_FILL 2
#define SHAPE_FILL_AND_OUTLINE 3
#define POLYGON_FILL_COLOR_MASK 12
#define POLYGON_FILL_COLOR_PER_PIXEL 4
#define POLYGON_FILL_COLOR_PER_ROW 8
#define POLYGON_EDGE_COLOR_PER_POINT 0x10
#define ELLIPSE_FILL_COLOR_MASK 0x1c
#define ELLIPSE_FILL_COLOR_PER_PIXEL 4
#define ELLIPSE_FILL_MIRRORED_COLORS 8
#define ELLIPSE_FILL_HORIZONTAL_COLOR_MASK 0x0c
#define ELLIPSE_FILL_COLOR_PER_ROW 0x10
#define ELLIPSE_EDGE_COLOR_PER_POINT 0x20

/* BIOS and DOS services. */
#define BIOS_KEYBOARD_INTERRUPT 0x16
#define DOS_IDLE_INTERRUPT 0x28
#define DOS_MULTIPLEX_INTERRUPT 0x2f
#define DOS_RELEASE_TIME_SLICE 0x1680

#define BIOS_KEY_F1 0x3b00
#define BIOS_KEY_F2 0x3c00
#define BIOS_KEY_F3 0x3d00
#define BIOS_KEY_F4 0x3e00
#define BIOS_KEY_F5 0x3f00
#define BIOS_KEY_F8 0x4200
#define BIOS_KEY_F9 0x4300
#define BIOS_KEY_F10 0x4400
#define BIOS_KEY_HOME 0x4700
#define BIOS_KEY_UP 0x4800
#define BIOS_KEY_PAGE_UP 0x4900
#define BIOS_KEY_LEFT 0x4b00
#define BIOS_KEY_RIGHT 0x4d00
#define BIOS_KEY_END 0x4f00
#define BIOS_KEY_DOWN 0x5000
#define BIOS_KEY_PAGE_DOWN 0x5100
#define BIOS_KEY_SHIFT_F10 0x5d00
#define BIOS_KEY_CTRL_F10 0x6700
#define BIOS_KEY_ALT_F9 0x7000
#define BIOS_KEY_ALT_F10 0x7100

#define MENU_ATTR_NORMAL 0x70
#define MENU_ATTR_SELECTED 0x0f
#define MENU_ATTR_FRAME 0x74
#define MENU_ATTR_ERROR 0x4e

#define TEXT_VIDEO_SEGMENT 0xb800
#define VGA_VIDEO_SEGMENT 0xa000
#define VGA_GC_INDEX_PORT 0x3ce
#define VGA_GC_DATA_PORT 0x3cf
#define BIOS_VIDEO_INTERRUPT 0x10
#define PCX_HEADER_BYTES 128
#define PCX_RLE_MARKER_MASK 0xc0
#define PCX_RLE_MARKER 0xc0
#define PCX_SINGLE_LITERAL_RUN 0xc1
#define PCX_MAX_RUN_LENGTH 63
#define PCX_VGA_PALETTE_BYTES 769
#define PCX_VGA_PALETTE_MARKER 12

#endif
