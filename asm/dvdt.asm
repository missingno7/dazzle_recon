; DVDT hardware and plasma kernels. TASM 3.0 /ml /m2. See docs/asm-origin.md.
; Hand-authored assembly evidence: parity branches, latch reads, explicit FLAGS/DS,
; ENTER/LEAVE fast palette writers and separate 8086 versions, manual call slots.
; Entry ABI: far caller cleanup unless marked near/interrupt; BP stack offsets
; below follow PUSH BP / MOV BP,SP. Far pointers are offset:segment pairs.
; DS is DGROUP unless an entry saves it for a far buffer. Normal entries
; use AX/BX/CX/DX/ES as scratch; saves in each body specify other preservation.
; Void entries have no defined AX result. Plot/read state is not reentrant.
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

; Indexed port writes use AL = register index and AH = register value.
SEQ_MAP_MASK equ 2
SEQ_MEMORY_MODE equ 4
GC_READ_MAP equ 4
GC_MODE equ 5
GC_MISC equ 6
GC_BIT_MASK equ 8
; Packed index/data words for a single OUT DX,AX.
GC_WRITE_MODE_2 equ 0205h
GC_ALL_PIXEL_BITS equ 0ff08h
SEQ_ALL_PLANES equ 0f02h
CRTC_OVERFLOW equ 7
CRTC_MAX_SCAN equ 9
CRTC_START_HIGH equ 0ch
CRTC_UNDERLINE equ 014h
CRTC_MODE equ 017h
CRTC_LINE_COMPARE equ 018h
; Input status 1 bit 3 is high during vertical retrace.
STATUS_VERTICAL_RETRACE equ 8
ATTR_BORDER_DISPLAY_ON equ 031h
EGA_BORDER_ENTRY equ 010h
CMOS_INDEX equ 070h
CMOS_DATA equ 071h
CMOS_DISK_TYPES equ 012h
CMOS_STATUS_D equ 0dh

BIOS_VIDEO_INT equ 010h
BIOS_DISK_INT equ 013h
DOS_INT equ 021h
BIOS_GET_MODE equ 0f00h
BIOS_GET_CURSOR equ 0300h
BIOS_SET_CURSOR equ 0100h
BIOS_DISPLAY_COMBINATION equ 01a00h
BIOS_EGA_INFO equ 01200h
BIOS_EGA_PALETTE equ 01002h
BIOS_DAC_BLOCK equ 01012h
BIOS_LOAD_8X8_FONT equ 01112h
BIOS_DISK_SEEK equ 0c00h
BIOS_DISK_PARAMETERS equ 8
DOS_PRINT_STRING equ 9
DOS_GET_TIME equ 02ch
BIOS_DATA_SEGMENT equ 040h
BDA_TEXT_LAST_ROW equ 084h
BDA_EGA_INFO equ 087h
BDA_KEY_HEAD equ 01ah
BDA_KEY_TAIL equ 01ch
EGA_BIOS_MODE equ 010h
EGA_BIOS_NO_CLEAR equ 090h
VGA_BIOS_MODE equ 013h
VGA_BIOS_NO_CLEAR equ 093h
EGA_PHYSICAL_ROWS equ 350
PLANAR_ROW_BYTES equ 80
FIRST_CYCLING_COLOR equ 3
KEY_CLEAR_SCREEN equ 06eh

; Stack arguments and local words; local equates are distances below BP.
RGB_DESTINATION equ 6
RGB_SOURCE equ 0ah
RGB_COLOR_COUNT equ 0eh
ROW_SOURCE equ 6
ROW_BYTE_COUNT equ 0ah
ROW_INDEX equ 0ch
PLOT_SYMMETRY equ 6
PLOT_X_PLUS equ 2
PLOT_X_MINUS equ 4
PLOT_Y_PLUS equ 6
PLOT_Y_MINUS equ 8
PIXEL_X_ARGUMENT equ 6
PIXEL_Y_ARGUMENT equ 8
PIXEL_X equ 2
PIXEL_Y equ 4
MODE_CLEAR equ 6
MODE_RESTORE equ 6
CLEAR_PATTERN equ 6
CLEAR_WORD_COUNT equ 8
CLEAR_PAGE_PARAGRAPH equ 0ah
SPLIT_LINE equ 6
DISPLAY_X equ 6
DISPLAY_Y equ 8
PLASMA_X0 equ 6
PLASMA_Y0 equ 8
PLASMA_X1 equ 0ah
PLASMA_Y1 equ 0ch
PLASMA_TOP_LEFT equ 0eh
PLASMA_TOP_RIGHT equ 010h
PLASMA_BOTTOM_RIGHT equ 012h
PLASMA_BOTTOM_LEFT equ 014h
PLASMA_NOISE_SCALE equ 2
PLASMA_MID_X equ 4
PLASMA_MID_Y equ 6
PLASMA_CENTER_COLOR equ 8
PLASMA_TOP_COLOR equ 0ah
PLASMA_RIGHT_COLOR equ 0ch
PLASMA_BOTTOM_COLOR equ 0eh
PLASMA_LEFT_COLOR equ 010h
SEED_TOP_LEFT equ 2
SEED_TOP_RIGHT equ 4
SEED_BOTTOM_RIGHT equ 6
SEED_BOTTOM_LEFT equ 8

PAL_ARGUMENT equ 6
PAL_OLD0 equ -2
PAL_OLD1 equ -4
PAL_OLD2 equ -6
PAL_OLD_BORDER equ -8
_DATA segment word public 'DATA'
extrn _background_fill_word:WORD
extrn _countdown_ticks:WORD
extrn _current_graphics_key:WORD
extrn _palette_cycle_ticks:WORD
extrn _draw_delay_count:WORD
extrn _exit_requested:WORD
extrn _current_image_header:WORD
extrn _mouse_callback_installed:WORD
extrn _palette_base_green:WORD
extrn _palette_base_blue:WORD
extrn _ega_border_color:WORD
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
extrn _show_duration_ticks:WORD

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
; Explicit zero emission ends the serialized image at the row-table end.
; Later C/RTL BSS and the COMMON stack reserve memory without file bytes.
_BSS segment word public 'BSS'
public _force_ega
_force_ega dw 0
public _force_vga
_force_vga dw 0
public _video_adapter
_video_adapter dw 0
public _saved_bios_mode
_saved_bios_mode dw 0
public _plot_color
_plot_color dw 0
public _saved_cursor_shape
_saved_cursor_shape dw 0
public _saved_text_last_row
_saved_text_last_row dw 0
public _ega_memory_code
_ega_memory_code dw 0
public _page_paragraph
_page_paragraph dw 0
public _plasma_random
_plasma_random dw 0
public _plasma_width
_plasma_width dw 0
public _plasma_height
_plasma_height dw 0
public _plasma_color_limit
_plasma_color_limit dw 0
public _ega_row_offsets
_ega_row_offsets dw 350 dup (0)
_BSS ends
DGROUP group _DATA, _BSS
assume ds:DGROUP
extrn _print_closing_text_and_exit:FAR
extrn _poll_graphics_keys:FAR
extrn _cycle_palette:FAR
DVDT6 segment word public 'CODE'
assume cs:DVDT6

; Quantize RGB bytes at thresholds 33h/77h/BBh into six-bit EGA palette codes.
; Stack +6: far destination, +Ah: far RGB source, +Eh: color count; no result.
; Writes one byte per color to the destination; preserves DS; no port/DGROUP writes.
; 1899:0006, 134 bytes
public _rgb8_to_ega_palette
_rgb8_to_ega_palette label far
rgb8_to_ega_palette label far
    push bp
    mov bp, sp
    push di
    push si
    push ds
    les di, dword ptr [bp + RGB_DESTINATION]
    lds si, dword ptr [bp + RGB_SOURCE]
    mov cx, word ptr [bp + RGB_COLOR_COUNT]
    sub bx, bx
    ; EGA color bits are secondary RGB in bits 5..3 and primary RGB in bits 2..0.
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

; Copy four consecutive source planes to one physical EGA row at A000h.
; Stack +6: far source, +Ah: bytes per plane, +Ch: row; no result.
; Uses ega_row_offsets; writes sequencer map mask 1/2/4/8, then 0Fh; restores DS.
; 1899:008C, 128 bytes
public _ega_write_planar_row
_ega_write_planar_row label far
ega_write_planar_row label far
    push bp
    mov bp, sp
    push di
    push si
    push ds
    lds si, dword ptr [bp + ROW_SOURCE]
    mov dx, word ptr [bp + ROW_INDEX]
    cmp dx, EGA_PHYSICAL_ROWS
    jge row_transfer_done
    push dx
    push si
    push ds
    push ax
    mov ax, DGROUP
    mov ds, ax
    pop ax
    mov si, offset DGROUP:_ega_row_offsets
    shl dx, 1
    add si, dx
    mov di, word ptr [si]
    pop ds
    pop si
    pop dx
    mov ax, VGA_MEMORY
    mov es, ax
    mov bx, word ptr [bp + ROW_BYTE_COUNT]
    mov cx, bx
    ; Each source plane uses the same destination row; SI advances across all four.
    mov al, SEQ_MAP_MASK
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
    mov al, SEQ_MAP_MASK
    mov dx, VGA_SEQ_INDEX
    out dx, al
    inc dx
    mov al, 2
    out dx, al
    push di
    repne movsb
    pop di
    mov cx, bx
    mov al, SEQ_MAP_MASK
    mov dx, VGA_SEQ_INDEX
    out dx, al
    inc dx
    mov al, 4
    out dx, al
    push di
    repne movsb
    pop di
    mov cx, bx
    mov al, SEQ_MAP_MASK
    mov dx, VGA_SEQ_INDEX
    out dx, al
    inc dx
    mov al, 8
    out dx, al
    push di
    repne movsb
    pop di
    mov al, SEQ_MAP_MASK
    mov dx, VGA_SEQ_INDEX
    out dx, al
    inc dx
    ; Re-enable all four write planes for subsequent drawing.
    mov al, 0fh
    out dx, al
row_transfer_done:
    pop ds
    pop si
    pop di
    pop bp
    retf

; Build ega_row_offsets[y] = 80*y for 350 rows, then set BIOS video mode 10h.
; No arguments/result; writes the DGROUP row table and changes the display mode.
; 1899:010C, 33 bytes
public _ega_init_row_table_and_mode
_ega_init_row_table_and_mode label far
ega_init_row_table_and_mode label far
    push di
    push si
    mov cx, EGA_PHYSICAL_ROWS
    sub dx, dx
    mov si, offset DGROUP:_ega_row_offsets
