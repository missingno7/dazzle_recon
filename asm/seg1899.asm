; DVDT hardware and plasma kernels. TASM 3.0 /ml /m2. See docs/asm-origin.md.
; Hand-authored assembly evidence: parity branches, latch reads, explicit FLAGS/DS,
; ENTER/LEAVE fast palette writers and separate 8086 versions, manual call slots.
.286
VGA_SEQ_INDEX equ 03c4h
VGA_SEQ_DATA equ 03c5h
VGA_GC_INDEX equ 03ceh
VGA_GC_DATA equ 03cfh
VGA_CRTC_INDEX equ 03d4h
VGA_STATUS equ 03dah
VGA_ATTR equ 03c0h
VGA_DAC_WRITE equ 03c8h
VGA_DAC_DATA equ 03c9h
VGA_MEMORY equ 0a000h
PAL_ARGUMENT equ 6
PAL_OLD0 equ -2
PAL_OLD1 equ -4
PAL_OLD2 equ -6
PAL_OLD_BORDER equ -8
_DATA segment word public 'DATA'
extrn _background_fill_word:WORD
extrn _countdown_ticks:WORD
extrn _g_5d92:WORD
extrn _palette_cycle_ticks:WORD
extrn _draw_delay_count:WORD
extrn _exit_requested:WORD
extrn _current_image_header:WORD
extrn _mouse_callback_installed:WORD
extrn _palette_base_green:WORD
extrn _palette_base_blue:WORD
extrn _g_5d8c:WORD
extrn _color_count:WORD
extrn _screen_half_height:WORD
extrn _screen_half_width:WORD
extrn _mouse_abort_pending:WORD
extrn _saved_keyboard_vector:WORD
extrn _saved_timer_vector:WORD
extrn _palette_buffer:WORD
extrn _palette_elapsed_ticks:WORD
extrn _input_pending:WORD
extrn _plot_callback:WORD
extrn _draw_x:WORD
extrn _draw_y:WORD
extrn _read_pixel_callback:WORD
extrn _g_5d86:WORD

; Mode flags and DOS diagnostics; these are data, not encoded instructions.
public _video_mode_cookie, _symmetry_edge, _symmetry_y_edge
_video_mode_cookie dw 0
_symmetry_edge db 0
_symmetry_y_edge db 0
mono_ega_error db 'Mono EGA is not supported',13,10,7,'$'
mono_vga_error db 'Mono VGA is not supported',13,10,7,'$'
color_adapter_error db 'Color EGA or VGA required, sorry',13,10,7,'$'
disk_one_message db ' Hard disk 1 $'
disk_two_message db 'and 2 $'
disk_park_message db 'retracted for your convenience.',13,10,'$'
public _disk_count, _first_disk_cylinders, _second_disk_cylinders, _input_poll_divider
_disk_count db 0
_first_disk_cylinders dw 0
_second_disk_cylinders dw 0
_input_poll_divider dw 5
_DATA ends
_BSS segment word public 'BSS'
public _force_ega
_force_ega dw ?
public _force_vga
_force_vga dw ?
public _video_adapter
_video_adapter dw ?
public _saved_bios_mode
_saved_bios_mode dw ?
public _plot_color
_plot_color dw ?
public _saved_cursor_shape
_saved_cursor_shape dw ?
public _saved_text_last_row
_saved_text_last_row dw ?
public _ega_memory_code
_ega_memory_code dw ?
public _page_paragraph
_page_paragraph dw ?
public _plasma_random
_plasma_random dw ?
public _plasma_width
_plasma_width dw ?
public _plasma_height
_plasma_height dw ?
public _plasma_color_limit
_plasma_color_limit dw ?
public _ega_row_offsets
_ega_row_offsets dw 350 dup (?)
_BSS ends
DGROUP group _DATA, _BSS
assume ds:DGROUP
extrn _print_closing_text_and_exit:FAR
extrn _poll_graphics_keys:FAR
extrn _cycle_palette:FAR
DVDT6 segment byte public 'CODE'
assume cs:DVDT6

; (far dst, far RGB triples, count): quantize 8-bit RGB into EGA register bits.
comment !
/* objcmp: {
 "function": "rgb8_to_ega_palette",
 "address": "1899:0006",
 "size": 134,
 "symbols": {
  "_ega_row_offsets": "DS:749a",
  "_page_paragraph": "DS:7490",
  "_draw_delay_count": "DS:5dc6",
  "_plot_color": "DS:7488",
  "_symmetry_edge": "DS:3bc8",
  "_screen_half_width": "DS:5db0",
  "_draw_x": "DS:5df0",
  "_screen_half_height": "DS:5db2",
  "_draw_y": "DS:5dee",
  "_symmetry_y_edge": "DS:3bc9",
  "_saved_bios_mode": "DS:7486",
  "_saved_cursor_shape": "DS:748a",
  "_saved_text_last_row": "DS:748c",
  "_video_adapter": "DS:7484",
  "_ega_memory_code": "DS:748e",
  "_force_ega": "DS:7480",
  "_force_vga": "DS:7482",
  "_print_closing_text_and_exit": "11c1:0d46",
  "_video_mode_cookie": "DS:3bc6",
  "_background_fill_word": "DS:5d8a",
  "_palette_buffer": "DS:5d94",
  "_palette_base_green": "DS:5d6c",
  "_palette_base_blue": "DS:5d6e",
  "_g_5d8c": "DS:5d8c",
  "_disk_count": "DS:3c5f",
  "_first_disk_cylinders": "DS:3c60",
  "_second_disk_cylinders": "DS:3c62",
  "_palette_cycle_ticks": "DS:5da6",
  "_palette_elapsed_ticks": "DS:5d7a",
  "_cycle_palette": "11c1:2515",
  "_input_pending": "DS:5d7e",
  "_poll_graphics_keys": "11c1:18e7",
  "_g_5d92": "DS:5d92",
  "_read_pixel_callback": "DS:5c3b",
  "_plot_callback": "DS:5c3f",
  "_plasma_random": "DS:7492",
  "_plasma_color_limit": "DS:7498",
  "_plasma_width": "DS:7494",
  "_plasma_height": "DS:7496",
  "_color_count": "DS:5db6",
  "_countdown_ticks": "DS:5d78",
  "_g_5d86": "DS:5d86",
  "_input_poll_divider": "DS:3c64",
  "_mouse_callback_installed": "DS:5cb9",
  "_current_image_header": "DS:5c5b",
  "_exit_requested": "DS:5d80",
  "_saved_timer_vector": "DS:5ff6",
  "_saved_keyboard_vector": "DS:5ff2",
  "_mouse_abort_pending": "DS:5d7c"
 },
 "segments": {
  "_DATA": "29bf:3bc6", "_BSS": "29bf:7480"
 }
} */
!
public _rgb8_to_ega_palette
_rgb8_to_ega_palette label far
rgb8_to_ega_palette label far
    push bp
    mov bp, sp
    push di
    push si
    push ds
    les di, dword ptr [bp + 6]
    lds si, dword ptr [bp + 0ah]
    mov cx, word ptr [bp + 0eh]
    sub bx, bx
rgb_next_color:
    mov byte ptr es:[di], 0
    cmp byte ptr [bx + si], 033h
    jbe rgb_green
    or byte ptr es:[di], 020h
    cmp byte ptr [bx + si], 077h
    jbe rgb_green
    and byte ptr es:[di], 0dfh
    or byte ptr es:[di], 4
    cmp byte ptr [bx + si], 0bbh
    jbe rgb_green
    or byte ptr es:[di], 024h
rgb_green:
    nop
    cmp byte ptr [bx + si + 1], 033h
    jbe rgb_blue
    or byte ptr es:[di], 010h
    cmp byte ptr [bx + si + 1], 077h
    jbe rgb_blue
    and byte ptr es:[di], 0efh
    or byte ptr es:[di], 2
    cmp byte ptr [bx + si + 1], 0bbh
    jbe rgb_blue
    or byte ptr es:[di], 012h
rgb_blue:
    nop
    cmp byte ptr [bx + si + 2], 033h
    jbe rgb_advance
    or byte ptr es:[di], 8
    cmp byte ptr [bx + si + 2], 077h
    jbe rgb_advance
    and byte ptr es:[di], 0f7h
    or byte ptr es:[di], 1
    cmp byte ptr [bx + si + 2], 0bbh
    jbe rgb_advance
    or byte ptr es:[di], 9
rgb_advance:
    nop
    add bx, 3
    inc di
    loop rgb_next_color
    pop ds
    pop si
    pop di
    pop bp
    retf

; (far planar bytes, byte count, row): copy the four planes and restore map mask.
comment !
/* objcmp: {
 "function": "ega_write_planar_row",
 "address": "1899:008c",
 "size": 128
} */
!
public _ega_write_planar_row
_ega_write_planar_row label far
ega_write_planar_row label far
    push bp
    mov bp, sp
    push di
    push si
    push ds
    lds si, dword ptr [bp + 6]
    mov dx, word ptr [bp + 0ch]
    cmp dx, 015eh
    jge row_transfer_done
    push dx
    push si
    push ds
    push ax
    mov ax, DGROUP
    mov ds, ax
    pop ax
    mov si, offset _ega_row_offsets
    shl dx, 1
    add si, dx
    mov di, word ptr [si]
    pop ds
    pop si
    pop dx
    mov ax, VGA_MEMORY
    mov es, ax
    mov bx, word ptr [bp + 0ah]
    mov cx, bx
    mov al, 2
    mov dx, VGA_SEQ_INDEX
    out dx, al
    inc dx
    mov al, 1
    out dx, al
    cld
    push di
    repne movsb
    pop di
    mov cx, bx
    mov al, 2
    mov dx, VGA_SEQ_INDEX
    out dx, al
    inc dx
    mov al, 2
    out dx, al
    push di
    repne movsb
    pop di
    mov cx, bx
    mov al, 2
    mov dx, VGA_SEQ_INDEX
    out dx, al
    inc dx
    mov al, 4
    out dx, al
    push di
    repne movsb
    pop di
    mov cx, bx
    mov al, 2
    mov dx, VGA_SEQ_INDEX
    out dx, al
    inc dx
    mov al, 8
    out dx, al
    push di
    repne movsb
    pop di
    mov al, 2
    mov dx, VGA_SEQ_INDEX
    out dx, al
    inc dx
    mov al, 0fh
    out dx, al
row_transfer_done:
    pop ds
    pop si
    pop di
    pop bp
    retf

; Build the 350-row stride-80 table, then enter BIOS mode 10h.
comment !
/* objcmp: {
 "function": "ega_init_row_table_and_mode",
 "address": "1899:010c",
 "size": 33
} */
!
public _ega_init_row_table_and_mode
_ega_init_row_table_and_mode label far
ega_init_row_table_and_mode label far
    push di
    push si
    mov cx, 015eh
    sub dx, dx
    mov si, offset _ega_row_offsets
