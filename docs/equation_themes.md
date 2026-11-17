# Equation themes

Each theme is an independent installed Edje archive, following Eclair's split
between a theme entry point, artwork, shared groups, and Meson compilation.
Artwork uses native SVG with a shared raster shadow; all controls remain Elementary widgets.
Ambient motion and interaction transitions run in native Edje programs. Each
theme packages its instrument, key states, field background, and copy/check icons.
Key artwork uses shallow material shading and fine inset edges, with no raised
base or outer key shadow. A native Edje accent line expands on hover.
`equation_radio.svg` / `equation_radio_on.svg` provide flat inset radio rings;
`equation_toggle.svg` / `equation_toggle_on.svg` provide low-profile switch states. `equation_radio_hover.svg` supplies the
animated hover halo, independently of selection.
Their Elementary groups preserve native radio/check behavior, focus highlighting,
and disabled feedback. Window controls retain transparent, borderless idle states.

| Theme | Direction | Display |
| --- | --- | --- |
| Observatory | Midnight instrument panel, cyan starlight, subtle constellation | Dark graph-paper instrument readout |
| Atelier | Warm paper, ivory keys, rust accents, drafting marks | Quiet technical worksheet |
| Crazy Lab | Oxidized green metal, brass screws, bakelite keys, warning stripes | Fourteen glass Nixie tubes, ghost cathodes, warm orange glow |

## Source layout

```
data/themes/
  observatory/
    equation_observatory.edc
    images/equation_images.edc
    images/equation_*.svg
    meson.build
  atelier/...
  crazy_lab/...
  common/groups/
    equation_frame.edc
    equation_buttons.edc
    equation_choices.edc
    equation_fields.edc
    equation_popup.edc
    equation_display.edc
    equation_results.edc
    equation_motion_parts.edc
    equation_motion_programs.edc
```

The entry points define palettes, typography, names, and artwork, then include
shared groups. Crazy Lab enables tube parts in the shared display contract.
The frame embeds `common/images/equation_window_shadow.png`, a shared 2x
nine-sliced shadow. Regenerate it with `python3 tools/equation_shadows.py`
(Pillow is needed only for regeneration). Its transparent margins contain the
penumbra without clipping. Six native Edje bands below the header follow
Elaro's fade pattern.

Each subdirectory's Meson target uses `edje_cc -deps` so edits to included groups
rebuild every affected archive. All three archives install as
`share/equation/themes/equation_<name>.edj`. No loose images are needed at runtime.

The default is Observatory. The menu switches themes immediately and stores the
catalog name in the Eet config. Only built-in catalog themes can be selected.

## Application contracts

A compatible archive supplies these groups:

- `equation/frame`: `equation.content` and `equation.controls` swallows,
  a separate `equation.menu` swallow to the left of `equation.title` text, `equation.move` hit surface, and the eight resize parts
  `top`, `bottom`, `left`, `right`, `tl`, `tr`, `bl`, and `br`.
  The `equation,dialog` signal from `equation` contracts the controls area to a
  single Close button and leaves room for the About title. `equation,maximized`
  and `equation,restored` signals from `equation` remove and restore the shadow
  padding. Move and resize surfaces follow the panel inside that padding.
- `equation/display`: `equation.value` text for the full decimal result.
  Optional text parts `equation.digit.0` through `equation.digit.13` receive
  individual characters, right-aligned. The UI emits `equation,digits` for
  results up to fourteen characters and `equation,compact` otherwise, both
  from `equation`. Full precision is retained in the copy action and selectable
  conversion entries. `equation.copy` swallows the copy button; `equation.note`
  shows angle/mode information and copy feedback. The full text readout shrinks
  to fit long results. `equation,motion,on` and `equation,motion,off`, from
  `equation`, start and stop ambient motion. `equation,result` triggers the
  calculation ignition and scan. Motion stops while hidden or iconified and
  respects the saved Ambient animations preference. Decorative animation never
  changes numeric values.
- `equation/result-row`: a conversion card with `equation.base` text and
  `equation.result` swallow for a transparent, scrolling, read-only entry.
- `equation/icon/copy` and `equation/icon/check`: scalable copy/confirmation
  artwork. The copy button uses the Elementary `equation/copy` style.
- Elementary groups for buttons (including `equation/window` and
  `equation/equals` styles), labels, radios, checkboxes, entries, selections,
  cursor, popup, notification backdrop, and separator. Shared group files are
  the reference implementation. Other widgets can fall back to the desktop
  theme.

Archives are applied as overlays to the process's Elementary theme. They do not
change the desktop's theme settings. The old overlay is removed after adding
its replacement, and all open windows are restyled together.

## Adding a theme

1. Copy a theme directory and rename its `equation_*.edc` entry point.
2. Change its palette, typography, metadata, and SVG artwork. Keep the shared
   part and signal contracts intact.
3. Register its subdirectory in `data/meson.build` and add a catalog entry in
   `src/fe-efl/equation_theme.c` (adjust the array size in its header).
4. Build and run `meson test -C build --print-errorlogs`.

To export previews from the real UI for all themes, including menu and About:

```sh
mkdir -p /tmp/equation-previews
ELM_ENGINE=buffer ELM_SCALE=1.5 EINA_LOG_LEVEL=0 ECORE_NO_SYSTEM_MODULES=1 \
  ./build/tests/equation_ui_test /tmp/equation-previews
```

The UI test uses a temporary XDG config directory and removes it afterward.

Crazy Lab constrains `tubes_area` and each tube with native Edje aspect rules,
so resizing changes their available space without stretching the glass. Digits
fit inside their tube geometry. Observatory includes `equation_hover_parts.edc`
and `equation_hover_programs.edc` for the cyclic cyan backlight. Hover or keyboard focus starts the loop. It fades to idle once neither remains,
or when disabled/hidden. Shared `equation_button_script.edc` and
`equation_button_programs.edc` merge pointer, keyboard focus, pressed and disabled
state, so losing focus while hovered does not erase the hover animation. The
`equation,hover,on` / `equation,hover,off` signals control the pulse separately
from press feedback. Decorative parts must keep `mouse_events: 0`; only the
button’s stable `event` rectangle owns pointer input.
