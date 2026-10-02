#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../../.."
mkdir -p build/games/brick_workshop
"${CC:-cc}" -std=c11 -O2 -Wall -Wextra -Werror -Imain/games/brick_workshop -Iassets/fonts \
    tests/games/test_brick_workshop.c main/games/brick_workshop/bw_game.c main/games/brick_workshop/bw_render.c \
    -o build/games/brick_workshop/test_game_plain
build/games/brick_workshop/test_game_plain
python3 tools/games/brick_workshop/generate_font.py --check
python3 tools/games/brick_workshop/build_preview.py --check
node tools/games/brick_workshop/check_parity.mjs
