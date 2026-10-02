#!/usr/bin/env python3
"""Build/check the shared C renderer and model as a standalone browser Wasm module."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[3]
SOURCES = [ROOT / 'main/games/road_rage' / f for f in ('rr_game.c', 'rr_game.h', 'rr_render.c', 'rr_render.h', 'rr_wasm.c', 'rr_copy.h')]
SOURCES += [ROOT / 'main/games/common/game_audio.c', ROOT / 'main/games/common/game_audio.h']
SOURCES += [ROOT / 'assets/fonts/road_rage_noto_sc_12.h', ROOT / 'assets/fonts/road_rage_noto_sc_subset.otf']
SOURCES += [ROOT / 'main/games/common/game_visual.h', ROOT / 'main/games/common/game_achievements.h']
OUT = ROOT / 'build/games/road_rage/preview'
SHELL = ROOT / 'tools/games/road_rage/preview'

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def check():
    manifest = json.loads((OUT / 'manifest.json').read_text())
    for name, expected in manifest['files'].items():
        path = ROOT / name if name.startswith(('main/', 'tools/', 'assets/')) else OUT / name
        if digest(path) != expected:
            raise SystemExit(f'Stale preview: {name}; rebuild with tools/games/road_rage/build_preview.py')
    print('Shared-source / Wasm / preview hashes: PASS')

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    if args.check:
        check()
    else:
        candidates = list((ROOT / 'build/toolchains').glob('wasi-sdk-*/bin/clang'))
        compiler = os.environ.get('WASI_CLANG') or (str(candidates[0]) if candidates else shutil.which('clang'))
        if not compiler:
            raise SystemExit('Set WASI_CLANG to a wasm32-capable clang from wasi-sdk.')
        OUT.mkdir(parents=True, exist_ok=True)
        exports = ('game_init', 'game_input', 'game_tick', 'game_frame', 'game_hash', 'game_phase', 'game_health', 'game_attack', 'game_knockouts', 'game_boost', 'game_sound_sample', 'game_music_theme', 'game_music')
        cmd = [compiler, '--target=wasm32', '-O2', '-std=c11', '-Wall', '-Wextra', '-Werror', '-nostdlib',
               '-fno-builtin', '-I' + str(ROOT / 'assets/fonts'), '-I' + str(ROOT / 'main/games/common'), '-Wl,--no-entry', '-Wl,--export-memory', '-Wl,--initial-memory=262144',
               '-Wl,--max-memory=262144', '-Wl,-z,stack-size=16384']
        cmd += [f'-Wl,--export={name}' for name in exports]
        cmd += [str(p) for p in SOURCES if p.suffix == '.c'] + ['-o', str(OUT / 'game.wasm')]
        subprocess.run(cmd, check=True)
        for source in SHELL.iterdir():
            if source.is_file(): shutil.copy2(source, OUT / source.name)
        tracked = SOURCES + [p for p in SHELL.iterdir() if p.is_file()]
        files = {str(p.relative_to(ROOT)): digest(p) for p in tracked}
        files.update({p.name: digest(p) for p in OUT.iterdir() if p.is_file() and p.name != 'manifest.json'})
        version = subprocess.check_output([compiler, '--version'], text=True).splitlines()[0]
        (OUT / 'manifest.json').write_text(json.dumps({'compiler': version, 'files': files}, indent=2) + '\n')
        check()
        print(f'Preview: {OUT}\nServe: python3 -m http.server 8765 --bind 127.0.0.1 --directory {OUT}')