row_offset_next:
    push dx
    mov ax, 050h
    mul dx
    mov word ptr [si], ax
    add si, 2
    pop dx
    inc dx
    loop row_offset_next
    mov ax, 010h
    int 010h
    pop si
    pop di
    retf

; (symmetry): half-height EGA; reject odd virtual rows before halving Y.
comment !
/* objcmp: {
 "function": "ega_plot_symmetric_640x700",
 "address": "1899:012d",
 "size": 443
} */
!
public _ega_plot_symmetric_640x700
_ega_plot_symmetric_640x700 label far
ega_plot_symmetric_640x700 label far
    push bp
    mov bp, sp
    sub sp, 8
    push si
    push di
    mov ax, word ptr [_page_paragraph]
    add ax, VGA_MEMORY
    mov es, ax
    mov cx, word ptr [_draw_delay_count]
ega700_delay:
    and ch, 01fh
    loop ega700_delay
    mov bl, byte ptr [_plot_color]
    mov dx, VGA_GC_INDEX
    xor ax, ax
    mov word ptr [_symmetry_edge], ax
    mov ax, word ptr [_screen_half_width]
    mov si, word ptr [_draw_x]
    cmp ax, si
    jg ega700_sum_x
    inc byte ptr [_symmetry_edge]
ega700_sum_x:
    add ax, si
    js ega700_outside
    mov word ptr [bp - 2], ax
    mov ax, word ptr [_screen_half_width]
    sub ax, si
    js ega700_outside
    mov word ptr [bp - 4], ax
    mov ax, word ptr [_screen_half_height]
    mov si, word ptr [_draw_y]
    cmp ax, si
    jg ega700_sum_y
    inc byte ptr [_symmetry_y_edge]
ega700_sum_y:
    add ax, si
    js ega700_outside
    test al, 1
    jne ega700_outside
    sar ax, 1
    mov word ptr [bp - 6], ax
    mov ax, word ptr [_screen_half_height]
    sub ax, si
    js ega700_outside
    test al, 1
    jne ega700_outside
    sar ax, 1
    mov word ptr [bp - 8], ax
    jmp short ega700_plot_quadrants
ega700_outside:
    jmp near ptr ega700_check_axis_swap
ega700_plot_quadrants:
    cmp word ptr [_symmetry_edge], 0
    jne ega700_check_symmetry
    mov cl, byte ptr [bp - 2]
    and cl, 7
    mov ax, 08000h
    shr ax, cl
    add ax, 8
    out dx, ax
    mov ax, word ptr [bp - 2]
    sar ax, 1
    sar ax, 1
    sar ax, 1
    mov di, ax
    mov ax, word ptr [bp - 6]
    mov cl, 4
    shl ax, cl
    add di, ax
    shl ax, 1
    shl ax, 1
    add di, ax
    mov al, byte ptr es:[di]
    mov byte ptr es:[di], bl
ega700_check_symmetry:
    test word ptr [bp + 6], 1
    jne ega700_plot_reflections
    jmp near ptr ega700_return
ega700_plot_reflections:
    cmp byte ptr [_symmetry_edge], 0
    jne ega700_left_bottom
    mov cl, byte ptr [bp - 2]
    and cl, 7
    mov ax, 08000h
    shr ax, cl
    add ax, 8
    out dx, ax
    mov ax, word ptr [bp - 2]
    sar ax, 1
    sar ax, 1
    sar ax, 1
    mov di, ax
    mov ax, word ptr [bp - 8]
    mov cl, 4
    shl ax, cl
    add di, ax
    shl ax, 1
    shl ax, 1
    add di, ax
    mov al, byte ptr es:[di]
    mov byte ptr es:[di], bl
ega700_left_bottom:
    cmp byte ptr [_symmetry_y_edge], 0
    jne ega700_left_top
    mov cl, byte ptr [bp - 4]
    and cl, 7
    mov ax, 08000h
    shr ax, cl
    add ax, 8
    out dx, ax
    mov ax, word ptr [bp - 4]
    sar ax, 1
    sar ax, 1
    sar ax, 1
    mov di, ax
    mov ax, word ptr [bp - 6]
    mov cl, 4
    shl ax, cl
    add di, ax
    shl ax, 1
    shl ax, 1
    add di, ax
    mov al, byte ptr es:[di]
    mov byte ptr es:[di], bl
ega700_left_top:
    mov cl, byte ptr [bp - 4]
    and cl, 7
    mov ax, 08000h
    shr ax, cl
    add ax, 8
    out dx, ax
    mov ax, word ptr [bp - 4]
    sar ax, 1
    sar ax, 1
    sar ax, 1
    mov di, ax
    mov ax, word ptr [bp - 8]
    mov cl, 4
    shl ax, cl
    add di, ax
    shl ax, 1
    shl ax, 1
    add di, ax
    mov al, byte ptr es:[di]
    mov byte ptr es:[di], bl
ega700_check_axis_swap:
    test word ptr [bp + 6], 3
    je ega700_return
    jp ega700_swap_axes
    jmp short ega700_return
ega700_swap_axes:
    xor ax, ax
    mov word ptr [_symmetry_edge], ax
    mov ax, word ptr [_screen_half_width]
    mov si, word ptr [_draw_y]
    cmp ax, si
    jg ega700_swapped_x
    inc byte ptr [_symmetry_edge]
ega700_swapped_x:
    add ax, si
    js ega700_return
    mov word ptr [bp - 2], ax
    mov ax, word ptr [_screen_half_width]
    sub ax, si
    js ega700_return
    mov word ptr [bp - 4], ax
    mov ax, word ptr [_screen_half_height]
    mov si, word ptr [_draw_x]
    cmp ax, si
    jg ega700_swapped_y
    inc byte ptr [_symmetry_y_edge]
ega700_swapped_y:
    add ax, si
    js ega700_return
    test al, 1
    jne ega700_return
    sar ax, 1
    mov word ptr [bp - 6], ax
    mov ax, word ptr [_screen_half_height]
    sub ax, si
    js ega700_return
    test al, 1
    jne ega700_return
    sar ax, 1
    mov word ptr [bp - 8], ax
    mov word ptr [bp + 6], 1
    jmp near ptr ega700_plot_quadrants
ega700_return:
    pop di
    pop si
    mov sp, bp
    pop bp
    retf

; (symmetry): full-height EGA; ES latch reads precede masked color stores.
comment !
/* objcmp: {
 "function": "ega_plot_symmetric_640x350",
 "address": "1899:02e8",
 "size": 419
} */
!
public _ega_plot_symmetric_640x350
_ega_plot_symmetric_640x350 label far
ega_plot_symmetric_640x350 label far
    push bp
    mov bp, sp
    sub sp, 8
    push si
    push di
    mov ax, word ptr [_page_paragraph]
    add ax, VGA_MEMORY
    mov es, ax
    mov cx, word ptr [_draw_delay_count]
ega350_delay:
    and ch, 01fh
    loop ega350_delay
    mov bl, byte ptr [_plot_color]
    mov dx, VGA_GC_INDEX
    xor ax, ax
    mov word ptr [_symmetry_edge], ax
    mov ax, word ptr [_screen_half_width]
    mov si, word ptr [_draw_x]
    cmp ax, si
    jg ega350_sum_x
    inc byte ptr [_symmetry_edge]
ega350_sum_x:
    add ax, si
    js ega350_outside
    mov word ptr [bp - 2], ax
    mov ax, word ptr [_screen_half_width]
    sub ax, si
    js ega350_outside
    mov word ptr [bp - 4], ax
    mov ax, word ptr [_screen_half_height]
    mov si, word ptr [_draw_y]
    cmp ax, si
    jg ega350_sum_y
    inc byte ptr [_symmetry_y_edge]
ega350_sum_y:
    add ax, si
    js ega350_outside
    mov word ptr [bp - 6], ax
    mov ax, word ptr [_screen_half_height]
    sub ax, si
    js ega350_outside
    mov word ptr [bp - 8], ax
    jmp short ega350_plot_quadrants
ega350_outside:
    jmp near ptr ega350_check_axis_swap
ega350_plot_quadrants:
    cmp word ptr [_symmetry_edge], 0
    jne ega350_check_symmetry
    mov cl, byte ptr [bp - 2]
    and cl, 7
    mov ax, 08000h
    shr ax, cl
    add ax, 8
    out dx, ax
    mov ax, word ptr [bp - 2]
    sar ax, 1
    sar ax, 1
    sar ax, 1
    mov di, ax
    mov ax, word ptr [bp - 6]
    mov cl, 4
    shl ax, cl
    add di, ax
    shl ax, 1
    shl ax, 1
    add di, ax
    mov al, byte ptr es:[di]
    mov byte ptr es:[di], bl
ega350_check_symmetry:
    test word ptr [bp + 6], 1
    jne ega350_plot_reflections
    jmp near ptr ega350_return
ega350_plot_reflections:
    cmp byte ptr [_symmetry_edge], 0
    jne ega350_left_bottom
    mov cl, byte ptr [bp - 2]
    and cl, 7
    mov ax, 08000h
    shr ax, cl
    add ax, 8
    out dx, ax
    mov ax, word ptr [bp - 2]
    sar ax, 1
    sar ax, 1
    sar ax, 1
    mov di, ax
    mov ax, word ptr [bp - 8]
    mov cl, 4
    shl ax, cl
    add di, ax
    shl ax, 1
    shl ax, 1
    add di, ax
    mov al, byte ptr es:[di]
    mov byte ptr es:[di], bl
ega350_left_bottom:
    cmp byte ptr [_symmetry_y_edge], 0
    jne dv_03fe
    mov cl, byte ptr [bp - 4]
    and cl, 7
    mov ax, 08000h
    shr ax, cl
    add ax, 8
    out dx, ax
    mov ax, word ptr [bp - 4]
    sar ax, 1
    sar ax, 1
    sar ax, 1
    mov di, ax
    mov ax, word ptr [bp - 6]
    mov cl, 4
    shl ax, cl
    add di, ax
    shl ax, 1
    shl ax, 1
    add di, ax
    mov al, byte ptr es:[di]
    mov byte ptr es:[di], bl
dv_03fe:
    mov cl, byte ptr [bp - 4]
    and cl, 7
    mov ax, 08000h
    shr ax, cl
    add ax, 8
    out dx, ax
    mov ax, word ptr [bp - 4]
    sar ax, 1
    sar ax, 1
    sar ax, 1
    mov di, ax
    mov ax, word ptr [bp - 8]
    mov cl, 4
    shl ax, cl
    add di, ax
    shl ax, 1
    shl ax, 1
    add di, ax
    mov al, byte ptr es:[di]
    mov byte ptr es:[di], bl