row_offset_next:
    push dx
    mov ax, PLANAR_ROW_BYTES
    mul dx
    mov word ptr [si], ax
    add si, 2
    pop dx
    inc dx
    loop row_offset_next
    mov ax, EGA_BIOS_MODE
    int BIOS_VIDEO_INT
    pop si
    pop di
    retf

; Plot draw_x/draw_y in plot_color about the screen center on page_paragraph.
; Stack +6: symmetry flags (bit 0 reflects, low bits 3 also swap axes); no result.
; Rejects odd virtual rows, halves Y; writes edge flags, VRAM and GC bit mask.
; 1899:012D, 443 bytes
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
    ; A zero LOOP count wraps; CH is masked again on every delay iteration.
    mov cx, word ptr [_draw_delay_count]
ega700_delay:
    and ch, 01fh
    loop ega700_delay
    mov bl, byte ptr [_plot_color]
    mov dx, VGA_GC_INDEX
    xor ax, ax
    ; One word clears the adjacent X and Y edge flags together.
    mov word ptr [_symmetry_edge], ax
    mov ax, word ptr [_screen_half_width]
    mov si, word ptr [_draw_x]
    cmp ax, si
    jg ega700_sum_x
    inc byte ptr [_symmetry_edge]
ega700_sum_x:
    add ax, si
    js ega700_outside
    mov word ptr [bp - PLOT_X_PLUS], ax
    mov ax, word ptr [_screen_half_width]
    sub ax, si
    js ega700_outside
    mov word ptr [bp - PLOT_X_MINUS], ax
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
    mov word ptr [bp - PLOT_Y_PLUS], ax
    mov ax, word ptr [_screen_half_height]
    sub ax, si
    js ega700_outside
    test al, 1
    jne ega700_outside
    sar ax, 1
    mov word ptr [bp - PLOT_Y_MINUS], ax
    jmp short ega700_plot_quadrants
ega700_outside:
    jmp near ptr ega700_check_axis_swap
ega700_plot_quadrants:
    cmp word ptr [_symmetry_edge], 0
    jne ega700_check_symmetry
    mov cl, byte ptr [bp - PLOT_X_PLUS]
    and cl, 7
    ; AH starts at bit 7; shifting by X&7 forms the GC pixel mask in AH.
    mov ax, 08000h
    shr ax, cl
    ; AL selects GC bit mask; OUT DX,AX writes the index and mask together.
    add ax, GC_BIT_MASK
    out dx, ax
    mov ax, word ptr [bp - PLOT_X_PLUS]
    sar ax, 1
    sar ax, 1
    sar ax, 1
    mov di, ax
    mov ax, word ptr [bp - PLOT_Y_PLUS]
    mov cl, 4
    shl ax, cl
    add di, ax
    shl ax, 1
    shl ax, 1
    add di, ax
    ; Read the EGA latches before the write-mode-2 masked color store.
    mov al, byte ptr es:[di]
    mov byte ptr es:[di], bl
ega700_check_symmetry:
    test word ptr [bp + PLOT_SYMMETRY], 1
    jne ega700_plot_reflections
    jmp near ptr ega700_return
ega700_plot_reflections:
    cmp byte ptr [_symmetry_edge], 0
    jne ega700_left_bottom
    mov cl, byte ptr [bp - PLOT_X_PLUS]
    and cl, 7
    mov ax, 08000h
    shr ax, cl
    add ax, GC_BIT_MASK
    out dx, ax
    mov ax, word ptr [bp - PLOT_X_PLUS]
    sar ax, 1
    sar ax, 1
    sar ax, 1
    mov di, ax
    mov ax, word ptr [bp - PLOT_Y_MINUS]
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
    mov cl, byte ptr [bp - PLOT_X_MINUS]
    and cl, 7
    mov ax, 08000h
    shr ax, cl
    add ax, GC_BIT_MASK
    out dx, ax
    mov ax, word ptr [bp - PLOT_X_MINUS]
    sar ax, 1
    sar ax, 1
    sar ax, 1
    mov di, ax
    mov ax, word ptr [bp - PLOT_Y_PLUS]
    mov cl, 4
    shl ax, cl
    add di, ax
    shl ax, 1
    shl ax, 1
    add di, ax
    mov al, byte ptr es:[di]
    mov byte ptr es:[di], bl
ega700_left_top:
    mov cl, byte ptr [bp - PLOT_X_MINUS]
    and cl, 7
    mov ax, 08000h
    shr ax, cl
    add ax, GC_BIT_MASK
    out dx, ax
    mov ax, word ptr [bp - PLOT_X_MINUS]
    sar ax, 1
    sar ax, 1
    sar ax, 1
    mov di, ax
    mov ax, word ptr [bp - PLOT_Y_MINUS]
    mov cl, 4
    shl ax, cl
    add di, ax
    shl ax, 1
    shl ax, 1
    add di, ax
    mov al, byte ptr es:[di]
    mov byte ptr es:[di], bl
ega700_check_axis_swap:
    ; After the zero case, even parity selects low bits 11b for the axis-swapped pass.
    test word ptr [bp + PLOT_SYMMETRY], 3
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
    mov word ptr [bp - PLOT_X_PLUS], ax
    mov ax, word ptr [_screen_half_width]
    sub ax, si
    js ega700_return
    mov word ptr [bp - PLOT_X_MINUS], ax
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
    mov word ptr [bp - PLOT_Y_PLUS], ax
    mov ax, word ptr [_screen_half_height]
    sub ax, si
    js ega700_return
    test al, 1
    jne ega700_return
    sar ax, 1
    mov word ptr [bp - PLOT_Y_MINUS], ax
    ; Reuse the caller's argument slot: the swapped pass needs reflections only.
    mov word ptr [bp + PLOT_SYMMETRY], 1
    jmp near ptr ega700_plot_quadrants
ega700_return:
    pop di
    pop si
    mov sp, bp
    pop bp
    retf

; Plot draw_x/draw_y in plot_color about the screen center on page_paragraph.
; Stack +6: symmetry flags (bit 0 reflects, low bits 3 also swap axes); no result.
; Uses physical EGA rows; writes symmetry edge flags, VRAM and GC bit mask.
; 1899:02E8, 419 bytes
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
    ; A zero LOOP count wraps; CH is masked again on every delay iteration.
    mov cx, word ptr [_draw_delay_count]
ega350_delay:
    and ch, 01fh
    loop ega350_delay
    mov bl, byte ptr [_plot_color]
    mov dx, VGA_GC_INDEX
    xor ax, ax
    ; One word clears the adjacent X and Y edge flags together.
    mov word ptr [_symmetry_edge], ax
    mov ax, word ptr [_screen_half_width]
    mov si, word ptr [_draw_x]
    cmp ax, si
    jg ega350_sum_x
    inc byte ptr [_symmetry_edge]
ega350_sum_x:
    add ax, si
    js ega350_outside
    mov word ptr [bp - PLOT_X_PLUS], ax
    mov ax, word ptr [_screen_half_width]
    sub ax, si
    js ega350_outside
    mov word ptr [bp - PLOT_X_MINUS], ax
    mov ax, word ptr [_screen_half_height]
    mov si, word ptr [_draw_y]
    cmp ax, si
    jg ega350_sum_y
    inc byte ptr [_symmetry_y_edge]
ega350_sum_y:
    add ax, si
    js ega350_outside
    mov word ptr [bp - PLOT_Y_PLUS], ax
    mov ax, word ptr [_screen_half_height]
    sub ax, si
    js ega350_outside
    mov word ptr [bp - PLOT_Y_MINUS], ax
    jmp short ega350_plot_quadrants
ega350_outside:
    jmp near ptr ega350_check_axis_swap
ega350_plot_quadrants:
    cmp word ptr [_symmetry_edge], 0
    jne ega350_check_symmetry
    mov cl, byte ptr [bp - PLOT_X_PLUS]
    and cl, 7
    ; AH starts at bit 7; shifting by X&7 forms the GC pixel mask in AH.
    mov ax, 08000h
    shr ax, cl
    ; AL selects GC bit mask; OUT DX,AX writes the index and mask together.
    add ax, GC_BIT_MASK
    out dx, ax
    mov ax, word ptr [bp - PLOT_X_PLUS]
    sar ax, 1
    sar ax, 1
    sar ax, 1
    mov di, ax
    mov ax, word ptr [bp - PLOT_Y_PLUS]
    mov cl, 4
    shl ax, cl
    add di, ax
    shl ax, 1
    shl ax, 1
    add di, ax
    ; Read the EGA latches before the write-mode-2 masked color store.
    mov al, byte ptr es:[di]
    mov byte ptr es:[di], bl
ega350_check_symmetry:
    test word ptr [bp + PLOT_SYMMETRY], 1
    jne ega350_plot_reflections
    jmp near ptr ega350_return
ega350_plot_reflections:
    cmp byte ptr [_symmetry_edge], 0
    jne ega350_left_bottom
    mov cl, byte ptr [bp - PLOT_X_PLUS]
    and cl, 7
    mov ax, 08000h
    shr ax, cl
    add ax, GC_BIT_MASK
    out dx, ax
    mov ax, word ptr [bp - PLOT_X_PLUS]
    sar ax, 1
    sar ax, 1
    sar ax, 1
    mov di, ax
    mov ax, word ptr [bp - PLOT_Y_MINUS]
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
    mov cl, byte ptr [bp - PLOT_X_MINUS]
    and cl, 7
    mov ax, 08000h
    shr ax, cl
    add ax, GC_BIT_MASK
    out dx, ax
    mov ax, word ptr [bp - PLOT_X_MINUS]
    sar ax, 1
    sar ax, 1
    sar ax, 1
    mov di, ax
    mov ax, word ptr [bp - PLOT_Y_PLUS]
    mov cl, 4
    shl ax, cl
    add di, ax
    shl ax, 1
    shl ax, 1
    add di, ax
    mov al, byte ptr es:[di]
    mov byte ptr es:[di], bl
