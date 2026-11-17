# Equation guide

An Elementary/EFL calculator written in C, built with Meson. The binary is
`equation`. It uses Elementary widgets inside a themed, client-decorated window.
Three independently compiled Edje themes are included; the calculation engine
has no dependency on the UI or themes.

## Build and run

Requires a C compiler, Meson, Ninja, pkg-config, and EFL/Elementary >= 1.26
including development headers, Eet, Ecore_File, `edje_cc`, and SVG image loading support.
Install GNU gettext tools for translation extraction, catalog compilation, and
localized desktop launcher generation. Gettext uses libc or libintl where available.

```sh
meson setup build --prefix=/usr/local
meson compile -C build
meson test -C build --print-errorlogs
./build/src/fe-efl/equation
```

If EFL is installed under `/usr/local` and pkg-config cannot find it, use
`PKG_CONFIG_PATH=/usr/local/lib/pkgconfig meson setup build --prefix=/usr/local`.

## Install

```sh
meson install -C build
```

The configured prefix must be writable. For an installation in your home directory,
configure with `--prefix="$HOME/.local"`. Installation includes the executable,
application launcher, scalable icon, and all three theme archives. Packaging can use
`meson install -C build --destdir /tmp/equation-package`.

## Use

Choose Scientific or Programmer using the radio buttons at the top. Enter an
expression with the keyboard or buttons and press Enter or `=`. Results in base
10, base 16, and base 2 appear in a large main readout and two conversion cards.
The main readout’s copy icon has a “Copy result” tooltip and briefly confirms
“Copied”; it copies the complete decimal value, including long 64-bit results.
Invalid calculations disable copying. Hex and binary entries support selection
and horizontal scrolling. Clear removes the expression;
Back removes the selection or the character before the cursor. Binary results
omit the `0b` prefix.

In Programmer mode, use **Input base** to choose Decimal (10), Hex (16), or
Binary (2). This sets the base of every unprefixed integer in the expression,
including shift counts. For example, `FF + 1` in Hex gives decimal 256, and
`1010 + 1` in Binary gives decimal 11. Switching the base reinterprets the current
expression without rewriting it. Explicit `0d`, `0x`, and `0b` prefixes override
the selection, so `0xFF + 0b10 + 0d10` works in any input base. Prefixes take
priority even in Hex: write `0x0B` for hexadecimal eleven. Invalid digits and
values exceeding 64 bits show an error. The input base starts at Decimal each
launch and is retained when switching modes during a session. Scientific mode
keeps its decimal and function syntax.

The burger button to the left of the window title opens Appearance, About Equation, and Quit. Choose
Observatory (midnight blue and cyan), Atelier (warm paper and drafting marks),
or Crazy Lab (oxidized metal, bakelite keys, warning stripes, and glowing Nixie
tubes). A marker identifies the selected theme. Theme changes update open windows
immediately.

The window opens at the current theme and calculator mode’s minimum layout size.
Enlarge it to give the controls more room; startup does not restore an expanded size.
About also opens at its content’s minimum size.

Drag the title bar to move the window, double-click it to maximize or restore,
and drag an edge or corner to resize. The menu sits left of the title; minimize, maximize/restore, and close sit on the right.
Title-bar icons are borderless with transparent backgrounds at rest and highlight
on hover; Close highlights red. About also uses client-side
decorations. All themes include soft outer drop shadows and an Elaro-style
header fade. Maximized windows fill the available area and hide the outer shadow.

Theme, calculator mode, Degrees, and Ambient animations are saved whenever changed, in
`$XDG_CONFIG_HOME/equation/equation.eet`, or `~/.config/equation/equation.eet`
when XDG_CONFIG_HOME is unset or relative. Settings use compressed Eet entries
and are replaced atomically. An unreadable config uses defaults; a missing or
incompatible saved theme falls back to Observatory. Save failures are shown in
the menu without interrupting calculations. Crazy Lab displays up to fourteen
characters in individual tubes and uses a full text readout for longer results,
including 64-bit values. The tube bank stays centered and preserves the glass
tubes’ original proportions when resized. Both keypads use aligned five-row grids with distinct
operator keys, illuminated equals, and animated hover/press feedback. Flat material keys use fine inset edges and restrained shading, with a
short accent light that expands on hover. Observatory also breathes a soft cyan
backlight while a button is hovered or highlighted for keyboard focus; it fades
away on exit. Presses have a small tactile movement.
Radio buttons use flat recessed rings and solid selected centers, with an animated
halo on hover; switches use matching low-profile sliders. Each theme keeps its
own palette, and title-bar icons remain borderless.


