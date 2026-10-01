#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../../.."
mkdir -p build/games/cloudbound
"${CC:-cc}" -std=c11 -O2 -Wall -Wextra -Werror -Imain/games/cloudbound -Iassets/fonts \
    tests/games/test_cloudbound.c main/games/cloudbound/cb_game.c main/games/cloudbound/cb_render.c \
    -o build/games/cloudbound/test_game_plain
build/games/cloudbound/test_game_plain
python3 tools/games/cloudbound/generate_font.py --check
python3 tools/games/cloudbound/build_preview.py --check
node tools/games/cloudbound/check_parity.mjs