ega350_check_axis_swap:
    test word ptr [bp + 6], 3
    je ega350_return
    jp dv_0438
    jmp short ega350_return
dv_0438:
    xor ax, ax
    mov word ptr [_symmetry_edge], ax
    mov ax, word ptr [_screen_half_width]
    mov si, word ptr [_draw_y]
    cmp ax, si
    jg dv_044c
    inc byte ptr [_symmetry_edge]
dv_044c:
    add ax, si
    js ega350_return
    mov word ptr [bp - 2], ax
    mov ax, word ptr [_screen_half_width]
    sub ax, si
    js ega350_return
    mov word ptr [bp - 4], ax
    mov ax, word ptr [_screen_half_height]
    mov si, word ptr [_draw_x]
    cmp ax, si
    jg dv_046c
    inc byte ptr [_symmetry_y_edge]
dv_046c:
    add ax, si
    js ega350_return
    mov word ptr [bp - 6], ax
    mov ax, word ptr [_screen_half_height]
    sub ax, si
    js ega350_return
    mov word ptr [bp - 8], ax
    mov word ptr [bp + 6], 1
    jmp near ptr ega350_plot_quadrants
ega350_return:
    pop di
    pop si
    mov sp, bp
    pop bp
    retf

; (symmetry): unchained VGA; X low bits select the map-mask plane.
comment !
/* objcmp: {
 "function": "vga_plot_symmetric_320x400",
 "address": "1899:048b",
 "size": 407
} */
!
public _vga_plot_symmetric_320x400
_vga_plot_symmetric_320x400 label far
vga_plot_symmetric_320x400 label far
    push bp
    mov bp, sp
    sub sp, 8
    push si
    push di
    mov ax, word ptr [_page_paragraph]
    add ax, VGA_MEMORY
    mov es, ax
    mov cx, word ptr [_draw_delay_count]
vga400_delay:
    and ch, 01fh
    loop vga400_delay
    mov bl, byte ptr [_plot_color]
    mov dx, VGA_SEQ_INDEX
    xor ax, ax
    mov word ptr [_symmetry_edge], ax
    mov ax, word ptr [_screen_half_width]
    mov si, word ptr [_draw_x]
    cmp ax, si
    jg dv_04bf
    inc byte ptr [_symmetry_edge]
dv_04bf:
    add ax, si
    js dv_04f2
    mov word ptr [bp - 2], ax
    mov ax, word ptr [_screen_half_width]
    sub ax, si
    js dv_04f2
    mov word ptr [bp - 4], ax
    mov ax, word ptr [_screen_half_height]
    mov si, word ptr [_draw_y]
    cmp ax, si
    jg dv_04df
    inc byte ptr [_symmetry_y_edge]
dv_04df:
    add ax, si
    js dv_04f2
    mov word ptr [bp - 6], ax
    mov ax, word ptr [_screen_half_height]
    sub ax, si
    js dv_04f2
    mov word ptr [bp - 8], ax
    jmp short dv_04f5
dv_04f2:
    jmp near ptr dv_05c4
dv_04f5:
    cmp word ptr [_symmetry_edge], 0
    jne dv_0528
    mov cl, byte ptr [bp - 2]
    and cl, 3
    mov ax, 0100h
    shl ax, cl
    inc ax
    inc ax
    out dx, ax
    mov ax, word ptr [bp - 2]
    sar ax, 1
    sar ax, 1
    mov di, ax
    mov ax, word ptr [bp - 6]
    mov cl, 4
    shl ax, cl
    add di, ax
    shl ax, 1
    shl ax, 1
    add di, ax
    mov al, byte ptr es:[di]
    mov byte ptr es:[di], bl
dv_0528:
    test word ptr [bp + 6], 1
    jne dv_0532
    jmp near ptr dv_061c
dv_0532:
    cmp byte ptr [_symmetry_edge], 0
    jne dv_0565
    mov cl, byte ptr [bp - 2]
    and cl, 3
    mov ax, 0100h
    shl ax, cl
    inc ax
    inc ax
    out dx, ax
    mov ax, word ptr [bp - 2]
    sar ax, 1
    sar ax, 1
    mov di, ax
    mov ax, word ptr [bp - 8]
    mov cl, 4
    shl ax, cl
    add di, ax
    shl ax, 1
    shl ax, 1
    add di, ax
    mov al, byte ptr es:[di]
    mov byte ptr es:[di], bl
dv_0565:
    cmp byte ptr [_symmetry_y_edge], 0
    jne dv_0598
    mov cl, byte ptr [bp - 4]
    and cl, 3
    mov ax, 0100h
    shl ax, cl
    inc ax
    inc ax
    out dx, ax
    mov ax, word ptr [bp - 4]
    sar ax, 1
    sar ax, 1
    mov di, ax
    mov ax, word ptr [bp - 6]
    mov cl, 4
    shl ax, cl
    add di, ax
    shl ax, 1
    shl ax, 1
    add di, ax
    mov al, byte ptr es:[di]
    mov byte ptr es:[di], bl
dv_0598:
    mov cl, byte ptr [bp - 4]
    and cl, 3
    mov ax, 0100h
    shl ax, cl
    inc ax
    inc ax
    out dx, ax
    mov ax, word ptr [bp - 4]
    sar ax, 1
    sar ax, 1
    mov di, ax
    mov ax, word ptr [bp - 8]
    mov cl, 4
    shl ax, cl
    add di, ax
    shl ax, 1
    shl ax, 1
    add di, ax
    mov al, byte ptr es:[di]
    mov byte ptr es:[di], bl
dv_05c4:
    test word ptr [bp + 6], 3
    je dv_061c
    jp dv_05cf
    jmp short dv_061c
dv_05cf:
    xor ax, ax
    mov word ptr [_symmetry_edge], ax
    mov ax, word ptr [_screen_half_width]
    mov si, word ptr [_draw_y]
    cmp ax, si
    jg dv_05e3
    inc byte ptr [_symmetry_edge]
dv_05e3:
    add ax, si
    js dv_061c
    mov word ptr [bp - 2], ax
    mov ax, word ptr [_screen_half_width]
    sub ax, si
    js dv_061c
    mov word ptr [bp - 4], ax
    mov ax, word ptr [_screen_half_height]
    mov si, word ptr [_draw_x]
    cmp ax, si
    jg dv_0603
    inc byte ptr [_symmetry_y_edge]
dv_0603:
    add ax, si
    js dv_061c
    mov word ptr [bp - 6], ax
    mov ax, word ptr [_screen_half_height]
    sub ax, si
    js dv_061c
    mov word ptr [bp - 8], ax
    mov word ptr [bp + 6], 1
    jmp near ptr dv_04f5
dv_061c:
    pop di
    pop si
    mov sp, bp
    pop bp
    retf

; (symmetry): chained VGA; byte address is X + 320*Y.
comment !
/* objcmp: {
 "function": "vga_plot_symmetric_320x200",
 "address": "1899:0622",
 "size": 315
} */
!
public _vga_plot_symmetric_320x200
_vga_plot_symmetric_320x200 label far
vga_plot_symmetric_320x200 label far
    push bp
    mov bp, sp
    sub sp, 8
    push si
    push di
    mov ax, word ptr [_page_paragraph]
    add ax, VGA_MEMORY
    mov es, ax
    mov cx, word ptr [_draw_delay_count]
vga200_delay:
    and ch, 01fh
    loop vga200_delay
    mov bl, byte ptr [_plot_color]
    xor ax, ax
    mov word ptr [_symmetry_edge], ax
    mov ax, word ptr [_screen_half_width]
    mov si, word ptr [_draw_x]
    cmp ax, si
    jg vga200_sum_x
    inc byte ptr [_symmetry_edge]
vga200_sum_x:
    add ax, si
    js vga200_outside
    mov word ptr [bp - 2], ax
    mov ax, word ptr [_screen_half_width]
    sub ax, si
    js vga200_outside
    mov word ptr [bp - 4], ax
    mov ax, word ptr [_screen_half_height]
    mov si, word ptr [_draw_y]
    cmp ax, si
    jg vga200_sum_y
    inc byte ptr [_symmetry_y_edge]
vga200_sum_y:
    add ax, si
    js vga200_outside
    mov word ptr [bp - 6], ax
    mov ax, word ptr [_screen_half_height]
    sub ax, si
    js vga200_outside
    mov word ptr [bp - 8], ax
    jmp short vga200_plot_quadrants
vga200_outside:
    jmp short vga200_check_axis_swap
vga200_plot_quadrants:
    cmp word ptr [_symmetry_edge], 0
    jne vga200_check_symmetry
    mov ax, word ptr [bp - 6]
    xchg ah, al
    mov di, word ptr [bp - 2]
    add di, ax
    shr ax, 1
    shr ax, 1
    add di, ax
    mov al, byte ptr es:[di]
    mov byte ptr es:[di], bl
vga200_check_symmetry:
    test word ptr [bp + 6], 1
    jne vga200_plot_reflections
    jmp near ptr vga200_return
vga200_plot_reflections:
    cmp byte ptr [_symmetry_edge], 0
    jne vga200_left_bottom
    mov ax, word ptr [bp - 8]
    xchg ah, al
    mov di, word ptr [bp - 2]
    add di, ax
    shr ax, 1
    shr ax, 1
    add di, ax
    mov al, byte ptr es:[di]
    mov byte ptr es:[di], bl
vga200_left_bottom:
    cmp byte ptr [_symmetry_y_edge], 0
    jne vga200_left_top
    mov ax, word ptr [bp - 6]
    xchg ah, al
    mov di, word ptr [bp - 4]
    add di, ax
    shr ax, 1
    shr ax, 1
    add di, ax
    mov al, byte ptr es:[di]
    mov byte ptr es:[di], bl
vga200_left_top:
    mov ax, word ptr [bp - 8]
    xchg ah, al
    mov di, word ptr [bp - 4]
    add di, ax
    shr ax, 1
    shr ax, 1
    add di, ax
    mov al, byte ptr es:[di]
    mov byte ptr es:[di], bl
vga200_check_axis_swap:
    test word ptr [bp + 6], 3
    je vga200_return
    jp vga200_swap_axes
    jmp short vga200_return
vga200_swap_axes:
    xor ax, ax
    mov word ptr [_symmetry_edge], ax
    mov ax, word ptr [_screen_half_width]
    mov si, word ptr [_draw_y]
    cmp ax, si
    jg vga200_swapped_x
    inc byte ptr [_symmetry_edge]
vga200_swapped_x:
    add ax, si
    js vga200_return
    mov word ptr [bp - 2], ax
    mov ax, word ptr [_screen_half_width]
    sub ax, si
    js vga200_return
    mov word ptr [bp - 4], ax
    mov ax, word ptr [_screen_half_height]
    mov si, word ptr [_draw_x]
    cmp ax, si
    jg vga200_swapped_y
    inc byte ptr [_symmetry_y_edge]
