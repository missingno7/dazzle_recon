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
and Auto values. Display, palette, delay, sound, presentation and renderer
choices live in `%APPDATA%\DAZZLE\dazzle.ini`. Bad individual INI values use
defaults; malformed or oversized `.DZL` text is rejected before DOSI reads it.
The saver never writes settings. Presentation and renderer choices are stored
and passed through `dz_display`; their rendering effects belong to the platform
video implementation.

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
