#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../../.."
mkdir -p build/games/road_rage
"${CC:-cc}" -std=c11 -O2 -Wall -Wextra -Werror -Imain/games/road_rage -Imain/games/common -Iassets/fonts \
    tests/games/test_road_rage.c main/games/road_rage/rr_game.c main/games/road_rage/rr_render.c \
    main/games/common/game_audio.c -o build/games/road_rage/test_game_plain
build/games/road_rage/test_game_plain
python3 tools/games/road_rage/generate_font.py --check
python3 tools/games/road_rage/build_preview.py --check
node tools/games/road_rage/check_parity.mjs
