<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Eatoday · what to eat today

> Stop arguing with yourself at noon. Hold one button, let the wheel decide.

**Eatoday** turns a FoloToy AI Passport into a pocket-sized "where should we eat"
wheel. Thirty places ship with it, a starter list of everyday favourites is ready on
first boot, and you can rewrite your own lists from your phone without installing
anything.

**Version: v1.1** — phone-side list editor, runtime Chinese font, no more on-device typing.

## How it plays

| Where | Gesture | What happens |
| --- | --- | --- |
| Main screen | hold `UP` | the wheel spins from a random entry; release and it slows to a stop |
| Main screen | tap `DOWN` | step to the next option by hand |
| Main screen | hold `DOWN` | switch wheels: starter list → your list 1 … list 3 → starter list (empty lists are skipped) |
| Main screen | tap `OK` | lock in the current pick |
| Main screen | hold `OK` | open the phone editor (starts a Wi-Fi hotspot) |
| While spinning | release `UP` | start slowing down |
| While spinning | tap `DOWN` | stop right there |
| Phone editor screen | hold `DOWN` | close the hotspot and go back |

Before you start spinning, the centre card shows the title and "hold UP to start" — it
never parks on one restaurant, and switching lists does not disturb it. Once you spin,
each spin starts from a random entry so the same place does not keep winning.

## Build your own lists from a phone

1. Hold `OK` on the main screen. The device shows a hotspot such as `Eatoday-3F2A`, the
   password `eatoday88`, and the address `http://192.168.4.1`.
2. Join that hotspot from your phone (no internet needed; keep the connection when the
   phone warns about it).
3. Open `http://192.168.4.1` and edit three independent lists: rename them, add or
   delete entries (up to 50 each), pick which one the wheel should use, then tap **Save**.
4. Hold `DOWN` on the device to leave; the hotspot closes.

Anything you type is drawn with a runtime font covering the full GB2312 set, so unusual
characters show up instead of empty boxes.

## What ships with it

- **30 starters** across roast duck, snacks and fast food, hotpot, Sichuan, home-style
  cooking, small neighbourhood places, and safe fallbacks.
- **A ready-made list of 18 everyday places**, written into your first list on first
  boot so you can spin something familiar immediately; edit or clear it any time.
- **Three private lists**, 50 entries each, independent from the starters. Empty lists do
  not appear on the device.
- **Everything on the device**: starter list and your lists live in NVS and survive power
  loss. The hotspot only exists while the editor screen is open; nothing is sent anywhere.

## Build and flash

Requires ESP-IDF 5.5.3 for esp32c3. The merged image can be written to `0x0`:

```bash
./tools/validate.sh            # repository checks, host tests, firmware build
python -m esptool --chip esp32c3 -p <PORT> write_flash 0x0 build/FoloToy-AI-Passport-full.bin
```

Host tests cover the stopped-index calculation and randomized starts, list storage and
empty-list skipping, the button routing state machine, and per-character font coverage.

## Inside

- Application code and screens: [`main/`](main/), described in
  [`docs/eat-what.md`](docs/eat-what.md).
- Board support and drivers: [`components/bsp/`](components/bsp/).
- Chinese bitmap subsets, the runtime TTF and their license:
  [`assets/fonts/README.md`](assets/fonts/README.md).

The screens are designed from scratch for this toy; the baseline hardware-test menu is
not used.

## License

MIT, see [`LICENSE`](LICENSE).
