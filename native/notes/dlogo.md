# DLOGO native port notes

- `load_logo_image`: the native reader loads the complete declared resource into one allocation. The supplied 43,422-byte resource fits; the DOS two-buffer chunking is unnecessary for this target.
- `load_logo_image`: the original file comes from `dz_options.original_exe`; its MZ page count and last-page byte count derive the tail metadata position using DAZZLE's `(e_cp - 1) * 512 + e_cblp + 1` formula.
- `draw_closing_graphic` and `configure_logo_dac`: BIOS mode and palette interrupts are represented by `dz_video_set_mode`, `ega_write_planar_row`, and direct native DAC/attribute palette updates.
- `draw_closing_graphic`: VGA and EGA decoded rows go to the indexed native framebuffer. `restore_logo_video_state` returns to the prior modeled video mode.
- `test_dlogo_vga`: a generated one-plane tail fixture covers the VGA decode and restore path; the real four-plane image is compared at runtime to `tools/logo.py`.
- `prepare_logo_image_resources`: malformed resources report an error and set `exit_requested` instead of terminating the host process through the DOS fatal path.
- `dlogo_seg`: native code has no DOS segment number, so the diagnostic getter returns zero.
