# Windows screensaver milestone

`dazzle.scr` is a Win32 GUI executable that runs the existing DAZZLE engine
with `/W /B` and its Auto loop. `/s` uses a primary-display fullscreen window,
hides the cursor, and covers other displays with black topmost windows. Keys,
mouse buttons, movement over three pixels from the first motion baseline, and
focus loss after activation dismiss it. Saver input is intercepted before the
DOS keyboard and mouse handlers. `/p HWND` wraps a child HWND for SDL, sizes it
to the parent's client area at launch, ignores input, and exits when the parent
is destroyed. `/c` opens the native settings dialog.

Settings reuse DOSI's `.DZL` reader and writer for image sizes, fade enables,
and Auto values. Display, palette, delay, sound, presentation, renderer and
per-effect style choices live in `%APPDATA%\DAZZLE\dazzle.ini`. The decimal
`NativeEffects` value is a 42-bit mask in image-record order (36 Alt, then 6
Ctrl). A missing value in an older INI defaults all records to Native. A `.DZL`
import changes sizes/fades/Auto values and leaves this mask intact; the `.DZL`
format has no style field. Bad individual INI values use defaults; malformed or
oversized `.DZL` text is rejected before DOSI reads it. The settings dialog
writes the files; animation does not save them automatically.

The Images tab shows Native/Faithful for records accepted by
`dz_native_available()` and a dash for other records. The selected row's style
control and the All Native/All Faithful buttons update the mask. Defaults set
all 42 bits. Extended selects Screen and retains each image's saved style;
Style controls remain enabled and the "Faithful only" label is removed. Canvas
selection is Screen for Extended or any enabled image with selected, available
Native; otherwise Classic. Existing INI values remain valid.
The canvas decision uses `dz_native_available()` at runtime rather than a list
of implemented effects.

Build and headless lifecycle checks:

```powershell
python -B -I tools/native.py test
```

The screensaver tests cover the command-line forms, defaults and range checks,
`.DZL` import, save/load round trip, text-free first animation frame, key and
large-motion dismissal, and small jitter that remains active until a later key.
Manual desktop checks on 2026-10-29: `/s` opened fullscreen windows on both
available displays and exited with code 0 after a posted key. A second `/s`
instance exited with code 0 while the first instance remained active. `/p`
created a child in a real Win32 parent, filled its 704x461 client area at 150%
scaling, ignored a posted key, and exited with code 0 after parent destruction.
`/c` opened `DAZZLE 5.0 Settings`; pressing OK exited with code 0 and wrote both
per-user settings files.

The synthetic preview parent did not keep the child at its new size after a
runtime resize; the Control Panel's fixed-size preview behavior was not checked.
The black secondary-display window was observed in window enumeration; its
pixels were not captured for visual inspection. Focus-loss dismissal was not
manually induced. The test-only activity hook is enabled only by
`DAZZLE_SCR_TEST_HOOKS=1` and is not part of normal screensaver launch behavior.

## System-wide activity check (integration, 2026-10-29)

A `/s` saver started by a background process (not by Windows itself) never received keyboard focus, so SDL
delivered neither keys nor (with another window under the cursor) mouse motion; a desktop test with a posted
Shift key and a 72-pixel `SetCursorPos` did not dismiss it. `dz_window_global_activity()` now also polls
`GetLastInputInfo` and `GetCursorPos` at every input poll: the cursor leaving a 3-pixel box around its start,
or new input while the cursor stayed put (key, button, wheel), dismisses. Desktop re-check: posted key exits in
0.10 s, 72-pixel cursor move in 0.06 s, a 2-pixel move keeps the saver running. Disabled under
`DAZZLE_SCR_TEST_HOOKS` so a user moving the mouse cannot disturb the headless test.

## Per-effect style and real-host pass (2026-11-04)

`python -B -I tools/native.py test`: **41/41 passed**, including defaults,
validation, legacy INI files without `NativeEffects`, save/load round trip,
Extended validation, `.DZL` import preserving the mask, and canvas selection
for enabled Native, disabled Native, Faithful-only and Extended settings. The
Classic reference hashes remain unchanged.

Real Win32 host checks used the built `dazzle.scr` with isolated app-data and
both `Renderer=0` (Auto) and `Renderer=2` (Software). `/s` created a 2560x1440
primary SDL window and exited 0 after Escape in both renderer modes. `/p` filled
a 544x341 test-host client, ignored Escape sent to the preview child, and exited
0 after the host window closed in both modes. `/c` opened and closed cleanly.
The Images and Auto and Display tab screenshots are in
`build/workers/s-select/settings-images.png` and
`build/workers/s-select/settings-display.png`; the Images capture has Sine
selected and shows its Native style control. This execution desktop made
`PIL.ImageGrab.grab()` fail with `screen grab failed`, so the same Python helper
captured the native dialog using Win32 `PrintWindow` as a fallback. No defect
was found in `window.c`, `input.c` or `present.c`; those files and the pinned
Classic hashes are unchanged.

Historical (superseded): Extended's live combo selection and style-control disable/re-enable state were
exercised through Win32 control notifications; the dropdown-open state was not
captured. Control Panel-managed preview behavior, resize during `/p`, focus-loss
dismissal and visible secondary-display pixels remain unverified.

## Unclipped Extended follow-up (2026-11-05)

The former Faithful-only/Classic rule is superseded. Existing real-host evidence
above remains historical. The s-pres settings/source checks cover both policies,
style retention, old INI round trips and canvas selection. See native-canvas.md
for the current comparison and validation receipts.

Current follow-up verification: `tools/native.py test` passed 46/46, including
style/canvas/legacy-INI checks. The latest `dazzle.scr` ran `/s` in Original and
Extended on the 2560x1440 display, and `/c` for both settings kept Style and both
All buttons enabled; the combo reads "Extended". The real GPU presenter was
captured at a fixed virtual deadline using test-only input/capture hooks. Both
modes exited normally. Current captures/receipts are under
`build/workers/s-pres/host/` and `host-current.log`. No user INI was changed.
Native comparisons and remaining integer-path/moire appearance choices are in
[native-canvas.md](../../docs/native-canvas.md#native-vs-classic-comparison).
