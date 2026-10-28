# Borland C/C++ DOS toolchains

Acquired and expanded the WinWorld disk sets on 2026-10-28. Every original .7z is retained under the product media/ directory with the original disk images. Full extracted disk files are in setup-source/; expanded CA and regular ZIP payloads are retained under files/packages/. Curated runnable files are in files/BIN, files/LIB, files/INCLUDE, files/STARTUP, and files/BGI. SHA-256 values for key executables, startup objects, libraries, and original disk images are in ../evidence/toolchains.json.

## Install roots and release banners

| Release | Install root | Compiler banner | TLINK / TLIB | TASM |
|---|---|---|---|---|
| Borland C++ 2.0 | `C:\tools\borland-c++-2.0` | `Borland C++  Version 2.0 Copyright (c) 1991 Borland International` | `Turbo Link  Version 4.0 Copyright (c) 1991 Borland International` / `TLIB 3.01 Copyright (c) 1991 Borland International` | `Turbo Assembler  Version 2.51  Copyright (c) 1988, 1991 Borland International` |
| Borland C++ 3.0 | `C:\tools\borland-c++-3.0` | `Borland C++  Version 3.00 Copyright (c) 1991 Borland International` | `Turbo Link  Version 5.0 Copyright (c) 1991 Borland International` / `TLIB 3.02 Copyright (c) 1991 Borland International` | `Turbo Assembler  Version 3.0  Copyright (c) 1988, 1991 Borland International` |
| Borland C++ 3.1 | `C:\tools\borland-c++-3.1` | `Borland C++  Version 3.1 Copyright (c) 1992 Borland International` | `Turbo Link  Version 5.1 Copyright (c) 1992 Borland International` / `TLIB 3.02 Copyright (c) 1992 Borland International` | `Turbo Assembler  Version 3.1  Copyright (c) 1988, 1992 Borland International` |
| Turbo C++ 3.0 DOS | `C:\tools\turbo-c++-3.0` | `Turbo C++ Version 3.00 Copyright (c) 1992 Borland International` | `Turbo Link  Version 5.0 Copyright (c) 1992 Borland International` / `TLIB 3.02 Copyright (c) 1991 Borland International` | `not bundled` |

All four published WinWorld checksums matched the downloaded archives. The outer 7z files passed 7-Zip tests, each split .CA archive passed CRC testing with Borland UNZIP 2.00, and the standard ZIP package archives passed integrity checks. The versions, archive URLs, published checksums, actual SHA-1/SHA-256/SHA-512, retained media hashes, file hashes, and FIDB commit are machine-readable in evidence/toolchains.json.

## Smoke tests

MS-DOS Player: C:/tools/nmlgcdos/msdos.exe. From each isolated scratch directory under build/workers/toolchain/smoke/, the compiler compiled both memory models with optimization. Each compiler also compiled HELLO.C; TLINK linked it against the small-model startup object and library; running the resulting EXE printed hello from Borland. All commands returned 0.

| Compiler | Large model | Small model | TLINK link | Run hello |
|---|---|---|---|---|
| BCC.EXE (Borland C++ 2.0) | `BCC.EXE -c -ml -O PROBE.C` pass | `-c -ms -O PROBE.C` pass | pass | `hello from Borland` |
| BCC.EXE (Borland C++ 3.0) | `BCC.EXE -c -ml -O PROBE.C` pass | `-c -ms -O PROBE.C` pass | pass | `hello from Borland` |
| BCC.EXE (Borland C++ 3.1) | `BCC.EXE -c -ml -O PROBE.C` pass | `-c -ms -O PROBE.C` pass | pass | `hello from Borland` |
| TCC.EXE (Turbo C++ 3.0 DOS) | `TCC.EXE -c -ml -O PROBE.C` pass | `-c -ms -O PROBE.C` pass | pass | `hello from Borland` |

The compiler-produced probe objects have OMF THEADR probe.c. Their OMF COMENT markers are:

- Borland C++ 2.0: TC86 Borland Turbo C++ 2.0.
- Borland C++ 3.0: TC86 Borland Turbo C++ 3.00.
- Borland C++ 3.1: TC86 Borland C++ 3.1.
- Turbo C++ 3.0: TC86 Borland Turbo C++ 3.00.

C0*.OBJ startup objects across all four releases contain the runtime string Borland C++ - Copyright 1991 Borland Intl. BC++ 3.0 and Turbo C++ 3.0 libraries also contain the compiler marker TC86 Borland Turbo C++ 3.00. The 3.1 compiler and TLINK banners are dated 1992, while its startup objects retain the 1991 runtime string.

## Invoking from an isolated scratch directory

The runner maps the current host directory as its DOS working directory. Copy the compiler, CPP.EXE, TLINK.EXE, model startup objects/libraries, and headers into that scratch directory (or use the prepared directories below), then run:

```powershell
Set-Location 'D:\Prog\dazzle_recon\build\workers\toolchain\smoke\bc30'
& 'C:\tools\nmlgcdos\msdos.exe' BCC.EXE -c -ml -O PROBE.C
& 'C:\tools\nmlgcdos\msdos.exe' BCC.EXE -c -ms -O PROBE.C
& 'C:\tools\nmlgcdos\msdos.exe' BCC.EXE -c -ms -O -IINCLUDE HELLO.C
& 'C:\tools\nmlgcdos\msdos.exe' TLINK.EXE C0S.OBJ 'HELLO.OBJ,HELLO.EXE,,CS.LIB'
& 'C:\tools\nmlgcdos\msdos.exe' HELLO.EXE
& 'C:\tools\nmlgcdos\msdos.exe' TASM.EXE SOURCE.ASM
```

Use TCC.EXE in place of BCC.EXE for Turbo C++ 3.0. No protected-mode loader or DOSBox-X fallback was needed for these compiler and linker commands under MS-DOS Player. TASM 3.0 and 3.1 are bundled with their matching BC++ releases; TASM 2.51 is bundled with BC++ 2.0. Turbo C++ 3.0 does not include TASM.

## Source and FIDB coverage

WinWorld supplied the Borland C++ 2.0, 3.0, 3.1, and Turbo C++ 3.0 sets. Their product and download pages plus direct archive URLs are in evidence/toolchains.json and the appended catalog section. The files were extracted with 7-Zip and Borland UNZIP; no installer was run natively on Windows.

Cloned https://github.com/moralrecordings/ghidra-fidb-dos-win16 to C:\tools\ghidra-fidb-dos-win16 at commit c59b439489df45aed0b479bbb9c7460f90085482. Its README lists blc200 (Borland C++ 2.0), blc300 (3.0), and blc310 (3.1). These are version-level FIDBs; the README does not specify per-memory-model FIDBs. It does not list Turbo C++ 3.0.

## Gaps

- Turbo C++ 3.0 ships TCC.EXE, not BCC.EXE, and does not include TASM or a startup RTL source package.
- The 3.0 and 3.1 Borland C++ releases include TASM 3.0 and 3.1 respectively. The 3.1 set also includes the CLIBSRC source package (95 files); the 2.0/3.0 releases include C0 startup assembly sources. All extracted packages remain preserved even when not copied into the curated folders.
- No acquisition, archive integrity, compile, link, or hello-world run failed.
