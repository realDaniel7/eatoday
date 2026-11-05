#!/usr/bin/env bash
# Regenerate the application's CJK font subsets from a licensed source font.
#
# Usage:
#   EAT_FONT_SOURCE=/path/to/NotoSansSC-Regular.otf ./tools/gen_eat_what_fonts.sh
#
# The character inventory is derived from the string literals in main/ (see
# tools/eat_what_glyphs.py), so adding UI text only requires re-running this
# script. tests/test_eat_what_font_coverage.py fails when the generated files
# no longer cover the sources.
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
font_source="${EAT_FONT_SOURCE:-}"
converter="${LV_FONT_CONV:-lv_font_conv}"

if [[ -z "${font_source}" || ! -f "${font_source}" ]]; then
    echo "EAT_FONT_SOURCE must point at a licensed CJK font (OTF/TTF)." >&2
    exit 2
fi
if ! command -v "${converter}" >/dev/null 2>&1; then
    echo "lv_font_conv is not available; install '@lvgl/lv_font_conv' first." >&2
    exit 2
fi

symbols="$(python3 "${repo_root}/tools/eat_what_glyphs.py")"
echo "glyphs: $(printf '%s' "${symbols}" | wc -c) characters"

for size in 16 20 24 32 44; do
    "${converter}" \
        --font "${font_source}" \
        --range 0x20-0x7E \
        --symbols "${symbols}" \
        --size "${size}" \
        --bpp 4 \
        --format lvgl \
        --no-compress \
        --lv-font-name "app_font_${size}" \
        --lv-include lvgl.h \
        --output "${repo_root}/assets/fonts/app_font_${size}.c"
    echo "generated assets/fonts/app_font_${size}.c"
done
