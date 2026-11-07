#!/usr/bin/env python3
"""Every character the application draws must be renderable by one of the fonts.

Two layers exist:

* the generated bitmap subsets carry the fixed UI text and the starter names;
* the embedded runtime TTF (``noto-sc-gb2312.ttf``) carries GB2312 level-1 and is
  used as the fallback for 16 px text and as the primary font for entries typed on
  the phone.

So a character is acceptable when it appears in the bitmap subset for the requested
size, or in the runtime TTF. Both are checked here; a negative control keeps the test
from passing unconditionally.
"""

from __future__ import annotations

import re
import struct
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import eat_what_glyphs  # noqa: E402  (repository tool, imported for reuse)

FONT_DIR = ROOT / "assets" / "fonts"
EXPECTED_SIZES = (16, 20, 24, 32, 44)
RUNTIME_TTF = FONT_DIR / "noto-sc-gb2312.ttf"
CMAP_RE = re.compile(
    r"\.range_start = (\d+), \.range_length = (\d+), \.glyph_id_start = \d+,\s*"
    r"\.unicode_list = (\w+), \.glyph_id_ofs_list = \w+, \.list_length = (\d+),\s*"
    r"\.type = (LV_FONT_FMT_TXT_CMAP_\w+)"
)


def parse_unicode_list(text: str, name: str) -> list[int]:
    match = re.search(rf"static const uint16_t {re.escape(name)}\[\] = \{{(.*?)\}};", text,
                      re.S)
    if match is None:
        raise AssertionError(f"unicode list {name} not found")
    return [int(value, 16) for value in re.findall(r"0x([0-9a-fA-F]+)", match.group(1))]


def bitmap_coverage(path: Path) -> set[int]:
    text = path.read_text(encoding="utf-8")
    covered: set[int] = set()
    for start, length, unicode_list, list_length, cmap_type in CMAP_RE.findall(text):
        start = int(start)
        length = int(length)
        entries = int(list_length)
        if cmap_type == "LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY":
            covered.update(range(start, start + length))
            continue
        offsets = parse_unicode_list(text, unicode_list)
        if len(offsets) != entries:
            raise AssertionError(f"{path.name}: {unicode_list} has {len(offsets)}, want {entries}")
        covered.update(start + offset for offset in offsets)
    return covered


def ttf_coverage(path: Path) -> set[int]:
    """Read the cmap of a TrueType file (formats 4 and 12) without extra deps."""
    data = path.read_bytes()
    table_count = struct.unpack(">H", data[4:6])[0]
    cmap_offset = None
    for index in range(table_count):
        record = 12 + index * 16
        if data[record:record + 4] == b"cmap":
            cmap_offset = struct.unpack(">I", data[record + 8:record + 12])[0]
            break
    if cmap_offset is None:
        raise AssertionError(f"{path.name}: no cmap table")

    codepoints: set[int] = set()
    subtables = struct.unpack(">H", data[cmap_offset + 2:cmap_offset + 4])[0]
    for index in range(subtables):
        record = cmap_offset + 4 + index * 8
        offset = struct.unpack(">I", data[record + 4:record + 8])[0]
        sub = cmap_offset + offset
        fmt = struct.unpack(">H", data[sub:sub + 2])[0]
        if fmt == 4:
            seg_x2 = struct.unpack(">H", data[sub + 6:sub + 8])[0]
            segs = seg_x2 // 2
            ends = struct.unpack(f">{segs}H", data[sub + 14:sub + 14 + seg_x2])
            starts_at = sub + 14 + seg_x2 + 2
            starts = struct.unpack(f">{segs}H", data[starts_at:starts_at + seg_x2])
            for start, end in zip(starts, ends):
                if end == 0xFFFF and start == 0xFFFF:
                    continue
                codepoints.update(range(start, end + 1))
        elif fmt == 12:
            groups = struct.unpack(">I", data[sub + 12:sub + 16])[0]
            base = sub + 16
            for group in range(groups):
                start, end, _ = struct.unpack(">III", data[base + group * 12:base + group * 12 + 12])
                codepoints.update(range(start, end + 1))
    return codepoints


class EatWhatFontCoverageTest(unittest.TestCase):
    def test_bitmap_subsets_cover_the_starter_names(self) -> None:
        """Starter entries are drawn with the bitmap fonts, so 24/32/44 px must carry them.

        Fixed UI text may fall back to the runtime TTF at 16 px; newer short strings
        therefore do not have to live in every bitmap size.
        """
        seed_only = [ROOT / "main/eat_what_seed.c"]
        symbols = eat_what_glyphs.visible_characters(seed_only)
        self.assertTrue(symbols, "no starter names were collected")
        for size in (24, 32, 44):
            covered = bitmap_coverage(FONT_DIR / f"app_font_{size}.c")
            missing = [c for c in symbols if ord(c) not in covered]
            self.assertEqual([], missing,
                             f"app_font_{size} lacks " + ", ".join(f"U+{ord(c):04X} {c}" for c in missing))

    def test_runtime_ttf_covers_every_screen_character(self) -> None:
        """The runtime TTF is the fallback for 16 px and the font for typed entries."""
        runtime = ttf_coverage(RUNTIME_TTF)
        self.assertGreater(len(runtime), 3000, "runtime TTF looks truncated")
        symbols = eat_what_glyphs.visible_characters()
        missing = [c for c in symbols if ord(c) not in runtime]
        self.assertEqual([], missing,
                         "runtime TTF lacks " + ", ".join(f"U+{ord(c):04X} {c}" for c in missing))

    def test_16px_text_is_covered_by_bitmap_or_runtime(self) -> None:
        """16 px uses the bitmap font with a runtime fallback, so either layer is fine."""
        bitmap = bitmap_coverage(FONT_DIR / "app_font_16.c")
        runtime = ttf_coverage(RUNTIME_TTF)
        for character in eat_what_glyphs.visible_characters():
            self.assertTrue(ord(character) in bitmap or ord(character) in runtime,
                            f"U+{ord(character):04X} {character} is in neither font")

    def test_negative_control_character_is_absent(self) -> None:
        # 龘 (U+9F98) is never displayed; it must not be in the subsets, otherwise the
        # coverage assertions could pass even if parsing collected everything.
        for size in EXPECTED_SIZES:
            self.assertNotIn(0x9F98, bitmap_coverage(FONT_DIR / f"app_font_{size}.c"),
                             f"app_font_{size} unexpectedly contains U+9F98")

    def test_symbols_used_by_the_ui_are_covered(self) -> None:
        runtime = ttf_coverage(RUNTIME_TTF)
        required = "★·…↑↓？"
        for size in EXPECTED_SIZES:
            covered = bitmap_coverage(FONT_DIR / f"app_font_{size}.c") | runtime
            for character in required:
                self.assertIn(ord(character), covered,
                              f"app_font_{size}: missing U+{ord(character):04X} {character}")


if __name__ == "__main__":
    unittest.main()
