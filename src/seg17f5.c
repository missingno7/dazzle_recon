#include <dos.h>

/* objcmp: {"function":"current_code_segment","address":"17f5:0008","size":3,"level":"UNKNOWN","blocker":"asm-origin"} */
/* Assembly description: MOV AX,CS; RETF. This short helper returns the active code segment. */

extern unsigned g_5d94;
extern unsigned g_5d96;

/* objcmp: {"function":"upload_reserved_dac_entries","address":"17f5:030f","size":62,"symbols":{"_g_5d94":"DS:5d94","_g_5d96":"DS:5d96","_int86x":"0000:323f"}} */
void far upload_reserved_dac_entries(unsigned count)
{
    union REGS regs;
    struct SREGS segs;

    regs.x.ax = 0x1012;
    regs.x.bx = 0;
    regs.x.cx = count;
    regs.x.dx = g_5d94;
    segs.es = g_5d96;
    int86x(0x10, &regs, &regs, &segs);
}

struct image_header {
    unsigned char reserved[0x42];
    unsigned output_size;
};

extern struct image_header far *g_747c;
extern unsigned g_39aa;
extern unsigned g_39ac;
extern unsigned g_746c;
extern unsigned g_746e;
extern unsigned char far *g_745a[2];

extern unsigned g_7456;
extern unsigned g_7466;
extern unsigned g_7468;

/* objcmp: {"function":"dim_logo_pixels","address":"17f5:034d","size":168,"symbols":{"_g_745a":"DS:745a","_g_7456":"DS:7456","_g_7466":"DS:7466","_g_7468":"DS:7468","_g_39aa":"DS:39aa"},"level":"PROVISIONAL","blocker":"register-and-loop-layout at 17f5:0354"} */
void far dim_logo_pixels(void)
{
    register unsigned frame;
    unsigned index;
    unsigned pixel;
    char far *source;

    if (g_7466) {
        if (g_7468 < 0x301) {
            index = g_7456 - 0x300 + g_7468;
            frame = 0;
            for (pixel = 0; pixel < 0x300; ++pixel) {
                source = g_745a[frame] + index;
                *source >>= 1;
                *source >>= 1;
                ++index;
                if (index >= g_39aa) {
                    index = 0;
                    frame = 1;
                }
            }
        } else {
            source = g_745a[1] + g_7468 - 0x300;
            for (pixel = 0; pixel < 0x300; ++pixel) {
                *source >>= 1;
                *source >>= 1;
                ++source;
            }
        }
    } else {
        source = g_745a[0] + g_7468 - 0x300;
        for (pixel = 0; pixel < 0x300; ++pixel) {
            *source >>= 1;
            *source >>= 1;
            ++source;
        }
    }
}

extern void far draw_logo_part(unsigned char far *destination,
                               unsigned char far *source, unsigned mask);
extern void far copy_logo_part(unsigned char far *source,
                               unsigned char far *destination, unsigned mask);

/* objcmp: {"function":"prepare_logo_chunk","address":"17f5:03f5","size":92,"symbols":{"_g_747c":"DS:747c","_g_5d94":"DS:5d94","_g_5d96":"DS:5d96","_draw_logo_part":"1899:0006","_copy_logo_part":"0000:4cee"}} */
void far prepare_logo_chunk(void)
{
    draw_logo_part((unsigned char far *)MK_FP(g_5d96, g_5d94),
                   (unsigned char far *)g_747c + 0x10,
                   1 << g_747c->reserved[0x41]);
    copy_logo_part((unsigned char far *)g_747c + 0x10,
                   (unsigned char far *)MK_FP(g_5d96, g_5d94),
                   1 << g_747c->reserved[0x41]);
    g_747c->reserved[0x20] = 0;
}

/* objcmp: {"function":"decode_logo_run","address":"17f5:0451","size":203,"symbols":{"_g_747c":"DS:747c","_g_39aa":"DS:39aa","_g_39ac":"DS:39ac","_g_746c":"DS:746c","_g_746e":"DS:746e","_g_745a":"DS:745a"},"level":"PROVISIONAL","blocker":"branch-layout at 17f5:045c"} */
void far decode_logo_run(unsigned char far *destination)
{
    register unsigned output_offset = 0;
    register unsigned char code;
    register unsigned repeat;

    if (g_39ac) {
        output_offset = g_39ac - g_747c->output_size;
        g_39ac = 0;
    }

    while (output_offset < g_747c->output_size) {
        code = g_745a[g_746e][g_746c++];
        if (g_746c >= g_39aa) {
            g_746e = 1;
            g_746c = 0;
        }
        if ((code & 0xc0) == 0xc0) {
            repeat = code & 0x3f;
            code = g_745a[g_746e][g_746c++];
            if (g_746c >= g_39aa) {
                g_746e = 1;
                g_746c = 0;
            }
            while (repeat) {
                destination[output_offset++] = code;
                --repeat;
            }
        } else {
            destination[output_offset++] = code;
        }
    }

    if (g_747c->output_size < output_offset)
        g_39ac = output_offset;
    else
        g_39ac = 0;
}

extern void far copy_logo_plane(unsigned destination_offset,
                                unsigned destination_segment,
                                unsigned char far *source,
                                unsigned operation);

/* objcmp: {"function":"configure_logo_dac","address":"17f5:09e1","size":100,"symbols":{"_g_747c":"DS:747c","_g_5d94":"DS:5d94","_g_5d96":"DS:5d96","_int86x":"0000:323f","_copy_logo_plane":"0000:4cee"},"level":"PROVISIONAL","blocker":"duplicate-register-address at 17f5:0a31"} */
void far configure_logo_dac(void)
{
    union {
        union REGS inregs;
        union REGS outregs;
    } regs;
    struct SREGS segs;

    copy_logo_plane(g_5d94, g_5d96,
                    (unsigned char far *)g_747c + 0x10, 0x11);
    regs.inregs.x.ax = 0x1002;
    regs.inregs.x.bx = 0;
    regs.inregs.x.dx = FP_OFF(g_747c) + 0x10;
    segs.es = FP_SEG((unsigned char far *)g_747c + 0x10);
    int86x(0x10, &regs.inregs, &regs.outregs, &segs);
}