vga200_swapped_y:
    add ax, si
    js vga200_return
    mov word ptr [bp - 6], ax
    mov ax, word ptr [_screen_half_height]
    sub ax, si
    js vga200_return
    mov word ptr [bp - 8], ax
    mov word ptr [bp + 6], 1
    jmp near ptr vga200_plot_quadrants
vga200_return:
    pop di
    pop si
    mov sp, bp
    pop bp
    retf

; (x,y) -> AX color; virtual Y must be even in the 640x700 interface.
comment !
/* objcmp: {
 "function": "ega_read_pixel_640x700",
 "address": "1899:075d",
 "size": 152
} */
!
public _ega_read_pixel_640x700
_ega_read_pixel_640x700 label far
ega_read_pixel_640x700 label far
    push bp
    mov bp, sp
    sub sp, 4
    push si
    push di
    mov ax, word ptr [_page_paragraph]
    add ax, VGA_MEMORY
    mov es, ax
    mov ax, word ptr [_screen_half_width]
    mov si, word ptr [bp + 6]
    add ax, si
    js dv_07ed
    mov word ptr [bp - 2], ax
    mov ax, word ptr [_screen_half_height]
    mov si, word ptr [bp + 8]
    add ax, si
    js dv_07ed
    sar ax, 1
    mov word ptr [bp - 4], ax
    mov ax, word ptr [bp - 2]
    sar ax, 1
    sar ax, 1
    sar ax, 1
    mov di, ax
    mov ax, word ptr [bp - 4]
    mov cl, 4
    shl ax, cl
    add di, ax
    shl ax, 1
    shl ax, 1
    add di, ax
    mov cx, word ptr [bp - 2]
    and cl, 7
    xor cl, 7
    mov bl, 1
    shl bl, cl
    mov dx, VGA_GC_INDEX
    mov ax, 4
    out dx, ax
    mov al, byte ptr es:[di]
    and al, bl
    ror al, cl
    mov bh, al
    dec cl
    inc dx
    mov al, 1
    out dx, al
    mov al, byte ptr es:[di]
    and al, bl
    ror al, cl
    or bh, al
    dec cl
    mov al, 2
    out dx, al
    mov al, byte ptr es:[di]
    and al, bl
    ror al, cl
    or bh, al
    dec cl
    mov al, 3
    out dx, al
    mov al, byte ptr es:[di]
    and al, bl
    ror al, cl
    or bh, al
    mov al, bh
dv_07ed:
    xor ah, ah
    pop di
    pop si
    mov sp, bp
    pop bp
    retf

; (x,y) -> AX; reconstruct EGA color from four read-map planes.
comment !
/* objcmp: {
 "function": "ega_read_pixel_640x350",
 "address": "1899:07f5",
 "size": 150
} */
!
public _ega_read_pixel_640x350
_ega_read_pixel_640x350 label far
ega_read_pixel_640x350 label far
    push bp
    mov bp, sp
    sub sp, 4
    push si
    push di
    mov ax, word ptr [_page_paragraph]
    add ax, VGA_MEMORY
    mov es, ax
    mov ax, word ptr [_screen_half_width]
    mov si, word ptr [bp + 6]
    add ax, si
    js dv_0883
    mov word ptr [bp - 2], ax
    mov ax, word ptr [_screen_half_height]
    mov si, word ptr [bp + 8]
    add ax, si
    js dv_0883
    mov word ptr [bp - 4], ax
    mov ax, word ptr [bp - 2]
    sar ax, 1
    sar ax, 1
    sar ax, 1
    mov di, ax
    mov ax, word ptr [bp - 4]
    mov cl, 4
    shl ax, cl
    add di, ax
    shl ax, 1
    shl ax, 1
    add di, ax
    mov cx, word ptr [bp - 2]
    and cl, 7
    xor cl, 7
    mov bl, 1
    shl bl, cl
    mov dx, VGA_GC_INDEX
    mov ax, 4
    out dx, ax
    mov al, byte ptr es:[di]
    and al, bl
    ror al, cl
    mov bh, al
    dec cl
    inc dx
    mov al, 1
    out dx, al
    mov al, byte ptr es:[di]
    and al, bl
    ror al, cl
    or bh, al
    dec cl
    mov al, 2
    out dx, al
    mov al, byte ptr es:[di]
    and al, bl
    ror al, cl
    or bh, al
    dec cl
    mov al, 3
    out dx, al
    mov al, byte ptr es:[di]
    and al, bl
    ror al, cl
    or bh, al
    mov al, bh
dv_0883:
    xor ah, ah
    pop di
    pop si
    mov sp, bp
    pop bp
    retf

; (x,y) -> AX; select unchained read-map plane from X&3.
comment !
/* objcmp: {
 "function": "vga_read_pixel_320x400",
 "address": "1899:088b",
 "size": 102
} */
!
public _vga_read_pixel_320x400
_vga_read_pixel_320x400 label far
vga_read_pixel_320x400 label far
    push bp
    mov bp, sp
    sub sp, 4
    push si
    push di
    mov ax, word ptr [_page_paragraph]
    add ax, VGA_MEMORY
    mov es, ax
    mov ax, word ptr [_screen_half_width]
    mov si, word ptr [bp + 6]
    add ax, si
    mov word ptr [bp - 2], ax
    mov ax, word ptr [_screen_half_height]
    mov si, word ptr [bp + 8]
    add ax, si
    mov word ptr [bp - 4], ax
    mov cl, byte ptr [bp - 2]
    and cl, 3
    mov ax, 0100h
    shl ax, cl
    inc ax
    inc ax
    mov dx, VGA_SEQ_INDEX
    out dx, ax
    mov dx, VGA_GC_INDEX
    mov ah, byte ptr [bp - 2]
    and ah, 3
    mov al, 4
    out dx, ax
    mov ax, word ptr [bp - 2]
    sar ax, 1
    sar ax, 1
    mov di, ax
    mov ax, word ptr [bp - 4]
    mov cl, 4
    shl ax, cl
    add di, ax
    shl ax, 1
    shl ax, 1
    add di, ax
    mov al, byte ptr es:[di]
    xor ah, ah
    pop di
    pop si
    mov sp, bp
    pop bp
    retf

; (x,y) -> AX; chained VGA pixel read.
comment !
/* objcmp: {
 "function": "vga_read_pixel_320x200",
 "address": "1899:08f1",
 "size": 65
} */
!
public _vga_read_pixel_320x200
_vga_read_pixel_320x200 label far
vga_read_pixel_320x200 label far
    push bp
    mov bp, sp
    sub sp, 4
    push si
    push di
    mov ax, word ptr [_page_paragraph]
    add ax, VGA_MEMORY
    mov es, ax
    mov ax, word ptr [_screen_half_width]
    mov si, word ptr [bp + 6]
    add ax, si
    mov word ptr [bp - 2], ax
    mov ax, word ptr [_screen_half_height]
    mov si, word ptr [bp + 8]
    add ax, si
    mov word ptr [bp - 4], ax
    mov ax, word ptr [bp - 4]
    xchg ah, al
    mov di, word ptr [bp - 2]
    add di, ax
    shr ax, 1
    shr ax, 1
    add di, ax
    mov al, byte ptr es:[di]
    xor ah, ah
    pop di
    pop si
    mov sp, bp
    pop bp
    retf

; Save BIOS mode, cursor shape, and BIOS text-row count.
comment !
/* objcmp: {
 "function": "save_video_state",
 "address": "1899:0932",
 "size": 39
} */
!
public _save_video_state
_save_video_state label far
save_video_state label far
    push di
    mov ax, 0f00h
    int 010h
    and ax, 07fh
    mov word ptr [_saved_bios_mode], ax
    mov ax, 0300h
    int 010h
    mov word ptr [_saved_cursor_shape], cx
    mov ax, 040h
    mov es, ax
    mov di, 084h
    mov al, byte ptr es:[di]
    xor ah, ah
    mov word ptr [_saved_text_last_row], ax
    pop di
    retf

; BIOS display-combination switch, EGA fallback, and forced adapter policy.
comment !
/* objcmp: {
 "function": "detect_video_adapter",
 "address": "1899:0959",
 "size": 332
} */
!
public _detect_video_adapter
_detect_video_adapter label far
detect_video_adapter label far
    push si
    mov word ptr [_video_adapter], 0
    mov ax, 01a00h
    xor bx, bx
    int 010h
    cmp al, 01ah
    je dv_096d
    jmp short dv_09e8
dv_096d:
    mov ax, bx
    mov si, ax
    xor ah, ah
    dec ax
    cmp ax, 0bh
    ja dv_09e2
    mov bx, ax
    shl bx, 1
    jmp word ptr cs:[bx + offset display_combination_table]
display_combination_table:
    dw offset dv_099a, offset dv_09a2, offset dv_09e2, offset dv_09aa, offset dv_09b2, offset dv_09ba, offset dv_09c2, offset dv_09ca, offset dv_09e2, offset dv_09e2, offset dv_09d2, offset dv_09da
dv_099a:
    mov word ptr [_video_adapter], 1
    jmp short dv_09e8
dv_09a2:
    mov word ptr [_video_adapter], 2
    jmp short dv_09e8
dv_09aa:
    mov word ptr [_video_adapter], 4
    jmp short dv_09e8
dv_09b2:
    mov word ptr [_video_adapter], 5
    jmp short dv_09e8
dv_09ba:
    mov word ptr [_video_adapter], 6
    jmp short dv_09e8
dv_09c2:
    mov word ptr [_video_adapter], 7
    jmp short dv_09e8
dv_09ca:
    mov word ptr [_video_adapter], 8
    jmp short dv_09e8
dv_09d2:
    mov word ptr [_video_adapter], 0bh
    jmp short dv_09e8
dv_09da:
    mov word ptr [_video_adapter], 0ch
    jmp short dv_09e8
dv_09e2:
    mov word ptr [_video_adapter], 0
dv_09e8:
    cmp word ptr [_video_adapter], 0
    jne dv_0a27
    mov ax, 01200h
    mov bx, 010h
    int 010h
    mov cx, bx
    cmp bl, 010h
    je dv_0a27
    and bx, 3
    mov word ptr [_ega_memory_code], bx
    mov bx, 040h
    mov es, bx
    mov bx, 087h
    test byte ptr es:[bx], 8
    jne dv_0a27
    test cx, 0100h
    je dv_0a21
    mov word ptr [_video_adapter], 5
    jmp short dv_0a27
dv_0a21:
    mov word ptr [_video_adapter], 4
dv_0a27:
    mov ax, word ptr [_video_adapter]
    sub ax, 4
    cmp ax, 4
    ja dv_0a6b
    mov bx, ax
    shl bx, 1
    jmp word ptr cs:[bx + offset adapter_action_table]
