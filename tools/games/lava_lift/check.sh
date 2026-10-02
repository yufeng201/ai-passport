#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../../.."
mkdir -p build/games/lava_lift
"${CC:-cc}" -std=c11 -O2 -Wall -Wextra -Werror -Imain/games/lava_lift -Iassets/fonts tests/games/test_lava_lift.c main/games/lava_lift/ll_game.c main/games/lava_lift/ll_render.c -o build/games/lava_lift/test_game_plain
build/games/lava_lift/test_game_plain
python3 tools/games/lava_lift/build_preview.py --check
node tools/games/lava_lift/check_parity.mjs
