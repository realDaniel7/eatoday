<p align="right">
  <a href="eat-what.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Eatoday: what to eat today

An offline decision toy for FoloToy AI Passport: a wheel of Beijing restaurants and
dishes, spun with the three physical buttons. No Wi-Fi, no Bluetooth, no audio, no
paid dependencies; everything is stored in NVS.

This application replaces the baseline hardware-test menu with its own two-screen UI.

## Interaction contract

| Context | Gesture | Result |
| --- | --- | --- |
| Main, idle | hold `UP` | start spinning; release slows down and stops |
| Main, idle | short `DOWN` | step to the next entry |
| Main, idle | hold `DOWN` | toggle wheel scope: all entries / seed pool only |
| Main, idle | short `OK` | accept the current result (shows "that's it") |
| Main, idle | hold `OK` | open the add page |
| Main, spinning | release `UP` | begin deceleration |
| Main, spinning | short `DOWN` | stop immediately |
| Add | short `UP` / `DOWN` | move the candidate cursor |
| Add | short `OK` | select the highlighted word as category, then optionally as a suffix |
| Add | hold `OK` | save (category only when nothing was appended) |
| Add | hold `UP` | cancel and return |

The three buttons share one ADC ladder, so every gesture is dispatched by a single
App Router (`main/eat_what_router.c`); pages never read buttons themselves.

## Screens

- Main: dark background `#0B0F14`; title "Beijing · what to eat today" on top with the
  battery percentage at the right; a single rounded card in the center showing the
  current option at 44/32/24 px depending on length; "seed N · mine M" below it;
  a scope line; a two-line key hint. User entries are prefixed with a star.
- Add: question at the top, the typed text in the middle (6 characters maximum),
  and three candidate rows at the bottom that scroll with the cursor.

Both screens are drawn from scratch with ordinary LVGL widgets; no baseline demo
menu, `ui_pixel_*` shell, or demo page is reused.

## Data

- Seed pool: 30 entries across roast duck, snacks, hotpot, Sichuan, homestyle, small
  local places, and light-decision fallbacks (`main/eat_what_seed.c`).
- First boot writes a seed snapshot into NVS (`namespace eatwhat`, key `seed`, with
  `seed_ver`); later boots read that snapshot, and fall back to the compiled table
  when the blob is missing or corrupt.
- User pool: key `user`, at most 32 names, each at most 31 UTF-8 bytes, duplicates
  rejected. Wheel scope: key `scope`.
- Blob layout (little endian): `u16 version | u16 count | count × char[32]`.
- The add page has no keyboard; it offers 12 high-frequency words. Selecting one
  saves just that word; short-pressing `OK` again appends a second word as a short
  name (`category·suffix`).

## Wheel timing

Positions are integer milli-index units, speed is milli-index per millisecond, so the
same `dt` sequence always stops on the same entry. Spin speed is 22 entries per
second; deceleration uses a 260 ms time constant and snaps to the nearest entry below
0.9 entries per second.

## Fonts

Chinese text uses five generated LVGL subsets (16/20/24/32/44 px) derived from Noto
Sans SC (OFL 1.1). `tools/gen_eat_what_fonts.sh` regenerates them from the literals in
`main/eat_what_text.h`, `main/eat_what_text.c`, and `main/eat_what_seed.c`;
`tests/test_eat_what_font_coverage.py` fails when any displayed character is missing.
See `assets/fonts/README.md` for the license and regeneration steps.

## Tests

Host tests run without hardware:

```bash
./tools/validate.sh --static
```

They cover the stopped-index calculation and deceleration determinism, user-pool
appends and blob round trips, the App Router branches for idle/spinning/add pages,
seed data integrity, and font coverage.