dv_03fe:
    mov cl, byte ptr [bp - PLOT_X_MINUS]
    and cl, 7
    mov ax, 08000h
    shr ax, cl
    add ax, GC_BIT_MASK
    out dx, ax
    mov ax, word ptr [bp - PLOT_X_MINUS]
    sar ax, 1
    sar ax, 1
    sar ax, 1
    mov di, ax
    mov ax, word ptr [bp - PLOT_Y_MINUS]
    mov cl, 4
    shl ax, cl
    add di, ax
    shl ax, 1
    shl ax, 1
    add di, ax
    mov al, byte ptr es:[di]
    mov byte ptr es:[di], bl
ega350_check_axis_swap:
    ; After the zero case, even parity selects low bits 11b for the axis-swapped pass.
    test word ptr [bp + PLOT_SYMMETRY], 3
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
    mov word ptr [bp - PLOT_X_PLUS], ax
    mov ax, word ptr [_screen_half_width]
    sub ax, si
    js ega350_return
    mov word ptr [bp - PLOT_X_MINUS], ax
    mov ax, word ptr [_screen_half_height]
    mov si, word ptr [_draw_x]
    cmp ax, si
    jg dv_046c
    inc byte ptr [_symmetry_y_edge]
dv_046c:
    add ax, si
    js ega350_return
    mov word ptr [bp - PLOT_Y_PLUS], ax
    mov ax, word ptr [_screen_half_height]
    sub ax, si
    js ega350_return
    mov word ptr [bp - PLOT_Y_MINUS], ax
    ; Reuse the caller's argument slot: the swapped pass needs reflections only.
    mov word ptr [bp + PLOT_SYMMETRY], 1
    jmp near ptr ega350_plot_quadrants
ega350_return:
    pop di
    pop si
    mov sp, bp
    pop bp
    retf

; Plot draw_x/draw_y in plot_color about the screen center on page_paragraph.
; Stack +6: symmetry flags (bit 0 reflects, low bits 3 also swap axes); no result.
; Writes edge flags and unchained VRAM; sequencer plane = 1 << (physical X & 3).
; 1899:048B, 407 bytes
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
    ; A zero LOOP count wraps; CH is masked again on every delay iteration.
    mov cx, word ptr [_draw_delay_count]
vga400_delay:
    and ch, 01fh
    loop vga400_delay
    mov bl, byte ptr [_plot_color]
    mov dx, VGA_SEQ_INDEX
    xor ax, ax
    ; One word clears the adjacent X and Y edge flags together.
    mov word ptr [_symmetry_edge], ax
    mov ax, word ptr [_screen_half_width]
    mov si, word ptr [_draw_x]
    cmp ax, si
    jg dv_04bf
    inc byte ptr [_symmetry_edge]
dv_04bf:
    add ax, si
    js dv_04f2
    mov word ptr [bp - PLOT_X_PLUS], ax
    mov ax, word ptr [_screen_half_width]
    sub ax, si
    js dv_04f2
    mov word ptr [bp - PLOT_X_MINUS], ax
    mov ax, word ptr [_screen_half_height]
    mov si, word ptr [_draw_y]
    cmp ax, si
    jg dv_04df
    inc byte ptr [_symmetry_y_edge]
dv_04df:
    add ax, si
    js dv_04f2
    mov word ptr [bp - PLOT_Y_PLUS], ax
    mov ax, word ptr [_screen_half_height]
    sub ax, si
    js dv_04f2
    mov word ptr [bp - PLOT_Y_MINUS], ax
    jmp short dv_04f5
dv_04f2:
    jmp near ptr dv_05c4
dv_04f5:
    cmp word ptr [_symmetry_edge], 0
    jne dv_0528
    mov cl, byte ptr [bp - PLOT_X_PLUS]
    and cl, 3
    ; AH = 1 << (X&3); the two INCs below select SC map mask in AL.
    mov ax, 0100h
    shl ax, cl
    inc ax
    inc ax
    out dx, ax
    mov ax, word ptr [bp - PLOT_X_PLUS]
    sar ax, 1
    sar ax, 1
    mov di, ax
    mov ax, word ptr [bp - PLOT_Y_PLUS]
    mov cl, 4
    shl ax, cl
    add di, ax
    shl ax, 1
    shl ax, 1
    add di, ax
    ; DI = 80*Y + (X>>2); the retained read does not alter the stored color.
    mov al, byte ptr es:[di]
    mov byte ptr es:[di], bl
dv_0528:
    test word ptr [bp + PLOT_SYMMETRY], 1
    jne dv_0532
    jmp near ptr dv_061c
dv_0532:
    cmp byte ptr [_symmetry_edge], 0
    jne dv_0565
    mov cl, byte ptr [bp - PLOT_X_PLUS]
    and cl, 3
    mov ax, 0100h
    shl ax, cl
    inc ax
    inc ax
    out dx, ax
    mov ax, word ptr [bp - PLOT_X_PLUS]
    sar ax, 1
    sar ax, 1
    mov di, ax
    mov ax, word ptr [bp - PLOT_Y_MINUS]
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
    mov cl, byte ptr [bp - PLOT_X_MINUS]
    and cl, 3
    mov ax, 0100h
    shl ax, cl
    inc ax
    inc ax
    out dx, ax
    mov ax, word ptr [bp - PLOT_X_MINUS]
    sar ax, 1
    sar ax, 1
    mov di, ax
    mov ax, word ptr [bp - PLOT_Y_PLUS]
    mov cl, 4
    shl ax, cl
    add di, ax
    shl ax, 1
    shl ax, 1
    add di, ax
    mov al, byte ptr es:[di]
    mov byte ptr es:[di], bl
dv_0598:
    mov cl, byte ptr [bp - PLOT_X_MINUS]
    and cl, 3
    mov ax, 0100h
    shl ax, cl
    inc ax
    inc ax
    out dx, ax
    mov ax, word ptr [bp - PLOT_X_MINUS]
    sar ax, 1
    sar ax, 1
    mov di, ax
    mov ax, word ptr [bp - PLOT_Y_MINUS]
    mov cl, 4
    shl ax, cl
    add di, ax
    shl ax, 1
    shl ax, 1
    add di, ax
    mov al, byte ptr es:[di]
    mov byte ptr es:[di], bl
dv_05c4:
    ; After the zero case, even parity selects low bits 11b for the axis-swapped pass.
    test word ptr [bp + PLOT_SYMMETRY], 3
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
    mov word ptr [bp - PLOT_X_PLUS], ax
    mov ax, word ptr [_screen_half_width]
    sub ax, si
    js dv_061c
    mov word ptr [bp - PLOT_X_MINUS], ax
    mov ax, word ptr [_screen_half_height]
    mov si, word ptr [_draw_x]
    cmp ax, si
    jg dv_0603
    inc byte ptr [_symmetry_y_edge]
dv_0603:
    add ax, si
    js dv_061c
    mov word ptr [bp - PLOT_Y_PLUS], ax
    mov ax, word ptr [_screen_half_height]
    sub ax, si
    js dv_061c
    mov word ptr [bp - PLOT_Y_MINUS], ax
    ; Reuse the caller's argument slot: the swapped pass needs reflections only.
    mov word ptr [bp + PLOT_SYMMETRY], 1
    jmp near ptr dv_04f5
dv_061c:
    pop di
    pop si
    mov sp, bp
    pop bp
    retf

; Plot draw_x/draw_y in plot_color about the screen center on page_paragraph.
; Stack +6: symmetry flags (bit 0 reflects, low bits 3 also swap axes); no result.
; Writes symmetry edge flags and chained VRAM at 320*Y+X; no port writes.
; 1899:0622, 315 bytes
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
    ; A zero LOOP count wraps; CH is masked again on every delay iteration.
    mov cx, word ptr [_draw_delay_count]
vga200_delay:
    and ch, 01fh
    loop vga200_delay
    mov bl, byte ptr [_plot_color]
    xor ax, ax
    ; One word clears the adjacent X and Y edge flags together.
    mov word ptr [_symmetry_edge], ax
    mov ax, word ptr [_screen_half_width]
    mov si, word ptr [_draw_x]
    cmp ax, si
    jg vga200_sum_x
    inc byte ptr [_symmetry_edge]
vga200_sum_x:
    add ax, si
    js vga200_outside
    mov word ptr [bp - PLOT_X_PLUS], ax
    mov ax, word ptr [_screen_half_width]
    sub ax, si
    js vga200_outside
    mov word ptr [bp - PLOT_X_MINUS], ax
    mov ax, word ptr [_screen_half_height]
    mov si, word ptr [_draw_y]
    cmp ax, si
    jg vga200_sum_y
    inc byte ptr [_symmetry_y_edge]
vga200_sum_y:
    add ax, si
    js vga200_outside
    mov word ptr [bp - PLOT_Y_PLUS], ax
    mov ax, word ptr [_screen_half_height]
    sub ax, si
    js vga200_outside
    mov word ptr [bp - PLOT_Y_MINUS], ax
    jmp short vga200_plot_quadrants
vga200_outside:
    jmp short vga200_check_axis_swap
vga200_plot_quadrants:
    cmp word ptr [_symmetry_edge], 0
    jne vga200_check_symmetry
    mov ax, word ptr [bp - PLOT_Y_PLUS]
    ; For physical rows 0..199, the byte swap supplies 256*Y; shifts add 64*Y.
    xchg ah, al
    mov di, word ptr [bp - PLOT_X_PLUS]
    add di, ax
    shr ax, 1
    shr ax, 1
    add di, ax
    mov al, byte ptr es:[di]
    mov byte ptr es:[di], bl
vga200_check_symmetry:
    test word ptr [bp + PLOT_SYMMETRY], 1
    jne vga200_plot_reflections
    jmp near ptr vga200_return
vga200_plot_reflections:
    cmp byte ptr [_symmetry_edge], 0
    jne vga200_left_bottom
    mov ax, word ptr [bp - PLOT_Y_MINUS]
    xchg ah, al
    mov di, word ptr [bp - PLOT_X_PLUS]
    add di, ax
    shr ax, 1
    shr ax, 1
    add di, ax
    mov al, byte ptr es:[di]
    mov byte ptr es:[di], bl
vga200_left_bottom:
    cmp byte ptr [_symmetry_y_edge], 0
    jne vga200_left_top
    mov ax, word ptr [bp - PLOT_Y_PLUS]
    xchg ah, al
    mov di, word ptr [bp - PLOT_X_MINUS]
    add di, ax
    shr ax, 1
    shr ax, 1
    add di, ax
    mov al, byte ptr es:[di]
    mov byte ptr es:[di], bl
