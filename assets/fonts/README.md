<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Application fonts

`app_font_<size>.c` are LVGL bitmap font sources generated for the "Eatoday"
application (the `feature/eat-what` branch). They are committed because the
generator needs Node.js and a licensed CJK outline font, which the firmware
build must not depend on.

| File | Size | Purpose |
| --- | --- | --- |
| `app_font_16.c` | 16 px, 4 bpp | Title, battery, statistics, key hints |
| `app_font_20.c` | 20 px, 4 bpp | Add-page question, candidate rows |
| `app_font_24.c` | 24 px, 4 bpp | Long option names in the center card |
| `app_font_32.c` | 32 px, 4 bpp | Medium option names, typed text |
| `app_font_44.c` | 44 px, 4 bpp | Short option names (the visual focus) |

Total Flash cost of the five files is about 2.3 MB of read-only data; they cost
no internal RAM beyond LVGL's glyph cache.

## Source and license

- Source font: Noto Sans SC Regular (Subset OTF), `NotoSansSC-Regular.otf`
  from the [noto-cjk](https://github.com/notofonts/noto-cjk) repository.
- License: SIL Open Font License 1.1, see [`OFL.txt`](OFL.txt). The generated
  subsets are derivatives of that font and stay under the same license.
- The 8 MB `.otf` source is intentionally not committed; only the generated
  subsets and this license are.

## Character inventory

`tools/eat_what_glyphs.py` collects every non-ASCII character from the string
literals in `main/eat_what_text.h`, `main/eat_what_text.c`, and
`main/eat_what_seed.c`. Those are exactly the files whose text reaches the
240 × 320 screen; log messages and comments are excluded so they do not inflate
the subsets. ASCII `0x20`–`0x7E` is always included for digits, `%`, and `--`.

## Regenerate

```bash
npm install lv_font_conv@1.5.3            # Node.js converter, not a firmware dependency
EAT_FONT_SOURCE=/path/to/NotoSansSC-Regular.otf ./tools/gen_eat_what_fonts.sh
```

The script writes `assets/fonts/app_font_<size>.c` for sizes 16, 20, 24, 32, and
44 with `--bpp 4 --format lvgl --no-compress`.

## Integration

`main/CMakeLists.txt` adds the five generated sources to the `main` component
with `target_sources`, and `main/app_fonts.h` declares them
(`LV_FONT_DECLARE`). `eat_font_for_name()` picks the size by character count.

## Verification

`tests/test_eat_what_font_coverage.py` parses the cmap structures of every
generated file and fails when a displayed character is missing, and it includes
a negative control (U+9F98 must stay absent) so the check cannot pass
unconditionally. Visual rendering on the panel still has to be confirmed on the
device; see the delivery notes.
