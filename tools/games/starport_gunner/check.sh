#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../../.."
mkdir -p build/games/starport_gunner
"${CC:-cc}" -std=c11 -O2 -Wall -Wextra -Werror -Imain/games/starport_gunner -Iassets/fonts \
    tests/games/test_starport_gunner.c main/games/starport_gunner/sg_game.c main/games/starport_gunner/sg_render.c \
    -o build/games/starport_gunner/test_game_plain
build/games/starport_gunner/test_game_plain
python3 tools/games/starport_gunner/generate_font.py --check
python3 tools/games/starport_gunner/build_preview.py --check
node tools/games/starport_gunner/check_parity.mjs