vga200_left_top:
    mov ax, word ptr [bp - PLOT_Y_MINUS]
    xchg ah, al
    mov di, word ptr [bp - PLOT_X_MINUS]
    add di, ax
    shr ax, 1
    shr ax, 1
    add di, ax
    mov al, byte ptr es:[di]
    mov byte ptr es:[di], bl
vga200_check_axis_swap:
    ; After the zero case, even parity selects low bits 11b for the axis-swapped pass.
    test word ptr [bp + PLOT_SYMMETRY], 3
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
    mov word ptr [bp - PLOT_X_PLUS], ax
    mov ax, word ptr [_screen_half_width]
    sub ax, si
    js vga200_return
    mov word ptr [bp - PLOT_X_MINUS], ax
    mov ax, word ptr [_screen_half_height]
    mov si, word ptr [_draw_x]
    cmp ax, si
    jg vga200_swapped_y
    inc byte ptr [_symmetry_y_edge]
vga200_swapped_y:
    add ax, si
    js vga200_return
    mov word ptr [bp - PLOT_Y_PLUS], ax
    mov ax, word ptr [_screen_half_height]
    sub ax, si
    js vga200_return
    mov word ptr [bp - PLOT_Y_MINUS], ax
    ; Reuse the caller's argument slot: the swapped pass needs reflections only.
    mov word ptr [bp + PLOT_SYMMETRY], 1
    jmp near ptr vga200_plot_quadrants
vga200_return:
    pop di
    pop si
    mov sp, bp
    pop bp
    retf

; Read a centered pixel on page_paragraph; translated Y is halved, even or odd.
; Stack +6: X, +8: Y; AX = color 0..15 for valid coordinates (AH always zero).
; Negative translated coordinates leave AL incidental; selects GC read maps 0..3.
; 1899:075D, 152 bytes
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
    mov si, word ptr [bp + PIXEL_X_ARGUMENT]
    add ax, si
    js dv_07ed
    mov word ptr [bp - PIXEL_X], ax
    mov ax, word ptr [_screen_half_height]
    mov si, word ptr [bp + PIXEL_Y_ARGUMENT]
    add ax, si
    js dv_07ed
    sar ax, 1
    mov word ptr [bp - PIXEL_Y], ax
    mov ax, word ptr [bp - PIXEL_X]
    sar ax, 1
    sar ax, 1
    sar ax, 1
    mov di, ax
    mov ax, word ptr [bp - PIXEL_Y]
    mov cl, 4
    shl ax, cl
    add di, ax
    shl ax, 1
    shl ax, 1
    add di, ax
    mov cx, word ptr [bp - PIXEL_X]
    and cl, 7
    xor cl, 7
    mov bl, 1
    shl bl, cl
    ; BL masks one pixel; decreasing CL rotates each plane into its color bit.
    mov dx, VGA_GC_INDEX
    ; GC read-map index 04h with AH=0 selects plane 0; data writes select 1..3.
    mov ax, GC_READ_MAP
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

; Read a centered pixel on page_paragraph by combining the four EGA plane bits.
; Stack +6: X, +8: Y; AX = color 0..15 for valid coordinates (AH always zero).
; Negative translated coordinates leave AL incidental; selects GC read maps 0..3.
; 1899:07F5, 150 bytes
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
    mov si, word ptr [bp + PIXEL_X_ARGUMENT]
    add ax, si
    js dv_0883
    mov word ptr [bp - PIXEL_X], ax
    mov ax, word ptr [_screen_half_height]
    mov si, word ptr [bp + PIXEL_Y_ARGUMENT]
    add ax, si
    js dv_0883
    mov word ptr [bp - PIXEL_Y], ax
    mov ax, word ptr [bp - PIXEL_X]
    sar ax, 1
    sar ax, 1
    sar ax, 1
    mov di, ax
    mov ax, word ptr [bp - PIXEL_Y]
    mov cl, 4
    shl ax, cl
    add di, ax
    shl ax, 1
    shl ax, 1
    add di, ax
    mov cx, word ptr [bp - PIXEL_X]
    and cl, 7
    xor cl, 7
    mov bl, 1
    shl bl, cl
    ; BL masks one pixel; decreasing CL rotates each plane into its color bit.
    mov dx, VGA_GC_INDEX
    ; GC read-map index 04h with AH=0 selects plane 0; data writes select 1..3.
    mov ax, GC_READ_MAP
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

; Read a centered unchained VGA pixel on page_paragraph, without bounds checks.
; Stack +6: X, +8: Y; AX = color 0..255; DGROUP state is read only.
; Selects sequencer map mask and GC read map using physical X & 3.
; 1899:088B, 102 bytes
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
    mov si, word ptr [bp + PIXEL_X_ARGUMENT]
    add ax, si
    mov word ptr [bp - PIXEL_X], ax
    mov ax, word ptr [_screen_half_height]
    mov si, word ptr [bp + PIXEL_Y_ARGUMENT]
    add ax, si
    mov word ptr [bp - PIXEL_Y], ax
    mov cl, byte ptr [bp - PIXEL_X]
    and cl, 3
    ; Select the same X&3 plane in the SC write mask and GC read-map register.
    mov ax, 0100h
    shl ax, cl
    inc ax
    inc ax
    mov dx, VGA_SEQ_INDEX
    out dx, ax
    mov dx, VGA_GC_INDEX
    mov ah, byte ptr [bp - PIXEL_X]
    and ah, 3
    mov al, GC_READ_MAP
    out dx, ax
    mov ax, word ptr [bp - PIXEL_X]
    sar ax, 1
    sar ax, 1
    mov di, ax
    mov ax, word ptr [bp - PIXEL_Y]
    mov cl, 4
    shl ax, cl
    add di, ax
    shl ax, 1
    shl ax, 1
    add di, ax
    ; DI = 80*translated Y + (translated X>>2), within the selected page.
    mov al, byte ptr es:[di]
    xor ah, ah
    pop di
    pop si
    mov sp, bp
    pop bp
    retf

; Read a centered chained VGA pixel on page_paragraph at 320*Y+X.
; Stack +6: X, +8: Y; AX = color 0..255; no bounds checks or port writes.
; Uses screen_half_width/height and page_paragraph without changing DGROUP.
; 1899:08F1, 65 bytes
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
    mov si, word ptr [bp + PIXEL_X_ARGUMENT]
    add ax, si
    mov word ptr [bp - PIXEL_X], ax
    mov ax, word ptr [_screen_half_height]
    mov si, word ptr [bp + PIXEL_Y_ARGUMENT]
    add ax, si
    mov word ptr [bp - PIXEL_Y], ax
    mov ax, word ptr [bp - PIXEL_Y]
    xchg ah, al
    mov di, word ptr [bp - PIXEL_X]
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

; Save BIOS mode, cursor shape and the BIOS data area text last-row byte.
; No arguments/result; INT 10h fills saved_bios_mode/cursor_shape/text_last_row.
; 1899:0932, 39 bytes
public _save_video_state
_save_video_state label far
save_video_state label far
    push di
    mov ax, BIOS_GET_MODE
    int BIOS_VIDEO_INT
    and ax, 07fh
    mov word ptr [_saved_bios_mode], ax
    ; BH still identifies the active text page returned by the mode query.
    mov ax, BIOS_GET_CURSOR
    int BIOS_VIDEO_INT
    mov word ptr [_saved_cursor_shape], cx
    mov ax, BIOS_DATA_SEGMENT
    mov es, ax
    mov di, BDA_TEXT_LAST_ROW
    mov al, byte ptr es:[di]
    xor ah, ah
    mov word ptr [_saved_text_last_row], ax
    pop di
    retf

; Detect the active display through INT 10h/1A00h, with an EGA 1200h fallback.
; No arguments/result; writes video_adapter and ega_memory_code in DGROUP.
; Reports unsupported displays via DOS; force_ega/force_vga override (VGA wins).
; 1899:0959, 332 bytes
public _detect_video_adapter
_detect_video_adapter label far
detect_video_adapter label far
    push si
    mov word ptr [_video_adapter], 0
    mov ax, BIOS_DISPLAY_COMBINATION
    xor bx, bx
    int BIOS_VIDEO_INT
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
    ; BIOS active-display codes index this table; unknown codes fall back to EGA probing.
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
    ; Unchanged BL=10h means the EGA information service is unavailable.
    mov ax, BIOS_EGA_INFO
    mov bx, 010h
    int BIOS_VIDEO_INT
    mov cx, bx
    cmp bl, 010h
    je dv_0a27
    and bx, 3
    mov word ptr [_ega_memory_code], bx
    mov bx, BIOS_DATA_SEGMENT
    mov es, bx
    mov bx, BDA_EGA_INFO
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
    ; Internal 4/8 select color EGA/VGA; code 7 is promoted to 8 without a warning.
adapter_action_table:
    dw offset dv_0a89, offset dv_0a45, offset dv_0a6b, offset dv_0a63, offset dv_0a89
dv_0a45:
    mov dx, 03bcah
    mov ah, DOS_PRINT_STRING
    int DOS_INT
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
    mov ah, DOS_PRINT_STRING
    int DOS_INT
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

; Select EGA mode 10h and configure graphics-controller write mode 2.
; Stack +6: clear policy (0 preserves VRAM, 2 skips BIOS mode set); no result.
; Writes video_mode_cookie; INT 10h may change mode; always writes GC mode.
; 1899:0AA5, 40 bytes
public _ega_set_mode
_ega_set_mode label far
ega_set_mode label far
    push bp
    mov bp, sp
    mov word ptr [_video_mode_cookie], 041h
    cmp word ptr [bp + MODE_CLEAR], 2
    je dv_0ac4
    cmp word ptr [bp + MODE_CLEAR], 0
    je dv_0abf
    mov ax, EGA_BIOS_MODE
    jmp short dv_0ac2
dv_0abf:
    mov ax, EGA_BIOS_NO_CLEAR
dv_0ac2:
    int BIOS_VIDEO_INT
dv_0ac4:
    ; GC index 05h, value 02h: expand the low four CPU color bits into the planes.
    mov ax, GC_WRITE_MODE_2
    mov dx, VGA_GC_INDEX
    out dx, ax
    pop bp
    retf