adapter_action_table:
    dw offset dv_0a89, offset dv_0a45, offset dv_0a6b, offset dv_0a63, offset dv_0a89
dv_0a45:
    mov dx, 03bcah
    mov ah, 9
    int 021h
    cmp word ptr [_force_ega], 0
    jne dv_0a89
    cmp word ptr [_force_vga], 0
    jne dv_0a96
    mov ax, 028h
    push ax
    call far ptr _print_closing_text_and_exit
dv_0a63:
    mov word ptr [_video_adapter], 8
    jmp short dv_0a89
dv_0a6b:
    mov dx, 03c04h
    mov ah, 9
    int 021h
    cmp word ptr [_force_ega], 0
    jne dv_0a89
    cmp word ptr [_force_vga], 0
    jne dv_0a96
    mov ax, 02ah
    push ax
    call far ptr _print_closing_text_and_exit
dv_0a89:
    cmp word ptr [_force_ega], 0
    je dv_0a96
    mov word ptr [_video_adapter], 4
dv_0a96:
    cmp word ptr [_force_vga], 0
    je dv_0aa3
    mov word ptr [_video_adapter], 8
dv_0aa3:
    pop si
    retf

; (clear): clear=0 uses no-clear EGA mode, clear=2 skips BIOS mode selection.
comment !
/* objcmp: {
 "function": "ega_set_mode",
 "address": "1899:0aa5",
 "size": 40
} */
!
public _ega_set_mode
_ega_set_mode label far
ega_set_mode label far
    push bp
    mov bp, sp
    mov word ptr [_video_mode_cookie], 041h
    cmp word ptr [bp + 6], 2
    je dv_0ac4
    cmp word ptr [bp + 6], 0
    je dv_0abf
    mov ax, 010h
    jmp short dv_0ac2
dv_0abf:
    mov ax, 090h
dv_0ac2:
    int 010h
dv_0ac4:
    mov ax, 0205h
    mov dx, VGA_GC_INDEX
    out dx, ax
    pop bp
    retf

; (color,word count,offset): enable all planes and REP STOSW VRAM.
comment !
/* objcmp: {
 "function": "clear_vram_words",
 "address": "1899:0acd",
 "size": 47
} */
!
public _clear_vram_words
_clear_vram_words label far
clear_vram_words label far
    push bp
    mov bp, sp
    push di
    sti
    mov dx, VGA_STATUS
dv_0ad5:
    in al, dx
    test al, 8
    jne dv_0ad5
dv_0ada:
    in al, dx
    test al, 8
    je dv_0ada
    mov dx, VGA_SEQ_INDEX
    mov ax, 0f02h
    out dx, ax
    mov ax, VGA_MEMORY
    add ax, word ptr [bp + 0ah]
    mov es, ax
    sub di, di
    cld
    mov ax, word ptr [bp + 6]
    mov cx, word ptr [bp + 8]
    rep stosw
    pop di
    pop bp
    retf

; (clear): set 13h/93h, unchain, select byte addressing and 400 scan lines.
comment !
/* objcmp: {
 "function": "vga_set_mode_320x400",
 "address": "1899:0afc",
 "size": 162
} */
!
public _vga_set_mode_320x400
_vga_set_mode_320x400 label far
vga_set_mode_320x400 label far
    push bp
    mov bp, sp
    mov word ptr [_video_mode_cookie], 043h
    cmp word ptr [bp + 6], 0
    je dv_0b10
    mov ax, 013h
    jmp short dv_0b13
dv_0b10:
    mov ax, 093h
dv_0b13:
    int 010h
    mov dx, VGA_MEMORY
    mov es, dx
    xor dx, dx
    xor bx, bx
    mov cx, 0100h
    mov ax, 01012h
    int 010h
    mov al, 4
    mov dx, VGA_SEQ_INDEX
    cli
    out dx, al
    inc dx
    in al, dx
    and al, 0f7h
    or al, 4
    out dx, al
    sti
    mov al, 5
    mov dx, VGA_GC_INDEX
    cli
    out dx, al
    inc dx
    in al, dx
    and al, 0e7h
    out dx, al
    sti
    mov al, 6
    mov dx, VGA_GC_INDEX
    cli
    out dx, al
    inc dx
    in al, dx
    and al, 0fdh
    out dx, al
    sti
    cmp word ptr [bp + 6], 0
    je dv_0b67
    xor ax, ax
    push ax
    mov ax, 0fa00h
    push ax
    mov ax, word ptr [_background_fill_word]
    push ax
    push cs
    call near ptr clear_vram_words
    add sp, 6
dv_0b67:
    mov al, 9
    mov dx, VGA_CRTC_INDEX
    cli
    out dx, al
    inc dx
    in al, dx
    and al, 060h
    out dx, al
    sti
    mov al, 014h
    mov dx, VGA_CRTC_INDEX
    cli
    out dx, al
    inc dx
    in al, dx
    and al, 0bfh
    out dx, al
    sti
    mov al, 017h
    mov dx, VGA_CRTC_INDEX
    cli
    out dx, al
    inc dx
    in al, dx
    or al, 040h
    out dx, al
    sti
    les dx, dword ptr [_palette_buffer]
    xor bx, bx
    mov cx, 9
    mov ax, 01012h
    int 010h
    pop bp
    retf

; (clear): ordinary mode 13h; optional explicit background fill.
comment !
/* objcmp: {
 "function": "vga_set_mode_320x200",
 "address": "1899:0b9e",
 "size": 47
} */
!
public _vga_set_mode_320x200
_vga_set_mode_320x200 label far
vga_set_mode_320x200 label far
    push bp
    mov bp, sp
    mov word ptr [_video_mode_cookie], 044h
    cmp word ptr [bp + 6], 0
    je dv_0bc6
    mov ax, 013h
    int 010h
    xor ax, ax
    push ax
    mov ax, 07d00h
    push ax
    mov ax, word ptr [_background_fill_word]
    push ax
    push cs
    call near ptr clear_vram_words
    add sp, 6
    jmp short dv_0bcb
dv_0bc6:
    mov ax, 093h
    int 010h
dv_0bcb:
    pop bp
    retf

; (restore): reset GC, optionally restore BIOS text mode/rows/cursor.
comment !
/* objcmp: {
 "function": "restore_video_state",
 "address": "1899:0bcd",
 "size": 70
} */
!
public _restore_video_state
_restore_video_state label far
restore_video_state label far
    push bp
    mov bp, sp
    push di
    cli
    mov ax, 0ff08h
    mov dx, VGA_GC_INDEX
    out dx, ax
    mov ax, 5
    out dx, ax
    sti
    cmp word ptr [bp + 6], 0
    je dv_0c10
    mov ax, word ptr [_saved_bios_mode]
    xor ah, ah
    int 010h
    cmp word ptr [_saved_text_last_row], 018h
    jbe dv_0c07
    mov ax, 040h
    mov es, ax
    mov di, 084h
    mov ax, word ptr [_saved_text_last_row]
    mov byte ptr es:[di], al
    mov ax, 01112h
    xor bx, bx
    int 010h
dv_0c07:
    mov cx, word ptr [_saved_cursor_shape]
    mov ax, 0100h
    int 010h
dv_0c10:
    pop di
    pop bp
    retf

;
comment !
/* objcmp: {
 "function": "ega_upload_palette_bios",
 "address": "1899:0c13",
 "size": 105
} */
!
public _ega_upload_palette_bios
_ega_upload_palette_bios label far
ega_upload_palette_bios label far
    ; (far palette): save four byte values in word slots. Keep AH zero
    ; across the substitutions; the common SI save is unused in this variant.
    ; BIOS service 1002h takes ES:DX. The saved entries are restored afterward.
    push bp
    mov bp, sp
    sub sp, 8
    push si
    les bx, dword ptr [bp + PAL_ARGUMENT]
    mov al, byte ptr es:[bx]
    mov ah, 0
    mov word ptr [bp + PAL_OLD0], ax
    mov al, byte ptr [_background_fill_word]
    mov byte ptr es:[bx], al
    mov al, byte ptr es:[bx + 1]
    mov word ptr [bp + PAL_OLD1], ax
    mov al, byte ptr [_palette_base_green]
    mov byte ptr es:[bx + 1], al
    mov al, byte ptr es:[bx + 2]
    mov word ptr [bp + PAL_OLD2], ax
    mov al, byte ptr [_palette_base_blue]
    mov byte ptr es:[bx + 2], al
    mov al, byte ptr es:[bx + 010h]
    mov word ptr [bp + PAL_OLD_BORDER], ax
    mov al, byte ptr [_g_5d8c]
    mov byte ptr es:[bx + 010h], al
    mov dx, bx
    mov ax, 01002h
    int 010h
    mov al, byte ptr [bp + PAL_OLD_BORDER]
    mov byte ptr es:[bx + 010h], al
    mov al, byte ptr [bp + PAL_OLD2]
    mov byte ptr es:[bx + 2], al
    mov al, byte ptr [bp + PAL_OLD1]
    mov byte ptr es:[bx + 1], al
    mov al, byte ptr [bp + PAL_OLD0]
    mov byte ptr es:[bx], al
    pop si
    mov sp, bp
    pop bp
    retf

;
comment !
/* objcmp: {
 "function": "ega_upload_palette_ports",
 "address": "1899:0c7c",
 "size": 152
} */
!
public _ega_upload_palette_ports
_ega_upload_palette_ports label far
ega_upload_palette_ports label far
    ; (far palette): ES:SI remains live across the attribute writes.
    ; Wait for the next retrace edge; each index/data pair excludes interrupts.
    ; Write palette registers 15..0, then border register 11h with display on.
    push bp
    mov bp, sp
    sub sp, 8
    push si
    les si, dword ptr [bp + PAL_ARGUMENT]
    mov al, byte ptr es:[si]
    mov ah, 0
    mov word ptr [bp + PAL_OLD0], ax
    mov al, byte ptr [_background_fill_word]
    mov byte ptr es:[si], al
    mov al, byte ptr es:[si + 1]
    mov word ptr [bp + PAL_OLD1], ax
    mov al, byte ptr [_palette_base_green]
    mov byte ptr es:[si + 1], al
    mov al, byte ptr es:[si + 2]
    mov word ptr [bp + PAL_OLD2], ax
    mov al, byte ptr [_palette_base_blue]
    mov byte ptr es:[si + 2], al
    mov al, byte ptr es:[si + 010h]
    mov word ptr [bp + PAL_OLD_BORDER], ax
    mov al, byte ptr [_g_5d8c]
    mov byte ptr es:[si + 010h], al
    sti
    mov dx, VGA_STATUS
ega_palette_wait_display:
    in al, dx
    test al, 8
    jne ega_palette_wait_display
ega_palette_wait_retrace:
    in al, dx
    test al, 8
    je ega_palette_wait_retrace
    mov dx, VGA_ATTR
    mov cx, 0fh
