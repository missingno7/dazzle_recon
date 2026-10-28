/* DVDT is reconstructed in asm/seg1899.asm, including the two EGA palette
 * entries at 0C13/0C7C. TASM 3.0 /ml /m2 verifies all 38 function extents.
 * C origin probes and their negative results are recorded in docs/asm-origin.md.
 * This file retains the public C declarations for the two palette interfaces.
 */
void far ega_upload_palette_bios(unsigned char far *palette);
void far ega_upload_palette_ports(unsigned char far *palette);