; Fill VRAM from offset zero after the next vertical retrace, enabling all planes.
; Stack +6: word pattern, +8: word count, +Ah: page paragraph; no result.
; Writes A000h+page and sequencer map mask; enables interrupts, clears DF.
; 1899:0ACD, 47 bytes
public _clear_vram_words
_clear_vram_words label far
clear_vram_words label far
    push bp
    mov bp, sp
    push di
    sti
    ; Wait for the current retrace to end, then for the next one to start.
    mov dx, VGA_STATUS
dv_0ad5:
    in al, dx
    test al, STATUS_VERTICAL_RETRACE
    jne dv_0ad5
dv_0ada:
    in al, dx
    test al, STATUS_VERTICAL_RETRACE
    je dv_0ada
    mov dx, VGA_SEQ_INDEX
    ; SC index 02h, value 0Fh: enable all four planes.
    mov ax, SEQ_ALL_PLANES
    out dx, ax
    mov ax, VGA_MEMORY
    add ax, word ptr [bp + CLEAR_PAGE_PARAGRAPH]
    mov es, ax
    sub di, di
    cld
    mov ax, word ptr [bp + CLEAR_PATTERN]
    mov cx, word ptr [bp + CLEAR_WORD_COUNT]
    ; CX counts words; DI wraps at 64 KiB, including the FA00h-count VGA clear.
    rep stosw
    pop di
    pop bp
    retf

; Set BIOS 13h/93h, then program unchained VGA 320x400 byte addressing.
; Stack +6: nonzero clears to background_fill_word; zero preserves VRAM; no result.
; Writes video_mode_cookie, SC/GC/CRTC and DAC; uploads palette_buffer entries 0..8.
; 1899:0AFC, 162 bytes
public _vga_set_mode_320x400
_vga_set_mode_320x400 label far
vga_set_mode_320x400 label far
    push bp
    mov bp, sp
    mov word ptr [_video_mode_cookie], 043h
    cmp word ptr [bp + MODE_CLEAR], 0
    je dv_0b10
    mov ax, VGA_BIOS_MODE
    jmp short dv_0b13
dv_0b10:
    mov ax, VGA_BIOS_NO_CLEAR
dv_0b13:
    int BIOS_VIDEO_INT
    ; Initialize 256 DAC entries from A000:0000 before applying palette_buffer entries.
    mov dx, VGA_MEMORY
    mov es, dx
    xor dx, dx
    xor bx, bx
    mov cx, 0100h
    mov ax, BIOS_DAC_BLOCK
    int BIOS_VIDEO_INT
    ; SC memory mode: clear chain-4 bit 3, set odd/even-disable bit 2.
    mov al, SEQ_MEMORY_MODE
    mov dx, VGA_SEQ_INDEX
    cli
    out dx, al
    inc dx
    in al, dx
    and al, 0f7h
    or al, 4
    out dx, al
    sti
    ; GC mode: clear read-mode bit 3 and host odd/even bit 4; keep 256-color shifts.
    mov al, GC_MODE
    mov dx, VGA_GC_INDEX
    cli
    out dx, al
    inc dx
    in al, dx
    and al, 0e7h
    out dx, al
    sti
    ; GC miscellaneous: clear chain odd/even bit 1.
    mov al, GC_MISC
    mov dx, VGA_GC_INDEX
    cli
    out dx, al
    inc dx
    in al, dx
    and al, 0fdh
    out dx, al
    sti
    cmp word ptr [bp + MODE_CLEAR], 0
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
    ; Keep high vertical bits 5/6; clear row-repeat count and double-scan bit 7.
    mov al, CRTC_MAX_SCAN
    mov dx, VGA_CRTC_INDEX
    cli
    out dx, al
    inc dx
    in al, dx
    and al, 060h
    out dx, al
    sti
    ; Clear CRTC doubleword-address bit 6.
    mov al, CRTC_UNDERLINE
    mov dx, VGA_CRTC_INDEX
    cli
    out dx, al
    inc dx
    in al, dx
    and al, 0bfh
    out dx, al
    sti
    ; Set CRTC byte-address bit 6 for the unchained scanout.
    mov al, CRTC_MODE
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
    mov ax, BIOS_DAC_BLOCK
    int BIOS_VIDEO_INT
    pop bp
    retf

; Set chained BIOS VGA 320x200; optionally fill VRAM with background_fill_word.
; Stack +6: nonzero clears, zero selects no-clear mode 93h; no result.
; Writes video_mode_cookie; BIOS changes mode, clearing also sets all SC planes.
; 1899:0B9E, 47 bytes
public _vga_set_mode_320x200
_vga_set_mode_320x200 label far
vga_set_mode_320x200 label far
    push bp
    mov bp, sp
    mov word ptr [_video_mode_cookie], 044h
    cmp word ptr [bp + MODE_CLEAR], 0
    je dv_0bc6
    mov ax, VGA_BIOS_MODE
    int BIOS_VIDEO_INT
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
    mov ax, VGA_BIOS_NO_CLEAR
    int BIOS_VIDEO_INT
dv_0bcb:
    pop bp
    retf

; Reset GC bit mask/write mode, then optionally restore the saved BIOS display.
; Stack +6: nonzero restores saved mode, text rows/font and cursor; no result.
; Reads saved_* DGROUP words; writes GC ports and, on restore, BIOS text-row byte.
; 1899:0BCD, 70 bytes
public _restore_video_state
_restore_video_state label far
restore_video_state label far
    push bp
    mov bp, sp
    push di
    cli
    ; GC index 08h gets FFh (all pixel bits); index 05h gets 00h (write mode 0).
    mov ax, GC_ALL_PIXEL_BITS
    mov dx, VGA_GC_INDEX
    out dx, ax
    mov ax, GC_MODE
    out dx, ax
    sti
    cmp word ptr [bp + MODE_RESTORE], 0
    je dv_0c10
    mov ax, word ptr [_saved_bios_mode]
    xor ah, ah
    int BIOS_VIDEO_INT
    cmp word ptr [_saved_text_last_row], 018h
    jbe dv_0c07
    mov ax, BIOS_DATA_SEGMENT
    mov es, ax
    mov di, BDA_TEXT_LAST_ROW
    mov ax, word ptr [_saved_text_last_row]
    mov byte ptr es:[di], al
    ; Restore the tall text layout using the ROM 8x8 font.
    mov ax, BIOS_LOAD_8X8_FONT
    xor bx, bx
    int BIOS_VIDEO_INT
dv_0c07:
    mov cx, word ptr [_saved_cursor_shape]
    mov ax, BIOS_SET_CURSOR
    int BIOS_VIDEO_INT
dv_0c10:
    pop di
    pop bp
    retf

; Upload 16 EGA colors plus border through BIOS INT 10h/1002h.
; Stack +6: far writable palette table; no result; no DGROUP writes.
; Temporarily substitutes background/base green/base blue/border, then restores table.
; 1899:0C13, 105 bytes
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
    mov al, byte ptr es:[bx + EGA_BORDER_ENTRY]
    mov word ptr [bp + PAL_OLD_BORDER], ax
    mov al, byte ptr [_ega_border_color]
    mov byte ptr es:[bx + EGA_BORDER_ENTRY], al
    mov dx, bx
    mov ax, BIOS_EGA_PALETTE
    int BIOS_VIDEO_INT
    mov al, byte ptr [bp + PAL_OLD_BORDER]
    mov byte ptr es:[bx + EGA_BORDER_ENTRY], al
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

; Upload EGA palette registers 15..0 and border at the next vertical retrace.
; Stack +6: far writable palette table; no result; writes attribute-controller port.
; Substitutes the four reserved DGROUP colors, restores table; exits with IF set.
; 1899:0C7C, 152 bytes
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
    mov al, byte ptr es:[si + EGA_BORDER_ENTRY]
    mov word ptr [bp + PAL_OLD_BORDER], ax
    mov al, byte ptr [_ega_border_color]
    mov byte ptr es:[si + EGA_BORDER_ENTRY], al
    sti
    ; Status reads also reset the attribute-controller index/data flip-flop.
    mov dx, VGA_STATUS
ega_palette_wait_display:
    in al, dx
    test al, STATUS_VERTICAL_RETRACE
    jne ega_palette_wait_display
ega_palette_wait_retrace:
    in al, dx
    test al, STATUS_VERTICAL_RETRACE
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
    ; Attribute index 11h with bit 5 set restores display output while setting border.
    mov al, ATTR_BORDER_DISPLAY_ON
    cli
    out dx, al
    mov al, byte ptr es:[si + EGA_BORDER_ENTRY]
    out dx, al
    sti
    mov al, byte ptr [bp + PAL_OLD_BORDER]
    mov byte ptr es:[si + EGA_BORDER_ENTRY], al
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

; Upload VGA DAC entries 3..255 through BIOS INT 10h/1012h.
; Stack +6: far RGB byte triples; no result; no DGROUP writes.
; 1899:0D14, 19 bytes
public _vga_upload_palette_bios
_vga_upload_palette_bios label far
vga_upload_palette_bios label far
    push bp
    mov bp, sp
    les dx, dword ptr [bp + PAL_ARGUMENT]
    mov bx, FIRST_CYCLING_COLOR
    mov cx, 0fdh
    mov ax, BIOS_DAC_BLOCK
    int BIOS_VIDEO_INT
    pop bp
    retf

; Upload DAC entries 3..128, then 129..255 with two BIOS INT 10h/1012h calls.
; Stack +6: far RGB triples; second source starts only 126 bytes later, as coded.
; No result or DGROUP writes; the source step is bytes, not 126 RGB triples.
; 1899:0D27, 33 bytes
public _vga_upload_palette_bios_split
_vga_upload_palette_bios_split label far
vga_upload_palette_bios_split label far
    push bp
    mov bp, sp
    les dx, dword ptr [bp + PAL_ARGUMENT]
    mov bx, FIRST_CYCLING_COLOR
    mov cx, 07eh
    mov ax, BIOS_DAC_BLOCK
    int BIOS_VIDEO_INT
    add dx, 07eh
    mov bx, 081h
    mov cx, 07fh
    mov ax, BIOS_DAC_BLOCK
    int BIOS_VIDEO_INT
    pop bp
    retf

