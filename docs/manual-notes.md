# DAZZLE 5.0 manual notes for reverse engineers

## Evidence and counts

- Source: assets/DAZZLE.DOC (Revision 5.0, 22 May 1992); companion context from assets/GO.BAT and assets/FILE_ID.DIZ.
- **24 command-line options:** 23 shown in the main/additional option menus plus the separately documented -* autoload-suppression marker. Bare * is an inputfile sentinel, not another switch.
- **0 individually named image algorithms** are printed. The runtime Image Size menu is said to show NAME, KEY, and SIZE, but the manual includes none of those rows.
- **62 possible image-selection key combinations:** Alt+top-row 0–9 (10), Alt+A–Z (26), Ctrl+A–Z (26). This is a key-space count; actual algorithm count is not given.
- **3 transition types described:** Pan/Scroll, split screens, and fade to black. The manual says R5.0 expanded the fade set but does not give its complete names or count.
- **17 common key/action entries** and **66 Semi-Auto-specific key combinations** (7 grouped rows). These counts exclude menu navigation and runtime-configured fade-key assignments.
- GO.BAT runs “dazzle -a -m -b -w”. The manual does not explain precedence/interaction for simultaneous Alternate and Mad. FILE_ID.DIZ identifies DAZZLE50.ZIP v5.0 as “The Ultimate Graphics Image Generator.”

## Modes and keys

- Startup enters **Auto**. **Alt+F9** toggles to/from **Semi-Auto**. Each switch clears the display and reinitializes the palette, with different tone cues.
- Auto randomly mixes image algorithms using configured minimum/maximum image counts and pause time. Fade keys queue the next fade while drawing continues. **P** is accepted only before Auto enters Fade-Out; **H** holds at the Auto pause until a fade is selected.
- Semi-Auto supports **F8** continuous drawing, **F9** random selection, **F10** draw/resume, **Alt+F10** reinitialize, **Alt+0–9/A–Z** select from the first image list, and **Ctrl+A–Z** select from the second. Fade keys execute immediately; **H** is ignored; **P** selects another enabled fade because Pan/Scroll is unsupported.
- Common controls: **ESC** terminates; **ENTER** terminates outside menus; **TAB** freezes and **SPACE/N** release it; TAB also toggles color cycling/static while frozen. **F5** saves PCX; **+/-** adjust cycling rate; **PgUp/PgDn** adjust VGA cycling step; **Shift+F10** randomizes palette; **Ctrl+F10** changes background; **N** interrupts work and starts screen clear. Mouse click usually acts like ENTER, but aborts in a text menu.
- Fade keys are configurable. The runtime Fade Enable Control screen has KEY/FADE/ENABLE fields, but the manual does not list the actual bindings or fade names.
- Help: **F1** toggles, with HOME/END/PgDn/PgUp/arrows for navigation. Configuration: **F2** toggles with similar navigation. File Access: **S** or **L** then ENTER; **F3** directory; **F4** executes save/load.

## Drawing, transitions, and timing

- EGA default: 640x350x16, mapped as if 640x700. VGA default: enhanced 320x400x256, at most 256 KB video memory. **/T** uses BIOS aspect/resolution (EGA 640x350, VGA 320x200); VGA /T loses dual-page pan/scroll.
- **/E** and **/V** force EGA/VGA and can continue after controller/monitor rejection while still reporting errors. Exact error wording is not printed. **/C** uses BIOS routines; **/2** splits VGA palette updates; **/X** supports XT CPUs; **/4** limits EGA to four colors; **/H** hides EGA border; **/+** ignores TARGA+64 in the special TARGA build.
- **/D[=xxxx]** delays drawing (1–8191 if numeric; bare /D uses an unspecified built-in default). TARGA+64 reverses the numeric direction. **/S** draws one panel; **/S=xxxx** accepts 1–3600 seconds, then exits at a fade boundary.
- Image Size values control relative draw duration/complexity and scale to display size/resolution. **$I**, **$X**, and **$F** configure minimum images, maximum images, and Auto fade/pause delay. At least one image size must be nonzero. Alternate and Mad increase overlays.
- Pan/Scroll can build on a hidden page, then move the image into view. Split screens are named but not explained. Fade to black clears at each fade end and is not a configurable listed fade.
- **F5** writes DAZ_xxxx.PCX (0000–9999 sequence in current path), matching current video mode. Active Pan/Scroll is temporarily full-screen during writing, then resumes.

## Configuration, support features, and integrity

- Options accept - or /, are case-insensitive and concatenable. Optional inputfile path/name may appear anywhere. Without one, DAZZLE attempts MTDAZZLE.DZL in the current path; * or -* suppresses this load.
- Config file: one Key ID and bracketed value per line. Shift tokens are ~ (Alt), ^ (Ctrl), and grave accent (Shift); $I/$X/$F designate Auto customization fields. Semicolon starts an inline comment; first non-comment line should be # plus an unsigned version. Numeric values are unsigned integers; fade values are Y/N.
- Suggested extension .DZL; .BAT, .COM, and .EXE are illegal. Wildcards * and ? open the directory list. File status strings: NOT FOUND, EXIST, READ ONLY.
- Scroll Lock: off = active work, blinking = automatic pause countdown, solid = user pause/menu. Num Lock: Pan/Scroll computation/display. Caps Lock: VGA palette computation. /W and /B suppress LED changes.
- **/Q** suppresses programmed tones, but generally not warning beeps. Manual documents state/mode and palette tones.
- Shareware evaluation is limited to no more than **10 days**, then registration is due. No runtime nag, reminder cadence, or registration prompt is documented; this does not prove the executable lacks one. Normal startup identifies DAZZLE/revision; **/W** suppresses startup messages, **/B** suppresses closing credits, and **/W** keeps the closing graphic but omits closing text.
- Integrity check examines the current on-disk DAZZLE.EXE, not the whole system, and is described as not exhaustive. Unauthorized changes may be detected and refused; R5.0 uses a faster CRC. **/I** and **/W** bypass it.

## Exact strings useful for code searches

- “Usage: DAZZLE -options inputfile”; “Press Any Key”; “Additional Options”.
- “NOT FOUND”; “EXIST”; “READ ONLY”; “DAZ_xxxx.PCX”.
- Configuration labels: “Minimum images per display”; “Maximum images per display”; “These numbers will be increased if in Alternate or Mad mode.”
- Credits artwork message: “Our Creator, evidenced by our creativity”.
- No literal adapter rejection or integrity failure text is given.
