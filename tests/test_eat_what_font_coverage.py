#!/usr/bin/env python3
"""Every character the application draws must exist in each generated font subset.

The generated LVGL fonts carry an explicit cmap: a dense range for ASCII plus a
sparse ``unicode_list`` for the CJK characters. This test reads those structures
and checks them against the characters collected from the on-screen literals, so
a new label with a missing glyph fails here instead of showing a blank box on the
device. A negative case keeps the check from passing unconditionally.
"""

from __future__ import annotations

import re
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import eat_what_glyphs  # noqa: E402  (repository tool, imported for reuse)

FONT_DIR = ROOT / "assets" / "fonts"
EXPECTED_SIZES = (16, 20, 24, 32, 44)
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


def coverage(path: Path) -> set[int]:
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


class EatWhatFontCoverageTest(unittest.TestCase):
    def test_every_screen_character_is_covered(self) -> None:
        symbols = eat_what_glyphs.visible_characters()
        self.assertTrue(symbols, "no on-screen characters were collected")
        for size in EXPECTED_SIZES:
            path = FONT_DIR / f"app_font_{size}.c"
            self.assertTrue(path.is_file(), f"missing generated font {path.name}")
            covered = coverage(path)
            missing = [character for character in symbols if ord(character) not in covered]
            self.assertEqual(
                [], missing,
                f"app_font_{size} lacks " + ", ".join(f"U+{ord(c):04X} {c}" for c in missing),
            )

    def test_ascii_is_covered_for_digits_and_battery_percent(self) -> None:
        for size in EXPECTED_SIZES:
            covered = coverage(FONT_DIR / f"app_font_{size}.c")
            for codepoint in range(0x20, 0x7F):
                self.assertIn(codepoint, covered, f"app_font_{size}: missing U+{codepoint:04X}")

    def test_negative_control_character_is_absent(self) -> None:
        # 龘 (U+9F98) is never displayed; it must not be in the subsets, otherwise the
        # coverage assertion above could pass even if parsing collected everything.
        for size in EXPECTED_SIZES:
            covered = coverage(FONT_DIR / f"app_font_{size}.c")
            self.assertNotIn(0x9F98, covered, f"app_font_{size} unexpectedly contains U+9F98")

    def test_symbols_used_by_the_ui_are_covered(self) -> None:
        # The star marks user entries, the arrow glyphs appear in the key hints.
        required = "★·…↑↓？"
        for size in EXPECTED_SIZES:
            covered = coverage(FONT_DIR / f"app_font_{size}.c")
            for character in required:
                self.assertIn(ord(character), covered,
                              f"app_font_{size}: missing U+{ord(character):04X} {character}")


if __name__ == "__main__":
    unittest.main()