ega_palette_next_register:
    mov al, cl
    cli
    out dx, al
    mov bx, cx
    mov al, byte ptr es:[bx + si]
    out dx, al
    sti
    loop ega_palette_next_register
    mov al, cl
    cli
    out dx, al
    mov bx, cx
    mov al, byte ptr es:[bx + si]
    out dx, al
    sti
    mov al, 031h
    cli
    out dx, al
    mov al, byte ptr es:[si + 010h]
    out dx, al
    sti
    mov al, byte ptr [bp + PAL_OLD_BORDER]
    mov byte ptr es:[si + 010h], al
    mov al, byte ptr [bp + PAL_OLD2]
    mov byte ptr es:[si + 2], al
    mov al, byte ptr [bp + PAL_OLD1]
    mov byte ptr es:[si + 1], al
    mov al, byte ptr [bp + PAL_OLD0]
    mov byte ptr es:[si], al
    pop si
    mov sp, bp
    pop bp
    retf

; (far RGB): BIOS DAC upload of entries 3..255.
comment !
/* objcmp: {
 "function": "vga_upload_palette_bios",
 "address": "1899:0d14",
 "size": 19
} */
!
public _vga_upload_palette_bios
_vga_upload_palette_bios label far
vga_upload_palette_bios label far
    push bp
    mov bp, sp
    les dx, dword ptr [bp + 6]
    mov bx, 3
    mov cx, 0fdh
    mov ax, 01012h
    int 010h
    pop bp
    retf

; (far RGB): split BIOS DAC upload; preserve the observed +126-byte step.
comment !
/* objcmp: {
 "function": "vga_upload_palette_bios_split",
 "address": "1899:0d27",
 "size": 33
} */
!
public _vga_upload_palette_bios_split
_vga_upload_palette_bios_split label far
vga_upload_palette_bios_split label far
    push bp
    mov bp, sp
    les dx, dword ptr [bp + 6]
    mov bx, 3
    mov cx, 07eh
    mov ax, 01012h
    int 010h
    add dx, 07eh
    mov bx, 081h
    mov cx, 07fh
    mov ax, 01012h
    int 010h
    pop bp
    retf

; (far RGB): 186+ REP OUTSB after the next vertical retrace edge.
comment !
/* objcmp: {
 "function": "vga_upload_palette_rep",
 "address": "1899:0d48",
 "size": 44
} */
!
public _vga_upload_palette_rep
_vga_upload_palette_rep label far
vga_upload_palette_rep label far
    enter 0, 0
    push si
    push ds
    lds si, dword ptr [bp + 6]
    sti
    mov dx, VGA_STATUS
dv_0d55:
    in al, dx
    test al, 8
    jne dv_0d55
dv_0d5a:
    in al, dx
    test al, 8
    je dv_0d5a
    mov ax, 3
    mov dx, VGA_DAC_WRITE
    out dx, al
    mov dx, VGA_DAC_DATA
    mov cx, 02f7h
    cld
    rep outsb
    sti
    pop ds
    pop si
    leave
    retf

; (far RGB): split REP OUTSB, one retrace per half.
comment !
/* objcmp: {
 "function": "vga_upload_palette_rep_split",
 "address": "1899:0d74",
 "size": 75
} */
!
public _vga_upload_palette_rep_split
_vga_upload_palette_rep_split label far
vga_upload_palette_rep_split label far
    enter 0, 0
    push si
    push ds
    lds si, dword ptr [bp + 6]
    sti
    mov dx, VGA_STATUS
dv_0d81:
    in al, dx
    test al, 8
    jne dv_0d81
dv_0d86:
    in al, dx
    test al, 8
    je dv_0d86
    mov ax, 3
    mov dx, VGA_DAC_WRITE
    out dx, al
    mov dx, VGA_DAC_DATA
    mov cx, 017ah
    cld
    rep outsb
    sti
    sti
    mov dx, VGA_STATUS
dv_0da0:
    in al, dx
    test al, 8
    jne dv_0da0
dv_0da5:
    in al, dx
    test al, 8
    je dv_0da5
    mov ax, 081h
    mov dx, VGA_DAC_WRITE
    out dx, al
    mov dx, VGA_DAC_DATA
    mov cx, 017dh
    cld
    rep outsb
    sti
    pop ds
    pop si
    leave
    retf

; (far RGB): 8086 fallback uses MOV/INC/OUT/LOOP.
comment !
/* objcmp: {
 "function": "vga_upload_palette_8086",
 "address": "1899:0dbf",
 "size": 47
} */
!
public _vga_upload_palette_8086
_vga_upload_palette_8086 label far
vga_upload_palette_8086 label far
    push bp
    mov bp, sp
    push si
    push ds
    lds si, dword ptr [bp + 6]
    sti
    mov dx, VGA_STATUS
dv_0dcb:
    in al, dx
    test al, 8
    jne dv_0dcb
dv_0dd0:
    in al, dx
    test al, 8
    je dv_0dd0
    mov ax, 3
    mov dx, VGA_DAC_WRITE
    out dx, al
    mov dx, VGA_DAC_DATA
    mov cx, 02f7h
    cld
dv_0de3:
    mov al, byte ptr [si]
    inc si
    out dx, al
    loop dv_0de3
    sti
    pop ds
    pop si
    pop bp
    retf

; (far RGB): split 8086 fallback.
comment !
/* objcmp: {
 "function": "vga_upload_palette_8086_split",
 "address": "1899:0dee",
 "size": 82
} */
!
public _vga_upload_palette_8086_split
_vga_upload_palette_8086_split label far
vga_upload_palette_8086_split label far
    push bp
    mov bp, sp
    push si
    push ds
    lds si, dword ptr [bp + 6]
    sti
    mov dx, VGA_STATUS
dv_0dfa:
    in al, dx
    test al, 8
    jne dv_0dfa
dv_0dff:
    in al, dx
    test al, 8
    je dv_0dff
    mov ax, 3
    mov dx, VGA_DAC_WRITE
    out dx, al
    mov dx, VGA_DAC_DATA
    mov cx, 017ah
    cld
dv_0e12:
    mov al, byte ptr [si]
    inc si
    out dx, al
    loop dv_0e12
    sti
    sti
    mov dx, VGA_STATUS
dv_0e1d:
    in al, dx
    test al, 8
    jne dv_0e1d
dv_0e22:
    in al, dx
    test al, 8
    je dv_0e22
    mov ax, 081h
    mov dx, VGA_DAC_WRITE
    out dx, al
    mov dx, VGA_DAC_DATA
    mov cx, 017dh
    cld
dv_0e35:
    mov al, byte ptr [si]
    inc si
    out dx, al
    loop dv_0e35
    sti
    pop ds
    pop si
    pop bp
    retf

; (line): program EGA line compare and overflow bits.
comment !
/* objcmp: {
 "function": "ega_set_line_compare",
 "address": "1899:0e40",
 "size": 51
} */
!
public _ega_set_line_compare
_ega_set_line_compare label far
ega_set_line_compare label far
    push bp
    mov bp, sp
    sti
    mov dx, VGA_STATUS
dv_0e47:
    in al, dx
    test al, 8
    jne dv_0e47
dv_0e4c:
    in al, dx
    test al, 8
    je dv_0e4c
    mov ax, word ptr [bp + 6]
    mov bh, ah
    and bh, 1
    mov cl, 4
    shl bh, cl
    mov ah, al
    mov al, 018h
    mov dx, VGA_CRTC_INDEX
    out dx, ax
    mov ah, 0fh
    or ah, bh
    mov al, 7
    out dx, ax
    mov ah, 0
    mov al, 9
    out dx, ax
    pop bp
    retf

; (line): VGA line compare includes both overflow bits.
comment !
/* objcmp: {
 "function": "vga_set_line_compare",
 "address": "1899:0e73",
 "size": 76
} */
!
public _vga_set_line_compare
_vga_set_line_compare label far
vga_set_line_compare label far
    push bp
    mov bp, sp
    sti
    mov dx, VGA_STATUS
dv_0e7a:
    in al, dx
    test al, 8
    jne dv_0e7a
dv_0e7f:
    in al, dx
    test al, 8
    je dv_0e7f
    mov ax, word ptr [bp + 6]
    mov bh, ah
    mov bl, bh
    and bx, 0201h
    mov cl, 4
    shl bx, cl
    shl bh, 1
    mov ah, al
    mov al, 018h
    mov dx, VGA_CRTC_INDEX
    out dx, ax
    mov al, 7
    out dx, al
    inc dx
    in al, dx
    dec dx
    mov ah, al
    and ah, 0efh
    or ah, bl
    mov al, 7
    out dx, ax
    mov al, 9
    out dx, al
    inc dx
    in al, dx
    dec dx
    mov ah, al
    and ah, 0bfh
    or ah, bh
    mov al, 9
    out dx, ax
    pop bp
    retf

; (x,y): program display-start CRTC address during retrace.
comment !
/* objcmp: {
 "function": "set_display_start",
 "address": "1899:0ebf",
 "size": 60
} */
!
public _set_display_start
_set_display_start label far
set_display_start label far
    push bp
    mov bp, sp
    push si
    mov ax, word ptr [bp + 6]
    mov bx, word ptr [bp + 8]
    mov cx, 8
    div cl
    mov cl, ah
    xor ah, ah
    xchg bx, ax
    mov si, 050h
    mul si
    add bx, ax
    sti
    mov dx, VGA_STATUS
dv_0ede:
    in al, dx
    test al, 8
    jne dv_0ede
dv_0ee3:
    in al, dx
    test al, 8
    je dv_0ee3
    mov dx, VGA_CRTC_INDEX
    mov ah, bh
    mov al, 0ch
    cli
    out dx, ax
    mov ah, bl
    inc al
    out dx, ax
    sti
    sti
    pop si
    pop bp
    retf

; Park up to two BIOS hard disks; CMOS geometry is the fallback.
comment !
/* objcmp: {
 "function": "park_disk_heads",
 "address": "1899:0efb",
 "size": 232
} */
!
public _park_disk_heads
_park_disk_heads label far
park_disk_heads label far
    push si
    mov al, 012h
    call near ptr read_cmos_byte
    or al, al
    jne dv_0f08
    jmp near ptr dv_0fe1
dv_0f08:
    mov byte ptr [_disk_count], 1
    and al, 0fh
    je dv_0f16
    mov byte ptr [_disk_count], 2
dv_0f16:
    push ds
    mov ax, 0
    mov ds, ax
    mov si, 0104h
    les bx, dword ptr [si]
    pop ds
    mov ax, word ptr es:[bx + 0ch]
    mov word ptr [_first_disk_cylinders], ax
    cmp byte ptr [_disk_count], 2
    jne dv_0f43
    push ds
    mov ax, 0
    mov ds, ax
    mov si, 0118h
    les bx, dword ptr [si]
    pop ds
    mov ax, word ptr es:[bx + 0ch]
    mov word ptr [_second_disk_cylinders], ax