; Upload DAC entries 3..255 at the next retrace using 186+ REP OUTSB.
; Stack +6: far RGB triples (759 bytes); no result; writes DAC index/data ports.
; Restores DS; no DGROUP writes; exits with IF set and DF clear.
; 1899:0D48, 44 bytes
public _vga_upload_palette_rep
_vga_upload_palette_rep label far
vga_upload_palette_rep label far
    enter 0, 0
    push si
    push ds
    lds si, dword ptr [bp + PAL_ARGUMENT]
    sti
    mov dx, VGA_STATUS
dv_0d55:
    in al, dx
    test al, STATUS_VERTICAL_RETRACE
    jne dv_0d55
dv_0d5a:
    in al, dx
    test al, STATUS_VERTICAL_RETRACE
    je dv_0d5a
    mov ax, FIRST_CYCLING_COLOR
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

; Upload DAC entries 3..128 and 129..255 on separate retraces with REP OUTSB.
; Stack +6: far RGB triples, consumed as 378 then 381 bytes; no result.
; Writes DAC ports, restores DS; no DGROUP writes; exits with IF set and DF clear.
; 1899:0D74, 75 bytes
public _vga_upload_palette_rep_split
_vga_upload_palette_rep_split label far
vga_upload_palette_rep_split label far
    enter 0, 0
    push si
    push ds
    lds si, dword ptr [bp + PAL_ARGUMENT]
    sti
    mov dx, VGA_STATUS
dv_0d81:
    in al, dx
    test al, STATUS_VERTICAL_RETRACE
    jne dv_0d81
dv_0d86:
    in al, dx
    test al, STATUS_VERTICAL_RETRACE
    je dv_0d86
    mov ax, FIRST_CYCLING_COLOR
    mov dx, VGA_DAC_WRITE
    out dx, al
    mov dx, VGA_DAC_DATA
    mov cx, 017ah
    cld
    rep outsb
    sti
    sti
    mov dx, VGA_STATUS
    ; The second DAC window waits for its own retrace; the source continues after 378 bytes.
dv_0da0:
    in al, dx
    test al, STATUS_VERTICAL_RETRACE
    jne dv_0da0
dv_0da5:
    in al, dx
    test al, STATUS_VERTICAL_RETRACE
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

; Upload DAC entries 3..255 at the next retrace with an 8086 byte OUT loop.
; Stack +6: far RGB triples (759 bytes); no result; writes DAC index/data ports.
; Restores DS; no DGROUP writes; exits with IF set and DF clear.
; 1899:0DBF, 47 bytes
public _vga_upload_palette_8086
_vga_upload_palette_8086 label far
vga_upload_palette_8086 label far
    push bp
    mov bp, sp
    push si
    push ds
    lds si, dword ptr [bp + PAL_ARGUMENT]
    sti
    mov dx, VGA_STATUS
dv_0dcb:
    in al, dx
    test al, STATUS_VERTICAL_RETRACE
    jne dv_0dcb
dv_0dd0:
    in al, dx
    test al, STATUS_VERTICAL_RETRACE
    je dv_0dd0
    mov ax, FIRST_CYCLING_COLOR
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

; Upload DAC entries 3..128 and 129..255 on separate retraces with byte OUT loops.
; Stack +6: far RGB triples, consumed as 378 then 381 bytes; no result.
; Writes DAC ports, restores DS; no DGROUP writes; exits with IF set and DF clear.
; 1899:0DEE, 82 bytes
public _vga_upload_palette_8086_split
_vga_upload_palette_8086_split label far
vga_upload_palette_8086_split label far
    push bp
    mov bp, sp
    push si
    push ds
    lds si, dword ptr [bp + PAL_ARGUMENT]
    sti
    mov dx, VGA_STATUS
dv_0dfa:
    in al, dx
    test al, STATUS_VERTICAL_RETRACE
    jne dv_0dfa
dv_0dff:
    in al, dx
    test al, STATUS_VERTICAL_RETRACE
    je dv_0dff
    mov ax, FIRST_CYCLING_COLOR
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
    ; The second DAC window waits for its own retrace; the source continues after 378 bytes.
dv_0e1d:
    in al, dx
    test al, STATUS_VERTICAL_RETRACE
    jne dv_0e1d
dv_0e22:
    in al, dx
    test al, STATUS_VERTICAL_RETRACE
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

; Set the EGA split-screen line at the next retrace through CRTC 18h/07h/09h.
; Stack +6: physical line; no result; no DGROUP writes; exits with IF set.
; Line bit 8 goes to overflow bit 4; other overflow bits become 0Fh, max scan = 0.
; 1899:0E40, 51 bytes
public _ega_set_line_compare
_ega_set_line_compare label far
ega_set_line_compare label far
    push bp
    mov bp, sp
    sti
    mov dx, VGA_STATUS
dv_0e47:
    in al, dx
    test al, STATUS_VERTICAL_RETRACE
    jne dv_0e47
dv_0e4c:
    in al, dx
    test al, STATUS_VERTICAL_RETRACE
    je dv_0e4c
    mov ax, word ptr [bp + SPLIT_LINE]
    mov bh, ah
    and bh, 1
    mov cl, 4
    shl bh, cl
    mov ah, al
    mov al, CRTC_LINE_COMPARE
    mov dx, VGA_CRTC_INDEX
    out dx, ax
    ; Overflow bits 0..3 are fixed; BH contributes line bit 8 as bit 4.
    mov ah, 0fh
    or ah, bh
    mov al, CRTC_OVERFLOW
    out dx, ax
    mov ah, 0
    mov al, CRTC_MAX_SCAN
    out dx, ax
    pop bp
    retf

; Set the VGA split-screen line at the next retrace through CRTC 18h/07h/09h.
; Stack +6: physical line; no result; no DGROUP writes; exits with IF set.
; Replaces line bits 8/9 in overflow bit 4/max-scan bit 6, preserving other bits.
; 1899:0E73, 76 bytes
public _vga_set_line_compare
_vga_set_line_compare label far
vga_set_line_compare label far
    push bp
    mov bp, sp
    sti
    mov dx, VGA_STATUS
dv_0e7a:
    in al, dx
    test al, STATUS_VERTICAL_RETRACE
    jne dv_0e7a
dv_0e7f:
    in al, dx
    test al, STATUS_VERTICAL_RETRACE
    je dv_0e7f
    mov ax, word ptr [bp + SPLIT_LINE]
    mov bh, ah
    mov bl, bh
    ; Arrange line bits 8/9 as BL bit 4 and BH bit 6 for the two overflow registers.
    and bx, 0201h
    mov cl, 4
    shl bx, cl
    shl bh, 1
    mov ah, al
    mov al, CRTC_LINE_COMPARE
    mov dx, VGA_CRTC_INDEX
    out dx, ax
    mov al, CRTC_OVERFLOW
    out dx, al
    inc dx
    in al, dx
    dec dx
    mov ah, al
    ; Replace only overflow bit 4 (line bit 8).
    and ah, 0efh
    or ah, bl
    mov al, CRTC_OVERFLOW
    out dx, ax
    mov al, CRTC_MAX_SCAN
    out dx, al
    inc dx
    in al, dx
    dec dx
    mov ah, al
    ; Replace only max-scan bit 6 (line bit 9).
    and ah, 0bfh
    or ah, bh
    mov al, CRTC_MAX_SCAN
    out dx, ax
    pop bp
    retf

; Set CRTC display start to 80*Y + floor(X/8) at the next vertical retrace.
; Stack +6: X, +8: physical Y; no result or DGROUP writes; exits with IF set.
; Writes CRTC start high/low; the computed X remainder is not sent to a port.
; 1899:0EBF, 60 bytes
public _set_display_start
_set_display_start label far
set_display_start label far
    push bp
    mov bp, sp
    push si
    mov ax, word ptr [bp + DISPLAY_X]
    mov bx, word ptr [bp + DISPLAY_Y]
    mov cx, 8
    div cl
    ; Keep X modulo 8 in CL, although this entry never writes a PEL-pan register.
    mov cl, ah
    xor ah, ah
    xchg bx, ax
    mov si, PLANAR_ROW_BYTES
    mul si
    add bx, ax
    sti
    mov dx, VGA_STATUS
dv_0ede:
    in al, dx
    test al, STATUS_VERTICAL_RETRACE
    jne dv_0ede
dv_0ee3:
    in al, dx
    test al, STATUS_VERTICAL_RETRACE
    je dv_0ee3
    mov dx, VGA_CRTC_INDEX
    mov ah, bh
    ; CRTC 0Ch/0Dh receive the display-start high/low bytes without interruption.
    mov al, CRTC_START_HIGH
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

; Park up to two hard disks via INT 13h; fall back to geometry-table seeks.
; No arguments/result; CMOS 12h chooses disk count; IVT 41h/46h supply cylinders.
; Writes disk_count/first_disk_cylinders/second_disk_cylinders; prints DOS messages.
; 1899:0EFB, 232 bytes
public _park_disk_heads
_park_disk_heads label far
park_disk_heads label far
    push si
    mov al, CMOS_DISK_TYPES
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
    ; INT 41h vector points to the first fixed-disk parameter table.
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
    ; The original first INT 13h request is 1800h; carry clear skips the seek fallbacks.
    mov ax, 01800h
    mov dl, 080h
    int BIOS_DISK_INT
    jae dv_0f83
    mov ah, BIOS_DISK_PARAMETERS
    mov dx, 080h
    int BIOS_DISK_INT
    jb dv_0f66
    mov dx, 080h
    ; Increment only cylinder low byte CH; the reported high bits in CL stay unchanged.
    inc ch
    and cl, 0c0h
    mov ax, BIOS_DISK_SEEK
    int BIOS_DISK_INT
    jb dv_0f66
    jmp short dv_0f83
dv_0f66:
    ; If BIOS parameters/seek fail, pack the saved table cylinder into CH:CL[7:6].
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
    mov ax, BIOS_DISK_SEEK
    int BIOS_DISK_INT
dv_0f83:
    mov dx, 03c28h
    mov ah, DOS_PRINT_STRING
    int DOS_INT
    cmp byte ptr [_disk_count], 2
    je dv_0f93
    jmp short dv_0fda
dv_0f93:
    mov ax, 01800h
    mov dl, 081h
    int BIOS_DISK_INT
    jae dv_0fd3
    mov ah, BIOS_DISK_PARAMETERS
    mov dx, 081h
    int BIOS_DISK_INT
    jb dv_0fb6
    mov dx, 081h
    inc ch
    and cl, 0c0h
    mov ax, BIOS_DISK_SEEK
    int BIOS_DISK_INT
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
    mov ax, BIOS_DISK_SEEK
    int BIOS_DISK_INT