Observatory has a rotating orbital instrument, Atelier a slow drafting compass,
and Crazy Lab a spinning reactor, breathing indicators, and a warm Nixie readout.
Evaluations trigger a brief ignition and light sweep. Disable Ambient animations
in the menu for a still display; ambient motion also pauses while hidden or minimized.

Scientific mode supports parentheses, `+ - * / %`, powers (`^`), `pi`, `e`,
scientific notation such as `1e3`, and `sin`, `cos`, `tan`, `asin`, `acos`, `atan`,
`sqrt`, `ln`, `log` (base 10), `exp`, `abs`, `floor`, and `ceil`. Functions take parentheses.
Angles default to radians; check Degrees to use degrees. `%` is remainder.
Powers associate right to left; `-2^2` means `-(2^2)`.
Hexadecimal and binary results require an integer with magnitude below 2^64;
negative scientific results use a leading minus sign in each base. Fractional
results remain available in decimal without silent truncation.

Programmer mode uses unsigned 64-bit integers. Decimal numbers have no prefix;
use `0x` for hexadecimal and `0b` for binary (for example `0xff & 0b1010`).
Operations include `+ - * / %`, `&`, `|`, `^` (XOR), `~`, `<<`, and `>>`.
Arithmetic wraps modulo 2^64 and division truncates. Shifts require a count
between 0 and 63. Overflowing literals and invalid expressions are rejected.

## Layout

- `src/engine/equation_engine.c`: expression parsing, validation, calculation,
  numeric semantics, and result formatting. No EFL dependency.
- `src/fe-efl/equation_ui.c`: Elementary widgets and event handling.
- `src/fe-efl/equation_window.c`: shared client decorations, window controls,
  native move/resize requests, and About window framing.
- `src/fe-efl/equation_theme.c`: theme discovery, validation, and result display.
- `src/config/equation_config.c`: XDG Eet preferences and atomic saves.
- `src/fe-efl/equation_main.c`: application lifecycle.
- `tests/equation_engine_test.c`: engine regression tests.
- `tests/equation_config_test.c`: defaults, Eet round trips, and malformed settings.
- `tests/equation_theme_test.c`: every compiled group and frame geometry.
- `tests/equation_ui_test.c`: real widget mouse clicks, live theme changes, About,
  calculations, Quit, and preference restoration at scales 1, 1.5, and 2.
- `data/themes/`: per-theme EDC entry points and vector artwork, with shared
  groups under `common/groups/`. Archives install to `share/equation/themes/`.

See [the theme guide](equation_themes.md) for the theme structure and contracts.
The build uses archives beside its executable; installed binaries use the
configured install prefix, so an installed copy cannot accidentally load stale
archives from a checkout.

Application functions and source/assets use the `equation_` prefix; application
types use `Equation_`. The C entry point and Meson's `meson.build` filenames
follow their required standard names.

## License and credits

Equation code and artwork are licensed under the GNU General Public License,
version 2 only (`GPL-2.0-only`). See [COPYING](../COPYING) for the full license
and [AUTHORS](../AUTHORS) for contributors.

(c) enlightenment development team

## Internationalization

Gettext support follows Elaro: the `equation` domain, UTF-8 catalogs, a Meson
`equation-pot` / `equation-update-po` workflow, and catalogs installed under
`share/locale`. Interface labels, tooltips, accessibility text, and engine
diagnostics are marked for translation, along with the desktop launcher.
English remains the fallback; the initial catalog template is ready for translators.
See [po/README.md](../po/README.md) to add a language.

The system locale selects the language. Expressions and copied numbers retain
the decimal point, and mathematical tokens stay unchanged across languages.

Button themes track pointer hover, keyboard focus, press, and disabled state
separately. Returning focus to the entry preserves the hovered key’s highlight;
dragging off a key cancels activation. Single, double, and triple clicks all
receive press feedback, and disabled buttons cannot activate.

## Source formatting

C sources and headers use Elaro/EFL style: three-space scope indentation,
two-space inset control braces, and function names below return types. EDC
keeps Elaro's compact statements and one-space nesting. Meson uses two spaces,
Python uses four, and SVG artwork is formatted XML.

With clang-format and Python 3 installed:

```sh
meson compile -C build equation-format
meson compile -C build equation-format-check
```

The formatter runs clang-format with `.clang-format`, then applies EFL scope
indentation. Run it through `tools/equation_format.py` or the Meson target to
retain the exact indentation. It also normalizes EDC indentation. Build files
can be formatted with `meson format -c meson.format -i -r meson.build` on recent Meson versions.
`.editorconfig` records the indentation conventions for editors.