dv_0f43:
    mov ax, 01800h
    mov dl, 080h
    int 013h
    jae dv_0f83
    mov ah, 8
    mov dx, 080h
    int 013h
    jb dv_0f66
    mov dx, 080h
    inc ch
    and cl, 0c0h
    mov ax, 0c00h
    int 013h
    jb dv_0f66
    jmp short dv_0f83
dv_0f66:
    mov cx, word ptr [_first_disk_cylinders]
    xchg ch, cl
    shl cl, 1
    shl cl, 1
    shl cl, 1
    shl cl, 1
    shl cl, 1
    shl cl, 1
    and cl, 0c0h
    mov dx, 080h
    mov ax, 0c00h
    int 013h
dv_0f83:
    mov dx, 03c28h
    mov ah, 9
    int 021h
    cmp byte ptr [_disk_count], 2
    je dv_0f93
    jmp short dv_0fda
dv_0f93:
    mov ax, 01800h
    mov dl, 081h
    int 013h
    jae dv_0fd3
    mov ah, 8
    mov dx, 081h
    int 013h
    jb dv_0fb6
    mov dx, 081h
    inc ch
    and cl, 0c0h
    mov ax, 0c00h
    int 013h
    jb dv_0fb6
    jmp short dv_0fd3
dv_0fb6:
    mov cx, word ptr [_second_disk_cylinders]
    xchg ch, cl
    shl cl, 1
    shl cl, 1
    shl cl, 1
    shl cl, 1
    shl cl, 1
    shl cl, 1
    and cl, 0c0h
    mov dx, 081h
    mov ax, 0c00h
    int 013h
dv_0fd3:
    mov dx, 03c36h
    mov ah, 9
    int 021h
dv_0fda:
    mov dx, 03c3dh
    mov ah, 9
    int 021h
dv_0fe1:
    pop si
    retf

; Near helper: AL CMOS index -> AL byte; preserve incoming FLAGS.
comment !
/* objcmp: {
 "function": "read_cmos_byte",
 "address": "1899:0fe3",
 "size": 18
} */
!
public _read_cmos_byte
_read_cmos_byte label near
read_cmos_byte label near
    pushf
    or al, 080h
    mov dx, 070h
    cli
    out dx, al
    in al, 071h
    push ax
    mov al, 0dh
    out dx, al
    sti
    pop ax
    popf
    ret

; Return 1 for post-8086 shift behavior, preserving FLAGS.
comment !
/* objcmp: {
 "function": "cpu_has_post_8086_shifts",
 "address": "1899:0ff5",
 "size": 37
} */
!
public _cpu_has_post_8086_shifts
_cpu_has_post_8086_shifts label far
cpu_has_post_8086_shifts label far
    pushf
    xor ax, ax
    push ax
    popf
    pushf
    pop ax
    and ax, 0f000h
    cmp ax, 0f000h
    je cpu_probe_shift
    jmp short cpu_probe_new_cpu
    jmp short cpu_probe_done
cpu_probe_shift:
    mov ax, 0ffffh
    mov cl, 021h
    shl ax, cl
    jne cpu_probe_new_cpu
    xor ax, ax
    jmp short cpu_probe_done
cpu_probe_new_cpu:
    mov ax, 1
cpu_probe_done:
    popf
    retf

; Recursive plasma: eight word arguments, midpoint/edge colors and subdivision.
comment !
/* objcmp: {
 "function": "subdivide_plasma",
 "address": "1899:101a",
 "size": 803
} */
!
public _subdivide_plasma
_subdivide_plasma label far
subdivide_plasma label far
    push bp
    mov bp, sp
    sub sp, 010h
    mov ax, word ptr [_palette_cycle_ticks]
    cmp word ptr [_palette_elapsed_ticks], ax
    jb dv_102e
    call far ptr _cycle_palette
dv_102e:
    cmp word ptr [_input_pending], 0
    je dv_1043
    call far ptr _poll_graphics_keys
    cmp ax, 0
    je dv_1043
    mov sp, bp
    pop bp
    retf
dv_1043:
    cmp byte ptr [_g_5d92], 06eh
    jne dv_104e
    mov sp, bp
    pop bp
    retf
dv_104e:
    mov ax, word ptr [bp + 0ah]
    mov si, ax
    mov bx, word ptr [bp + 0ch]
    mov di, bx
    mov cx, word ptr [bp + 6]
    mov dx, word ptr [bp + 8]
    sub si, cx
    cmp si, 2
    jge dv_1070
    sub di, dx
    cmp di, 2
    jge dv_1070
    mov sp, bp
    pop bp
    retf
dv_1070:
    mov si, ax
    add si, cx
    shr si, 1
    adc si, 0
    mov word ptr [bp - 4], si
    mov di, bx
    add di, dx
    shr di, 1
    adc di, 0
    mov word ptr [bp - 6], di
    push word ptr [bp - 6]
    push word ptr [bp - 4]
    call dword ptr [_read_pixel_callback]
    add sp, 4
    mov dx, word ptr [_background_fill_word]
    cmp al, dl
    je dv_10a1
    cmp al, dh
    jne dv_10d5
dv_10a1:
    mov dx, word ptr [bp + 0eh]
    add dx, word ptr [bp + 010h]
    add dx, word ptr [bp + 012h]
    add dx, word ptr [bp + 014h]
    shr dx, 1
    shr dx, 1
    mov word ptr [_plot_color], dx
    mov ax, word ptr [bp - 4]
    mov word ptr [_draw_x], ax
    mov ax, word ptr [bp - 6]
    mov word ptr [_draw_y], ax
    push ax
    push bp
    mov bp, sp
    mov word ptr [bp + 2], 1
    pop bp
    call dword ptr [_plot_callback]
    add sp, 2
    mov ax, word ptr [_plot_color]
dv_10d5:
    mov word ptr [bp - 8], ax
    mov bx, word ptr [bp + 0ah]
    sub bx, word ptr [bp + 6]
    add bx, word ptr [bp + 0ch]
    sub bx, word ptr [bp + 8]
    shl bx, 1
    mov word ptr [bp - 2], bx
    push word ptr [bp + 8]
    push word ptr [bp - 4]
    call dword ptr [_read_pixel_callback]
    add sp, 4
    mov dx, word ptr [_background_fill_word]
    cmp al, dl
    je dv_1102
    cmp al, dh
    jne dv_115b
dv_1102:
    mov cx, word ptr [_plasma_random]
    add cx, 09248h
    ror cx, 1
    ror cx, 1
    ror cx, 1
    mov word ptr [_plasma_random], cx
    mov ax, word ptr [bp - 2]
    imul cx
    add dx, word ptr [bp + 0eh]
    add dx, word ptr [bp + 010h]
    sar dx, 1
    cmp dx, 4
    jl dv_112e
    cmp dx, word ptr [_plasma_color_limit]
    jg dv_1133
    jmp short dv_1137
dv_112e:
    mov dx, 3
    jmp short dv_1137
dv_1133:
    mov dx, word ptr [_plasma_color_limit]
dv_1137:
    mov word ptr [_plot_color], dx
    mov ax, word ptr [bp - 4]
    mov word ptr [_draw_x], ax
    mov ax, word ptr [bp + 8]
    mov word ptr [_draw_y], ax
    push ax
    push bp
    mov bp, sp
    mov word ptr [bp + 2], 1
    pop bp
    call dword ptr [_plot_callback]
    add sp, 2
    mov ax, word ptr [_plot_color]
dv_115b:
    mov word ptr [bp - 0ah], ax
    push word ptr [bp - 6]
    push word ptr [bp + 6]
    call dword ptr [_read_pixel_callback]
    add sp, 4
    mov dx, word ptr [_background_fill_word]
    cmp al, dl
    je dv_1177
    cmp al, dh
    jne dv_11d0
dv_1177:
    mov cx, word ptr [_plasma_random]
    add cx, 09248h
    ror cx, 1
    ror cx, 1
    ror cx, 1
    mov word ptr [_plasma_random], cx
    mov ax, word ptr [bp - 2]
    imul cx
    add dx, word ptr [bp + 0eh]
    add dx, word ptr [bp + 014h]
    sar dx, 1
    cmp dx, 4
    jl dv_11a3
    cmp dx, word ptr [_plasma_color_limit]
    jg dv_11a8
    jmp short dv_11ac
dv_11a3:
    mov dx, 3
    jmp short dv_11ac
dv_11a8:
    mov dx, word ptr [_plasma_color_limit]
dv_11ac:
    mov word ptr [_plot_color], dx
    mov ax, word ptr [bp + 6]
    mov word ptr [_draw_x], ax
    mov ax, word ptr [bp - 6]
    mov word ptr [_draw_y], ax
    push ax
    push bp
    mov bp, sp
    mov word ptr [bp + 2], 1
    pop bp
    call dword ptr [_plot_callback]
    add sp, 2
    mov ax, word ptr [_plot_color]
dv_11d0:
    mov word ptr [bp - 010h], ax
    push word ptr [bp - 6]
    push word ptr [bp + 0ah]
    call dword ptr [_read_pixel_callback]
    add sp, 4
    mov dx, word ptr [_background_fill_word]
    cmp al, dl
    je dv_11ec
    cmp al, dh
    jne dv_1245
dv_11ec:
    mov cx, word ptr [_plasma_random]
    add cx, 09248h
    ror cx, 1
    ror cx, 1
    ror cx, 1
    mov word ptr [_plasma_random], cx
    mov ax, word ptr [bp - 2]
    imul cx
    add dx, word ptr [bp + 010h]
    add dx, word ptr [bp + 012h]
    sar dx, 1
    cmp dx, 4
    jl dv_1218
    cmp dx, word ptr [_plasma_color_limit]
    jg dv_121d
    jmp short dv_1221
dv_1218:
    mov dx, 3
    jmp short dv_1221
dv_121d:
    mov dx, word ptr [_plasma_color_limit]
dv_1221:
    mov word ptr [_plot_color], dx
    mov ax, word ptr [bp + 0ah]
    mov word ptr [_draw_x], ax
    mov ax, word ptr [bp - 6]
    mov word ptr [_draw_y], ax
    push ax
    push bp
    mov bp, sp
    mov word ptr [bp + 2], 1
    pop bp
    call dword ptr [_plot_callback]
    add sp, 2
    mov ax, word ptr [_plot_color]
dv_1245:
    mov word ptr [bp - 0ch], ax
    push word ptr [bp + 0ch]
    push word ptr [bp - 4]
    call dword ptr [_read_pixel_callback]
    add sp, 4
    mov dx, word ptr [_background_fill_word]
    cmp al, dl
    je dv_1261
    cmp al, dh
    jne dv_12ba