dv_0fd3:
    mov dx, 03c36h
    mov ah, DOS_PRINT_STRING
    int DOS_INT
dv_0fda:
    mov dx, 03c3dh
    mov ah, DOS_PRINT_STRING
    int DOS_INT
dv_0fe1:
    pop si
    retf

; Read a CMOS byte with NMI temporarily masked; near helper, AL index -> AL byte.
; DX becomes 70h; preserves AH and incoming FLAGS; no DGROUP access.
; Writes CMOS index port 70h (then selects 0Dh with NMI enabled); reads port 71h.
; 1899:0FE3, 18 bytes
public _read_cmos_byte
_read_cmos_byte label near
read_cmos_byte label near
    pushf
    ; Index bit 7 masks NMI during the CMOS read; selecting 0Dh below re-enables NMI.
    or al, 080h
    mov dx, CMOS_INDEX
    cli
    out dx, al
    in al, CMOS_DATA
    push ax
    mov al, CMOS_STATUS_D
    out dx, al
    sti
    pop ax
    popf
    ret

; Distinguish 8086/88 from later CPUs through FLAGS high bits and a shift by 33.
; No arguments; AX = 0 for old shift behavior, 1 otherwise; CX is scratch.
; Preserves incoming FLAGS; no port or DGROUP access.
; 1899:0FF5, 37 bytes
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

; Recursively fill a plasma rectangle with averaged center and noisy edge colors.
; Stack +6/+8/+Ah/+Ch: x0,y0,x1,y1; +Eh/+10h/+12h/+14h: TL,TR,BR,BL colors.
; No result; updates RNG/pen/color, calls plot/read and palette/input hooks; SI/DI scratch.
; 1899:101A, 803 bytes
public _subdivide_plasma
_subdivide_plasma label far
subdivide_plasma label far
    push bp
    mov bp, sp
    sub sp, 010h
    ; Recursion stays responsive to palette cycling and pending input.
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
    cmp byte ptr [_current_graphics_key], KEY_CLEAR_SCREEN
    jne dv_104e
    mov sp, bp
    pop bp
    retf
dv_104e:
    mov ax, word ptr [bp + PLASMA_X1]
    mov si, ax
    mov bx, word ptr [bp + PLASMA_Y1]
    mov di, bx
    mov cx, word ptr [bp + PLASMA_X0]
    mov dx, word ptr [bp + PLASMA_Y0]
    ; Stop only when both rectangle spans are less than two.
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
    ; Round both coordinate midpoints upward with SHR/ADC.
    mov si, ax
    add si, cx
    shr si, 1
    adc si, 0
    mov word ptr [bp - PLASMA_MID_X], si
    mov di, bx
    add di, dx
    shr di, 1
    adc di, 0
    mov word ptr [bp - PLASMA_MID_Y], di
    push word ptr [bp - PLASMA_MID_Y]
    push word ptr [bp - PLASMA_MID_X]
    call dword ptr [_read_pixel_callback]
    add sp, 4
    ; Either background byte marks an unassigned sample; preserve colors already drawn.
    mov dx, word ptr [_background_fill_word]
    cmp al, dl
    je dv_10a1
    cmp al, dh
    jne dv_10d5
dv_10a1:
    ; Center color is the four-corner average, without random displacement.
    mov dx, word ptr [bp + PLASMA_TOP_LEFT]
    add dx, word ptr [bp + PLASMA_TOP_RIGHT]
    add dx, word ptr [bp + PLASMA_BOTTOM_RIGHT]
    add dx, word ptr [bp + PLASMA_BOTTOM_LEFT]
    shr dx, 1
    shr dx, 1
    mov word ptr [_plot_color], dx
    mov ax, word ptr [bp - PLASMA_MID_X]
    mov word ptr [_draw_x], ax
    mov ax, word ptr [bp - PLASMA_MID_Y]
    mov word ptr [_draw_y], ax
    push ax
    push bp
    mov bp, sp
    ; The temporary frame exposes the pushed argument word at [BP+2] for symmetry=1.
    mov word ptr [bp + 2], 1
    pop bp
    call dword ptr [_plot_callback]
    add sp, 2
    mov ax, word ptr [_plot_color]
dv_10d5:
    mov word ptr [bp - PLASMA_CENTER_COLOR], ax
    ; Noise scale is twice the sum of the rectangle width and height.
    mov bx, word ptr [bp + PLASMA_X1]
    sub bx, word ptr [bp + PLASMA_X0]
    add bx, word ptr [bp + PLASMA_Y1]
    sub bx, word ptr [bp + PLASMA_Y0]
    shl bx, 1
    mov word ptr [bp - PLASMA_NOISE_SCALE], bx
    push word ptr [bp + PLASMA_Y0]
    push word ptr [bp - PLASMA_MID_X]
    call dword ptr [_read_pixel_callback]
    add sp, 4
    mov dx, word ptr [_background_fill_word]
    cmp al, dl
    je dv_1102
    cmp al, dh
    jne dv_115b
    ; Top edge: perturb the endpoint sum with the signed high half of noise_scale * RNG.
dv_1102:
    mov cx, word ptr [_plasma_random]
    ; Advance the 16-bit RNG by addition then rotate right three bits.
    add cx, 09248h
    ror cx, 1
    ror cx, 1
    ror cx, 1
    mov word ptr [_plasma_random], cx
    mov ax, word ptr [bp - PLASMA_NOISE_SCALE]
    imul cx
    add dx, word ptr [bp + PLASMA_TOP_LEFT]
    add dx, word ptr [bp + PLASMA_TOP_RIGHT]
    sar dx, 1
    ; Clamp noisy edge colors to the cycling range 3..plasma_color_limit.
    cmp dx, 4
    jl dv_112e
    cmp dx, word ptr [_plasma_color_limit]
    jg dv_1133
    jmp short dv_1137
dv_112e:
    mov dx, FIRST_CYCLING_COLOR
    jmp short dv_1137
dv_1133:
    mov dx, word ptr [_plasma_color_limit]
dv_1137:
    mov word ptr [_plot_color], dx
    mov ax, word ptr [bp - PLASMA_MID_X]
    mov word ptr [_draw_x], ax
    mov ax, word ptr [bp + PLASMA_Y0]
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
    mov word ptr [bp - PLASMA_TOP_COLOR], ax
    push word ptr [bp - PLASMA_MID_Y]
    push word ptr [bp + PLASMA_X0]
    call dword ptr [_read_pixel_callback]
    add sp, 4
    mov dx, word ptr [_background_fill_word]
    cmp al, dl
    je dv_1177
    cmp al, dh
    jne dv_11d0
    ; Left edge uses the top-left and bottom-left corner colors.
dv_1177:
    mov cx, word ptr [_plasma_random]
    add cx, 09248h
    ror cx, 1
    ror cx, 1
    ror cx, 1
    mov word ptr [_plasma_random], cx
    mov ax, word ptr [bp - PLASMA_NOISE_SCALE]
    imul cx
    add dx, word ptr [bp + PLASMA_TOP_LEFT]
    add dx, word ptr [bp + PLASMA_BOTTOM_LEFT]
    sar dx, 1
    cmp dx, 4
    jl dv_11a3
    cmp dx, word ptr [_plasma_color_limit]
    jg dv_11a8
    jmp short dv_11ac
dv_11a3:
    mov dx, FIRST_CYCLING_COLOR
    jmp short dv_11ac
dv_11a8:
    mov dx, word ptr [_plasma_color_limit]
dv_11ac:
    mov word ptr [_plot_color], dx
    mov ax, word ptr [bp + PLASMA_X0]
    mov word ptr [_draw_x], ax
    mov ax, word ptr [bp - PLASMA_MID_Y]
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
    mov word ptr [bp - PLASMA_LEFT_COLOR], ax
    push word ptr [bp - PLASMA_MID_Y]
    push word ptr [bp + PLASMA_X1]
    call dword ptr [_read_pixel_callback]
    add sp, 4
    mov dx, word ptr [_background_fill_word]
    cmp al, dl
    je dv_11ec
    cmp al, dh
    jne dv_1245
    ; Right edge uses the top-right and bottom-right corner colors.
dv_11ec:
    mov cx, word ptr [_plasma_random]
    add cx, 09248h
    ror cx, 1
    ror cx, 1
    ror cx, 1
    mov word ptr [_plasma_random], cx
    mov ax, word ptr [bp - PLASMA_NOISE_SCALE]
    imul cx
    add dx, word ptr [bp + PLASMA_TOP_RIGHT]
    add dx, word ptr [bp + PLASMA_BOTTOM_RIGHT]
    sar dx, 1
    cmp dx, 4
    jl dv_1218
    cmp dx, word ptr [_plasma_color_limit]
    jg dv_121d
    jmp short dv_1221
dv_1218:
    mov dx, FIRST_CYCLING_COLOR
    jmp short dv_1221
dv_121d:
    mov dx, word ptr [_plasma_color_limit]
dv_1221:
    mov word ptr [_plot_color], dx
    mov ax, word ptr [bp + PLASMA_X1]
    mov word ptr [_draw_x], ax
    mov ax, word ptr [bp - PLASMA_MID_Y]
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
    mov word ptr [bp - PLASMA_RIGHT_COLOR], ax
    push word ptr [bp + PLASMA_Y1]
    push word ptr [bp - PLASMA_MID_X]
    call dword ptr [_read_pixel_callback]
    add sp, 4
    mov dx, word ptr [_background_fill_word]
    cmp al, dl
    je dv_1261
    cmp al, dh
    jne dv_12ba
    ; Bottom edge uses the bottom-right and bottom-left corner colors.
dv_1261:
    mov cx, word ptr [_plasma_random]
    add cx, 09248h
    ror cx, 1
    ror cx, 1
    ror cx, 1
    mov word ptr [_plasma_random], cx
    mov ax, word ptr [bp - PLASMA_NOISE_SCALE]
    imul cx
    add dx, word ptr [bp + PLASMA_BOTTOM_LEFT]
    add dx, word ptr [bp + PLASMA_BOTTOM_RIGHT]
    sar dx, 1
    cmp dx, 4
    jl dv_128d
    cmp dx, word ptr [_plasma_color_limit]
    jg dv_1292
    jmp short dv_1296
dv_128d:
    mov dx, FIRST_CYCLING_COLOR
    jmp short dv_1296
