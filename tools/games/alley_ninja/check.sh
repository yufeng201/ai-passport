#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../../.."
mkdir -p build/games/alley_ninja
"${CC:-cc}" -std=c11 -O2 -Wall -Wextra -Werror -Imain/games/alley_ninja -Iassets/fonts \
    tests/games/test_alley_ninja.c main/games/alley_ninja/an_game.c main/games/alley_ninja/an_render.c \
    -o build/games/alley_ninja/test_game_plain
build/games/alley_ninja/test_game_plain
python3 tools/games/alley_ninja/generate_font.py --check
python3 tools/games/alley_ninja/build_preview.py --check
node tools/games/alley_ninja/check_parity.mjs
