<p align="right">
  <a href="eat-what.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Eatoday: what to eat today

An offline decision toy for FoloToy AI Passport: a wheel of restaurants, your own lists,
and a phone-side editor for them. Spun with the three physical buttons. No internet
access, no audio, no paid dependencies; everything is stored in NVS.

This application replaces the baseline hardware-test menu with its own two screens.

## Interaction contract

| Context | Gesture | Result |
| --- | --- | --- |
| Main, idle | hold `UP` | spin from a random entry; release slows down and stops |
| Main, idle | short `DOWN` | step to the next entry |
| Main, idle | hold `DOWN` | cycle wheels: starter list → list 1 … list 3 → starter list, skipping empty lists |
| Main, idle | short `OK` | accept the current result |
| Main, idle | hold `OK` | open the phone editor (SoftAP + HTTP) |
| Main, spinning | release `UP` | begin deceleration |
| Main, spinning | short `DOWN` | stop immediately |
| Editor | hold `DOWN` | close the hotspot and return to the previous screen |
| Phone page | "back to cover" | device returns to the idle screen and closes the hotspot |

There is no on-device typing: lists are edited from a phone. Before the first spin the
centre card shows the title plus a "hold UP to start" hint, and switching lists does not
leave that state — the idle screen never parks on one restaurant.

## Screens

- Main: dark background `#0B0F14`; title and battery on top; one rounded card in the
  centre (idle hint, or the current option at 40/28 px via the runtime font for your own
  entries); a statistics line "starter N · list M"; a range line naming the active list;
  a two-line key hint.
- Editor: hotspot name, password, address and a hint; stays until `DOWN` is held.

Both are drawn from scratch with ordinary LVGL widgets; no baseline demo menu or
`ui_pixel_*` shell is reused.

## Phone editor

`main/eat_wifi.c` starts a WPA2 SoftAP named `Eatoday-XXXX` (last MAC bytes) with the
password `eatoday88`; `main/eat_web.c` serves:

| Route | Purpose |
| --- | --- |
| `GET /` | single-page editor (rename, add, delete, pick active list, save) |
| `GET /api/state` | current lists and active index as JSON |
| `POST /api/save` | full replace; validated, written to NVS, applied to the wheel |

Device-side limits: 3 lists, 50 entries each, 10 characters per entry, 7 characters per
list name, duplicates rejected, request body ≤ 8 KB. Wi-Fi only runs while the editor
screen is open.

## Data

- Starter pool: 30 entries (`main/eat_what_seed.c`), written to NVS on first boot with
  `seed_ver`; a missing or corrupt snapshot falls back to the compiled table.
- Factory list: on first boot (no list metadata in NVS) list 1 is filled with 18 everyday
  places and named "regulars" (`eat_sets_fill_defaults`). Clearing it is permanent; the
  defaults are not re-applied once metadata exists.
- Lists: `set_meta` (names and active index) plus `set0`…`set2` (one blob per list, at
  most 50 × 32 bytes). Lists are independent from the starters; an empty list is skipped
  when cycling, so the wheel is never blank.
- Migration: a v0.1 flat `user` list is imported into list 1 on first boot of the new
  format.

## Wheel timing and randomness

Positions are integer milli-index units and speed is milli-index per millisecond, so a
given `dt` sequence always stops on the same entry. Spin speed is 22 entries per second;
deceleration uses a 260 ms time constant and snaps below 0.9 entries per second.
`eat_wheel_randomize()` picks the starting entry from a caller-supplied random value:
the boot entry and every spin start are randomized, so the same place does not keep
winning.

## Fonts

Two layers:

1. Bitmap subsets (16/20/24/32/44 px) for the fixed UI text and the starter names,
   generated from the literals in `main/eat_what_text.h`, `main/eat_what_text.c`, and
   `main/eat_what_seed.c`; the 16 px size carries a runtime fallback, so a missed
   character degrades to the TTF instead of a box.
2. A runtime TTF (`assets/fonts/noto-sc-gb2312.ttf`, full GB2312, 6763 characters)
   embedded as read-only data and rasterised by LVGL's Tiny TTF, used for the fallback
   and for everything typed on the phone. A bitmap subset of the same range would need
   roughly 6.5 MB at 24 px, which does not fit the 8 MB flash.

Both derive from Noto Sans SC under the SIL Open Font License 1.1; see
`assets/fonts/README.md`.

## Tests

```bash
./tools/validate.sh --static
```

Covers the stopped-index calculation and randomized starts, list storage, meta round
trips, factory defaults, empty-list skipping, the App Router branches for idle/spinning/
editor pages, seed data integrity, and font coverage.

## Unverified on device

The hotspot and HTTP round trip are exercised by hand (opening the editor, joining from a
phone, saving a list). Automated tests cover the data model and routing, not the radio.
