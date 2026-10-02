#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../../.."
mkdir -p build/games/rooftop_runner
"${CC:-cc}" -std=c11 -O2 -Wall -Wextra -Werror -Imain/games/rooftop_runner -Iassets/fonts \
    tests/games/test_rooftop_runner.c main/games/rooftop_runner/rp_game.c main/games/rooftop_runner/rp_render.c \
    -o build/games/rooftop_runner/test_game_plain
build/games/rooftop_runner/test_game_plain
python3 tools/games/rooftop_runner/generate_font.py --check
python3 tools/games/rooftop_runner/build_preview.py --check
node tools/games/rooftop_runner/check_parity.mjs