dv_1292:
    mov dx, word ptr [_plasma_color_limit]
dv_1296:
    mov word ptr [_plot_color], dx
    mov ax, word ptr [bp - PLASMA_MID_X]
    mov word ptr [_draw_x], ax
    mov ax, word ptr [bp + PLASMA_Y1]
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
    mov word ptr [bp - PLASMA_BOTTOM_COLOR], ax
    ; Subdivide top-left, top-right, bottom-right, bottom-left, reusing all edge samples.
    push word ptr [bp - PLASMA_LEFT_COLOR]
    push word ptr [bp - PLASMA_CENTER_COLOR]
    push word ptr [bp - PLASMA_TOP_COLOR]
    push word ptr [bp + PLASMA_TOP_LEFT]
    push word ptr [bp - PLASMA_MID_Y]
    push word ptr [bp - PLASMA_MID_X]
    push word ptr [bp + PLASMA_Y0]
    push word ptr [bp + PLASMA_X0]
    push cs
    call near ptr subdivide_plasma
    add sp, 010h
    push word ptr [bp - PLASMA_CENTER_COLOR]
    push word ptr [bp - PLASMA_RIGHT_COLOR]
    push word ptr [bp + PLASMA_TOP_RIGHT]
    push word ptr [bp - PLASMA_TOP_COLOR]
    push word ptr [bp - PLASMA_MID_Y]
    push word ptr [bp + PLASMA_X1]
    push word ptr [bp + PLASMA_Y0]
    push word ptr [bp - PLASMA_MID_X]
    push cs
    call near ptr subdivide_plasma
    add sp, 010h
    push word ptr [bp - PLASMA_BOTTOM_COLOR]
    push word ptr [bp + PLASMA_BOTTOM_RIGHT]
    push word ptr [bp - PLASMA_RIGHT_COLOR]
    push word ptr [bp - PLASMA_CENTER_COLOR]
    push word ptr [bp + PLASMA_Y1]
    push word ptr [bp + PLASMA_X1]
    push word ptr [bp - PLASMA_MID_Y]
    push word ptr [bp - PLASMA_MID_X]
    push cs
    call near ptr subdivide_plasma
    add sp, 010h
    push word ptr [bp + PLASMA_BOTTOM_LEFT]
    push word ptr [bp - PLASMA_BOTTOM_COLOR]
    push word ptr [bp - PLASMA_CENTER_COLOR]
    push word ptr [bp - PLASMA_LEFT_COLOR]
    push word ptr [bp + PLASMA_Y1]
    push word ptr [bp - PLASMA_MID_X]
    push word ptr [bp - PLASMA_MID_Y]
    push word ptr [bp + PLASMA_X0]
    push cs
    call near ptr subdivide_plasma
    add sp, 010h
    mov sp, bp
    pop bp
    retf

; Seed four random corner colors, then subdivide the positive-coordinate quarter.
; No arguments/result; uses screen_half_width/height and color_count, DOS time seeds RNG.
; Writes plasma bounds/color limit/random and pen/color; plotting uses plot_callback.
; 1899:133D, 394 bytes
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
    ; DOS time returns seconds/hundredths in DX, the initial plasma RNG state.
    mov ah, DOS_GET_TIME
    int DOS_INT
    mov word ptr [_plasma_random], dx
    mov cx, word ptr [_plasma_random]
    add cx, 09248h
    ror cx, 1
    ror cx, 1
    ror cx, 1
    mov word ptr [_plasma_random], cx
    ; Each corner takes the RNG high byte, clamped to 3..plasma_color_limit.
    mov dl, ch
    xor dh, dh
    cmp dx, 4
    jl dv_1385
    cmp dx, word ptr [_plasma_color_limit]
    jg dv_138a
    jmp short dv_138e
dv_1385:
    mov dx, FIRST_CYCLING_COLOR
    jmp short dv_138e
dv_138a:
    mov dx, word ptr [_plasma_color_limit]
dv_138e:
    mov word ptr [bp - SEED_TOP_LEFT], dx
    mov word ptr [_plot_color], dx
    mov ax, 0
    mov word ptr [_draw_x], ax
    mov ax, 0
    mov word ptr [_draw_y], ax
    push ax
    push bp
    mov bp, sp
    ; The pushed word becomes symmetry=1 for each corner's reflected plot.
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
    mov dx, FIRST_CYCLING_COLOR
    jmp short dv_13de
dv_13da:
    mov dx, word ptr [_plasma_color_limit]
dv_13de:
    mov word ptr [bp - SEED_TOP_RIGHT], dx
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
    mov dx, FIRST_CYCLING_COLOR
    jmp short dv_142e
dv_142a:
    mov dx, word ptr [_plasma_color_limit]
dv_142e:
    mov word ptr [bp - SEED_BOTTOM_RIGHT], dx
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
    mov dx, FIRST_CYCLING_COLOR
    jmp short dv_147e
dv_147a:
    mov dx, word ptr [_plasma_color_limit]
dv_147e:
    mov word ptr [bp - SEED_BOTTOM_LEFT], dx
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
    ; Corner order passed to subdivision is TL, TR, BR, BL after x0,y0,x1,y1.
    push word ptr [bp - SEED_BOTTOM_LEFT]
    push word ptr [bp - SEED_BOTTOM_RIGHT]
    push word ptr [bp - SEED_TOP_RIGHT]
    push word ptr [bp - SEED_TOP_LEFT]
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

; INT 1Ch: advance palette ticks and decrement nonzero drawing/show countdowns.
; Interrupt frame -> IRET; saves DS/AX/BX/ES; chains saved_timer_vector.
; Every fifth tick checks BIOS key queue and saver flag; updates input/exit/header state.
; 1899:14C7, 133 bytes
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
    cmp word ptr [_show_duration_ticks], 0
    je timer_poll_divider
    dec word ptr [_show_duration_ticks]
timer_poll_divider:
    dec word ptr [_input_poll_divider]
    cmp word ptr [_input_poll_divider], 0
    jne timer_chain
    mov word ptr [_input_poll_divider], 5
    mov ax, BIOS_DATA_SEGMENT
    mov es, ax
    cli
    ; A different BIOS queue head/tail signals input even when INT 09h is not installed.
    mov ax, word ptr es:[BDA_KEY_HEAD]
    cmp ax, word ptr es:[BDA_KEY_TAIL]
    sti
    je timer_check_screensaver
    cmp word ptr [_input_pending], 0
    jne timer_check_screensaver
    inc word ptr [_input_pending]
timer_check_screensaver:
    cmp word ptr [_mouse_callback_installed], 0
    je timer_chain
    les bx, dword ptr [_current_image_header]
    ; Only the saver image header (signature 777h) consumes the driver event bit.
    cmp word ptr es:[bx + 012h], 0777h
    jne timer_chain
    ; Bit 12 requests exit; clear it after setting the input/exit counters.
    test word ptr es:[bx + 0ch], 01000h
    je timer_chain
    inc word ptr [_exit_requested]
    inc word ptr [_input_pending]
    and word ptr es:[bx + 0ch], 0efffh
timer_chain:
    ; The saved FLAGS word lets the chained interrupt handler return with IRET.
    call dword ptr [_saved_timer_vector]
    pop es
    pop bx
    pop ax
    pop ds
    iret

; INT 09h: increment input_pending, then chain saved_keyboard_vector.
; Interrupt frame -> IRET; establishes DGROUP, saves DS/AX; no direct port access.
; 1899:154C, 19 bytes
public _keyboard_pending_isr
_keyboard_pending_isr label far
keyboard_pending_isr label far
    push ds
    push ax
    pushf
    mov ax, DGROUP
    mov ds, ax
    inc word ptr [_input_pending]
    ; The saved FLAGS word supplies the chained handler's interrupt-return frame.
    call dword ptr [_saved_keyboard_vector]
    pop ax
    pop ds
    iret

; Mouse event callback: increment input_pending and mouse_abort_pending in DGROUP.
; Ignores event registers; far return, no result; preserves DS/BP and other registers.
; No direct port access; BP temporarily establishes DS without disturbing AX.
; 1899:155F, 18 bytes
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

; Return this module's actual CS in AX; no arguments, port or DGROUP access.
; Caller 11C1:0394 proves the entry at 1899:1571.
; 1899:1571, 3 bytes
public _dvdt_seg
_dvdt_seg label far
dvdt_seg label far
    push cs
    pop ax
    retf
DVDT6 ends

LZH_WINDOW equ 4096
LZH_LOOKAHEAD equ 60
LZH_SYMBOLS equ 314
LZH_TREE equ (2*LZH_SYMBOLS-1)
MENU_BACKUP_ROWS equ 3
MENU_BACKUP_COLUMNS equ 78

; Shared far workspace. Logical ownership is menus followed by DHUF.
; Word alignment between the two packed layouts explains the byte at 0259h.
FAR_DATA segment para public 'FAR_DATA'
public _error_screen_backup, _directory_tail, _directory_find_data
public _editable_images, _directory_path, _editable_image_count
_error_screen_backup dw (MENU_BACKUP_ROWS*MENU_BACKUP_COLUMNS) dup (?)
_directory_tail dd ?
_directory_find_data db 43 dup (?)
_editable_images dd ?
_directory_path db 80 dup (?)
_editable_image_count dw ?
align 2
public _huff_son, _huff_parent, _huff_frequency, _prefix_length
public _right_child, _parent_node, _left_child, _match_length, _match_position
public _dictionary, _progress_step, _progress_next, _compressed_size, _expanded_size
public _lzh_output, _lzh_input
_huff_son dw LZH_TREE dup (?)
_huff_parent dw (LZH_TREE+LZH_SYMBOLS) dup (?)
_huff_frequency dw (LZH_TREE+1) dup (?)
_prefix_length db (LZH_WINDOW+1) dup (?)
_right_child dw (LZH_WINDOW*2+1) dup (?)
_parent_node dw (LZH_WINDOW+1) dup (?)
_left_child dw (LZH_WINDOW+1) dup (?)
_match_length dw ?
_match_position dw ?
_dictionary db (LZH_WINDOW+LZH_LOOKAHEAD-1) dup (?)
_progress_step dd ?
_progress_next dd ?
_compressed_size dd ?
_expanded_size dd ?
_lzh_output dd ?
_lzh_input dd ?
FAR_DATA ends

end
