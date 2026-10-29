# DHUF native port notes

- `extract_manual`: the packed executable's MZ header derives the appended-tail base; metadata at that base supplies the length and compressed-stream extent. The native decoder reads exactly that extent from `dz_options.original_exe`.
- `extract_manual`: the DOS self-check and registration prompt are not invoked; they are separate DOS startup services and are not needed to read the retained original resource. The native copy reports input/output errors to stderr.
- `extract_manual`: DOS `exit(501)` becomes `exit_requested = 1`, allowing the native event loop to end and the test harness to compare `DAZZLE.DOC` byte-for-byte.
- `lzh_reconstruct_huffman`: the DOS 16-bit pointer reinterpretation between signed and unsigned word arrays is replaced by indexed 16-bit moves, preserving the stored word patterns without native aliasing undefined behavior.
- `dhuf_seg`: native code has no DOS segment number, so the diagnostic getter returns zero.
