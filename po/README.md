# Translating Equation

Equation uses GNU gettext with the `equation` domain, as in Elaro. Catalogs
install to Meson's `localedir` (normally `share/locale`). English is the fallback;
no completed translations ship yet.

Install the gettext tools, then refresh the template:

```sh
meson compile -C build equation-pot
```

Create a language catalog and add its language code to `po/LINGUAS`:

```sh
msginit --input=po/equation.pot --locale=fr --output-file=po/fr.po
```

After editing it, validate and build:

```sh
msgfmt --check --check-format -o /tmp/equation-fr.mo po/fr.po
meson setup --reconfigure build
meson compile -C build
meson install -C build
```

Use `meson compile -C build equation-update-po` after source messages change.
List new translatable source files in `po/POTFILES`. `_()` translates immediately;
`N_()` marks static messages translated when displayed. The desktop launcher is
merged from the same catalogs. Retain placeholders such as `%s` and `%zu`, and
keep the semicolon separators in desktop keywords. Preserve the copyright line
in About. Calculator tokens (`sin`, `0x`, operators, digits) stay unchanged.

The system locale chooses the UI language; GNU gettext also supports `LANGUAGE`.
For example, after installing a French catalog, use
`LANGUAGE=fr equation` with a non-C system locale. A C/POSIX locale uses English.
Expressions and copied numeric results always use `.` as the decimal separator;
only text and diagnostics are translated.

Tests build a synthetic catalog under `build/tests/locale`; it is never installed.
The translation tests require the `en_US.UTF-8` locale and skip if unavailable.
