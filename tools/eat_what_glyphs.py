#!/usr/bin/env python3
"""Collect the non-ASCII characters that the Eatoday application actually displays.

Only C string literals in the on-screen text files are inspected: comments and
log messages must not enlarge the generated font subsets. The output is a single line of characters for
``lv_font_conv --symbols``.
"""

from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
# Only the files whose literals are actually drawn on the 240x320 screen:
# log messages and internal keys must not inflate the generated subsets.
SOURCES = [
    ROOT / "main/eat_what_text.h",
    ROOT / "main/eat_what_text.c",
    ROOT / "main/eat_what_seed.c",
]


def strip_comments_and_strings(source: str) -> tuple[str, list[str]]:
    """Return the code with comments blanked, plus every string literal found."""
    code: list[str] = []
    literals: list[str] = []
    index = 0
    length = len(source)
    while index < length:
        char = source[index]
        nxt = source[index + 1] if index + 1 < length else ""
        if char == "/" and nxt == "/":
            while index < length and source[index] != "\n":
                index += 1
            continue
        if char == "/" and nxt == "*":
            index += 2
            while index + 1 < length and not (source[index] == "*" and source[index + 1] == "/"):
                index += 1
            index += 2
            continue
        if char == '"':
            index += 1
            buffer: list[str] = []
            while index < length and source[index] != '"':
                if source[index] == "\\" and index + 1 < length:
                    buffer.append(source[index])
                    buffer.append(source[index + 1])
                    index += 2
                    continue
                buffer.append(source[index])
                index += 1
            literals.append("".join(buffer))
            code.append('""')
            index += 1
            continue
        code.append(char)
        index += 1
    return "".join(code), literals


def visible_characters() -> str:
    characters: set[str] = set()
    for path in SOURCES:
        _, literals = strip_comments_and_strings(path.read_text(encoding="utf-8"))
        for literal in literals:
            for character in literal:
                if ord(character) > 0x7E and character not in "\n\r\t":
                    characters.add(character)
    return "".join(sorted(characters))


def main() -> int:
    symbols = visible_characters()
    if not symbols:
        print("no non-ASCII literals found in main/", file=sys.stderr)
        return 1
    print(symbols)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