dv_1261:
    mov cx, word ptr [_plasma_random]
    add cx, 09248h
    ror cx, 1
    ror cx, 1
    ror cx, 1
    mov word ptr [_plasma_random], cx
    mov ax, word ptr [bp - 2]
    imul cx
    add dx, word ptr [bp + 014h]
    add dx, word ptr [bp + 012h]
    sar dx, 1
    cmp dx, 4
    jl dv_128d
    cmp dx, word ptr [_plasma_color_limit]
    jg dv_1292
    jmp short dv_1296
dv_128d:
    mov dx, 3
    jmp short dv_1296
dv_1292:
    mov dx, word ptr [_plasma_color_limit]
dv_1296:
    mov word ptr [_plot_color], dx
    mov ax, word ptr [bp - 4]
    mov word ptr [_draw_x], ax
    mov ax, word ptr [bp + 0ch]
    mov word ptr [_draw_y], ax
    push ax
    push bp
    mov bp, sp
    mov word ptr [bp + 2], 1
    pop bp
    call dword ptr [_plot_callback]
    add sp, 2
    mov ax, word ptr [_plot_color]
dv_12ba:
    mov word ptr [bp - 0eh], ax
    push word ptr [bp - 010h]
    push word ptr [bp - 8]
    push word ptr [bp - 0ah]
    push word ptr [bp + 0eh]
    push word ptr [bp - 6]
    push word ptr [bp - 4]
    push word ptr [bp + 8]
    push word ptr [bp + 6]
    push cs
    call near ptr subdivide_plasma
    add sp, 010h
    push word ptr [bp - 8]
    push word ptr [bp - 0ch]
    push word ptr [bp + 010h]
    push word ptr [bp - 0ah]
    push word ptr [bp - 6]
    push word ptr [bp + 0ah]
    push word ptr [bp + 8]
    push word ptr [bp - 4]
    push cs
    call near ptr subdivide_plasma
    add sp, 010h
    push word ptr [bp - 0eh]
    push word ptr [bp + 012h]
    push word ptr [bp - 0ch]
    push word ptr [bp - 8]
    push word ptr [bp + 0ch]
    push word ptr [bp + 0ah]
    push word ptr [bp - 6]
    push word ptr [bp - 4]
    push cs
    call near ptr subdivide_plasma
    add sp, 010h
    push word ptr [bp + 014h]
    push word ptr [bp - 0eh]
    push word ptr [bp - 8]
    push word ptr [bp - 010h]
    push word ptr [bp + 0ch]
    push word ptr [bp - 4]
    push word ptr [bp - 6]
    push word ptr [bp + 6]
    push cs
    call near ptr subdivide_plasma
    add sp, 010h
    mov sp, bp
    pop bp
    retf

; (width,height): seed corner colors and call the subdivision kernel.
comment !
/* objcmp: {
 "function": "seed_plasma",
 "address": "1899:133d",
 "size": 394
} */
!
public _seed_plasma
_seed_plasma label far
seed_plasma label far
    push bp
    mov bp, sp
    sub sp, 8
    push di
    push si
    mov ax, word ptr [_screen_half_width]
    dec ax
    mov word ptr [_plasma_width], ax
    mov ax, word ptr [_screen_half_height]
    dec ax
    mov word ptr [_plasma_height], ax
    mov ax, word ptr [_color_count]
    dec ax
    mov word ptr [_plasma_color_limit], ax
    mov ah, 02ch
    int 021h
    mov word ptr [_plasma_random], dx
    mov cx, word ptr [_plasma_random]
    add cx, 09248h
    ror cx, 1
    ror cx, 1
    ror cx, 1
    mov word ptr [_plasma_random], cx
    mov dl, ch
    xor dh, dh
    cmp dx, 4
    jl dv_1385
    cmp dx, word ptr [_plasma_color_limit]
    jg dv_138a
    jmp short dv_138e
dv_1385:
    mov dx, 3
    jmp short dv_138e
dv_138a:
    mov dx, word ptr [_plasma_color_limit]
dv_138e:
    mov word ptr [bp - 2], dx
    mov word ptr [_plot_color], dx
    mov ax, 0
    mov word ptr [_draw_x], ax
    mov ax, 0
    mov word ptr [_draw_y], ax
    push ax
    push bp
    mov bp, sp
    mov word ptr [bp + 2], 1
    pop bp
    call dword ptr [_plot_callback]
    add sp, 2
    mov cx, word ptr [_plasma_random]
    add cx, 09248h
    ror cx, 1
    ror cx, 1
    ror cx, 1
    mov word ptr [_plasma_random], cx
    mov dl, ch
    xor dh, dh
    cmp dx, 4
    jl dv_13d5
    cmp dx, word ptr [_plasma_color_limit]
    jg dv_13da
    jmp short dv_13de
dv_13d5:
    mov dx, 3
    jmp short dv_13de
dv_13da:
    mov dx, word ptr [_plasma_color_limit]
dv_13de:
    mov word ptr [bp - 4], dx
    mov word ptr [_plot_color], dx
    mov ax, word ptr [_plasma_width]
    mov word ptr [_draw_x], ax
    mov ax, 0
    mov word ptr [_draw_y], ax
    push ax
    push bp
    mov bp, sp
    mov word ptr [bp + 2], 1
    pop bp
    call dword ptr [_plot_callback]
    add sp, 2
    mov cx, word ptr [_plasma_random]
    add cx, 09248h
    ror cx, 1
    ror cx, 1
    ror cx, 1
    mov word ptr [_plasma_random], cx
    mov dl, ch
    xor dh, dh
    cmp dx, 4
    jl dv_1425
    cmp dx, word ptr [_plasma_color_limit]
    jg dv_142a
    jmp short dv_142e
dv_1425:
    mov dx, 3
    jmp short dv_142e
dv_142a:
    mov dx, word ptr [_plasma_color_limit]
dv_142e:
    mov word ptr [bp - 6], dx
    mov word ptr [_plot_color], dx
    mov ax, word ptr [_plasma_width]
    mov word ptr [_draw_x], ax
    mov ax, word ptr [_plasma_height]
    mov word ptr [_draw_y], ax
    push ax
    push bp
    mov bp, sp
    mov word ptr [bp + 2], 1
    pop bp
    call dword ptr [_plot_callback]
    add sp, 2
    mov cx, word ptr [_plasma_random]
    add cx, 09248h
    ror cx, 1
    ror cx, 1
    ror cx, 1
    mov word ptr [_plasma_random], cx
    mov dl, ch
    xor dh, dh
    cmp dx, 4
    jl dv_1475
    cmp dx, word ptr [_plasma_color_limit]
    jg dv_147a
    jmp short dv_147e
dv_1475:
    mov dx, 3
    jmp short dv_147e
dv_147a:
    mov dx, word ptr [_plasma_color_limit]
dv_147e:
    mov word ptr [bp - 8], dx
    mov word ptr [_plot_color], dx
    mov ax, 0
    mov word ptr [_draw_x], ax
    mov ax, word ptr [_plasma_height]
    mov word ptr [_draw_y], ax
    push ax
    push bp
    mov bp, sp
    mov word ptr [bp + 2], 1
    pop bp
    call dword ptr [_plot_callback]
    add sp, 2
    push word ptr [bp - 8]
    push word ptr [bp - 6]
    push word ptr [bp - 4]
    push word ptr [bp - 2]
    push word ptr [_plasma_height]
    push word ptr [_plasma_width]
    xor ax, ax
    push ax
    push ax
    push cs
    call near ptr subdivide_plasma
    add sp, 010h
    pop si
    pop di
    mov sp, bp
    pop bp
    retf

; INT 1Ch: count ticks, poll keys every fifth tick, chain saved BIOS vector.
comment !
/* objcmp: {
 "function": "graphics_timer_tick_isr",
 "address": "1899:14c7",
 "size": 133
} */
!
public _graphics_timer_tick_isr
_graphics_timer_tick_isr label far
graphics_timer_tick_isr label far
    push ds
    push ax
    push bx
    push es
    pushf
    sti
    mov ax, DGROUP
    mov ds, ax
    inc word ptr [_palette_elapsed_ticks]
    cmp word ptr [_countdown_ticks], 0
    je timer_show_countdown
    dec word ptr [_countdown_ticks]
timer_show_countdown:
    cmp word ptr [_g_5d86], 0
    je timer_poll_divider
    dec word ptr [_g_5d86]
timer_poll_divider:
    dec word ptr [_input_poll_divider]
    cmp word ptr [_input_poll_divider], 0
    jne timer_chain
    mov word ptr [_input_poll_divider], 5
    mov ax, 040h
    mov es, ax
    cli
    mov ax, word ptr es:[01ah]
    cmp ax, word ptr es:[01ch]
    sti
    je timer_check_screensaver
    cmp word ptr [_input_pending], 0
    jne timer_check_screensaver
    inc word ptr [_input_pending]
timer_check_screensaver:
    cmp word ptr [_mouse_callback_installed], 0
    je timer_chain
    les bx, dword ptr [_current_image_header]
    cmp word ptr es:[bx + 012h], 0777h
    jne timer_chain
    test word ptr es:[bx + 0ch], 01000h
    je timer_chain
    inc word ptr [_exit_requested]
    inc word ptr [_input_pending]
    and word ptr es:[bx + 0ch], 0efffh
timer_chain:
    call dword ptr [_saved_timer_vector]
    pop es
    pop bx
    pop ax
    pop ds
    iret

; INT 9: count pending input and chain the old keyboard handler.
comment !
/* objcmp: {
 "function": "keyboard_pending_isr",
 "address": "1899:154c",
 "size": 19
} */
!
public _keyboard_pending_isr
_keyboard_pending_isr label far
keyboard_pending_isr label far
    push ds
    push ax
    pushf
    mov ax, DGROUP
    mov ds, ax
    inc word ptr [_input_pending]
    call dword ptr [_saved_keyboard_vector]
    pop ax
    pop ds
    iret

; Mouse callback: flag pending input and mouse abort; preserve DS/BP.
comment !
/* objcmp: {
 "function": "mouse_pending_callback",
 "address": "1899:155f",
 "size": 18
} */
!
public _mouse_pending_callback
_mouse_pending_callback label far
mouse_pending_callback label far
    push ds
    push bp
    mov bp, DGROUP
    mov ds, bp
    inc word ptr [_input_pending]
    inc word ptr [_mouse_abort_pending]
    pop bp
    pop ds
    retf

; Return the actual DVDT CS. Caller 11C1:0394 proves 1899:1571.
comment !
/* objcmp: {
 "function": "video_code_segment",
 "address": "1899:1571",
 "size": 3
} */
!
public _video_code_segment
_video_code_segment label far
video_code_segment label far
    push cs
    pop ax
    retf
DVDT6 ends
end
