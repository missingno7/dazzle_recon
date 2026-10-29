# DAZPCX native port notes

- `save_pcx`: DOS `creatnew`, `_fmode`, and `fcloseall` are replaced by a non-overwriting stdio open, binary streams, and closing the file owned by the save operation. The filename sequence and PCX bytes are preserved.
- `save_pcx`: DOS speaker calls use `dz_sound`, `dz_delay_ms`, and `dz_nosound`; display pixels come from `dz_video.vram` using `page_paragraph` for the second page.
- `pcx_encode_row`: the original compares one byte beyond the row before checking its bound. Native passes the following scanline byte when the source memory has one, and zero for DOS-allocated row buffers whose overread value was indeterminate.
- `save_pcx`: file and allocation failures report through stderr and set `exit_requested` instead of terminating the native process through the DOS `exit` path.
- `dazpcx_seg`: native code has no DOS segment number, so the diagnostic getter returns zero.
