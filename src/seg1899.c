/* The two EGA palette entry points whose packaging is still unresolved.
 * C plus register intrinsics / inline port operations is a tested hypothesis;
 * the current candidates are PROVISIONAL, not accepted assembly transcriptions.
 * The other DVDT routines are in asm/seg1899.asm.
 */
#include <dos.h>
extern unsigned char background, reserved_one, reserved_two, border;

/* objcmp: {"function":"ega_upload_palette_bios","address":"1899:0c13","size":105,"level":"PROVISIONAL","blocker":"origin-ambiguous-C-versus-hand-assembly; redundant AH clears and unused SI save","symbols":{"_background":"DS:5d8a","_reserved_one":"DS:5d6c","_reserved_two":"DS:5d6e","_border":"DS:5d8c"}} */
void ega_upload_palette_bios(unsigned char *palette)
{
    int a,b,c,d;
    a=palette[0]; palette[0]=background;
    b=palette[1]; palette[1]=reserved_one;
    c=palette[2]; palette[2]=reserved_two;
    d=palette[16]; palette[16]=border;
    /* Native register intrinsics reproduce this BIOS call, but not the
     * original surrounding save/restore allocation. ES:BX currently names
     * palette in the tested BC30 build; the source-origin question stays open.
     */
    _DX=_BX; _AX=0x1002; geninterrupt(0x10);
    palette[16]=d; palette[2]=c; palette[1]=b; palette[0]=a;
}

/* objcmp: {"function":"ega_upload_palette_ports","address":"1899:0c7c","size":152,"level":"PROVISIONAL","blocker":"origin-ambiguous-C-versus-hand-assembly; ES:SI palette lifetime"} */
void ega_upload_palette_ports(unsigned char *palette)
{
    int a,b,c,d,i;
    a=palette[0]; palette[0]=background;
    b=palette[1]; palette[1]=reserved_one;
    c=palette[2]; palette[2]=reserved_two;
    d=palette[16]; palette[16]=border;
    /* CLI/STI and attribute flip-flop timing are direct assembly evidence.
     * These localized instructions are not a substitute for the C body.
     */
    asm sti;
    while(inportb(0x3da)&8);
    while(!(inportb(0x3da)&8));
    for(i=15;i>0;i--) {
        asm cli;
        outportb(0x3c0,i); outportb(0x3c0,palette[i]);
        asm sti;
    }
    asm cli;
    outportb(0x3c0,0); outportb(0x3c0,palette[0]);
    asm sti;
    asm cli;
    outportb(0x3c0,0x31); outportb(0x3c0,palette[16]);
    asm sti;
    palette[16]=d; palette[2]=c; palette[1]=b; palette[0]=a;
}
