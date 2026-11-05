<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Eatoday · what to eat today

> Stop arguing with yourself at noon. Hold one button, let the wheel decide.

**Eatoday** turns a FoloToy AI Passport into a pocket-sized "where should we eat"
wheel. Thirty Beijing places and dishes ship with it, the three physical buttons spin
the wheel, and anything you add yourself joins the wheel marked with a star. No Wi-Fi,
no Bluetooth, no account, no phone — it works the moment it boots.

**Version: v0.1**

## How it plays

| Where | Gesture | What happens |
| --- | --- | --- |
| Main screen | hold `UP` | the wheel starts spinning; let go and it slows down, then stops |
| Main screen | tap `DOWN` | step to the next option by hand |
| Main screen | hold `DOWN` | switch the wheel between *everything* and *starter list only* |
| Main screen | tap `OK` | lock in the current pick |
| Main screen | hold `OK` | open the add screen |
| While spinning | release `UP` | start slowing down |
| While spinning | tap `DOWN` | stop right there |
| Add screen | tap `UP` / `DOWN` | move between suggested words |
| Add screen | tap `OK` | pick that word as the category, tap again to append a second word |
| Add screen | hold `OK` | save it to your own list |
| Add screen | hold `UP` | cancel and go back |

The centre card shows the current option in large type; the line underneath counts
the starter list and your own entries, and the next line shows which range the wheel
is using. Your own entries appear with a star.

## What ships with it

- **30 starters** across roast duck, snacks and fast food, hotpot, Sichuan, home-style
  cooking, small neighbourhood places, and safe fallbacks for days when nobody cares.
- **Your own list**, saved on the device. Nothing to install, nothing to sync.
- **A starter list that lives in flash**: the first boot writes it to NVS, later boots
  read it back, and a corrupt copy falls back to the built-in table.
- **Twelve suggested words** for the add screen, since typing on three buttons is no fun.

## Build and flash

Requires ESP-IDF 5.5.3 for esp32c3. The merged image can be written to `0x0`:

```bash
./tools/validate.sh            # repository checks, host tests, firmware build
python -m esptool --chip esp32c3 -p <PORT> write_flash 0x0 build/FoloToy-AI-Passport-full.bin
```

Host tests cover the stopped-index calculation, the user list, the button routing state
machine, and per-character font coverage, so the logic is verifiable without hardware.

## Inside

- Application code and screens: [`main/`](main/), described in
  [`docs/eat-what.md`](docs/eat-what.md).
- Board support and drivers: [`components/bsp/`](components/bsp/).
- Chinese font subsets and their license: [`assets/fonts/README.md`](assets/fonts/README.md).

The screens are designed from scratch for this toy; the baseline hardware-test menu is
not used. Everything is stored on the device in NVS — no network code is enabled.

## License

MIT, see [`LICENSE`](LICENSE).
