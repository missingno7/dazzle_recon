# Runnable build smoke test

Build with `python -B -I tools/build.py`. It runs the strict link, records its receipt at `build/link.json`, and generates a CRC tail for the linked MZ, producing the unpacked `build/DAZZLE.EXE`.

The four DOSBox-X runs used `SDL_VIDEODRIVER=dummy`, `SDL_AUDIODRIVER=dummy`, SVGA/S3, and fixed 200,000 cycles. Each run used a config under `build/workers/dz-smoke/`; the original and rebuilt EXEs ran with `-a -m -b -w` and with no options. A small TSR requested each screenshot after 60 BIOS timer ticks. The options runs returned normally. The no-options runs kept animating, so DOSBox-X was killed after capture; the original run reached the 25-second limit.

```powershell
$env:SDL_VIDEODRIVER='dummy'; $env:SDL_AUDIODRIVER='dummy'
& 'C:\tools\dosbox-x-2026.08.31-vsbuild-win64\bin\x64\Release\dosbox-x.exe' -noconsole -conf build\workers\dz-smoke\original-options\run.conf
```

Repeat with `rebuilt-options`, `original-default`, and `rebuilt-default`; run `z-extract\run.conf` for `/Z`.

The link completed `EXACT` (201,542 load-image bytes and 2,431 relocations). The four screenshots show DAZZLE graphics without a CRC error or early exit code 100. The screens are visually comparable at 640 x 400; the animation frame differs between runs. `/Z` extracted 84,927 bytes identical to `assets/DAZZLE.DOC` (SHA-256 `53b47422b6977433d54ee92751a2b86a3d253a9b953d458eddedc1320dd1cce5`).

`build/DAZZLE.EXE`: 290,545 bytes, SHA-256 `9b2b4af308ed2ee4bd15499cc3408c955676470e4ff68fa73069651c0a66298b` (212,294-byte MZ + 78,251-byte tail).

Screenshots:

- Options: [original](../build/smoke/original-options.png), [rebuilt](../build/smoke/rebuilt-options.png)
- No options: [original](../build/smoke/original-default.png), [rebuilt](../build/smoke/rebuilt-default.png)
