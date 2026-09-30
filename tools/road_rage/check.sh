#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
mkdir -p build/road_rage
"${CC:-cc}" -std=c11 -O2 -Wall -Wextra -Werror -Imain/road_rage -Iassets/fonts \
    tests/test_road_rage.c main/road_rage/rr_game.c main/road_rage/rr_render.c \
    main/road_rage/rr_sound.c -o build/road_rage/test_game_plain
build/road_rage/test_game_plain
python3 tools/road_rage/generate_font.py --check
python3 tools/road_rage/build_preview.py --check
node tools/road_rage/check_parity.mjs
