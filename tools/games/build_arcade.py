#!/usr/bin/env python3
"""Build a portable static arcade using the verified per-game browser previews."""
import argparse
import json
from pathlib import Path
import shutil
import subprocess
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'build/games/arcade'
GAMES = [
    ('road_rage', '狂飙骑手', '连超三车，触发加速，一路冲线。', 'A/C 换道 · B 可选攻击', 795, 'community-f3e36d6a', '竞速'),
    ('cloudbound', '云间一跃', '按住、松手，连续精准落在云海浮岛。', 'B 蓄力 · 松手跳跃', 807, 'community-ae67f644', '跳跃'),
    ('alley_ninja', '夜巷忍者', '读懂出招，完美格挡后反击。', 'A/C 斩击 · B 格挡', 809, 'community-74ce7b27', '对决'),
    ('brick_workshop', '弹砖工坊', '控制反弹，把整面彩窗逐块打亮。', 'A/C 移动 · B 发球或减速', 831, 'community-1e38a32c', '技巧'),
    ('rooftop_runner', '跃影疾行', '越过城市屋顶，挑战无失误与全收集。', 'A 左 · B 右 · C 跳跃', 843, 'community-5b9d80a1', '跑酷'),
]

def build(build_games=False):
    OUT.mkdir(parents=True, exist_ok=True)
    cards = []
    for game, title, pitch, controls, project, slug, category in GAMES:
        if build_games:
            subprocess.run([sys.executable, str(ROOT / f'tools/games/{game}/build_preview.py')], check=True)
        subprocess.run([sys.executable, str(ROOT / f'tools/games/{game}/build_preview.py'), '--check'], check=True)
        src = ROOT / f'build/games/{game}/preview'
        dest = OUT / 'games' / game
        dest.mkdir(parents=True, exist_ok=True)
        # Only public preview files; never copy a build directory or credentials.
        for name in ('index.html', 'style.css', 'game.js', 'game.wasm'):
            shutil.copy2(src / name, dest / name)
        community = f'https://ai-passport.folotoy.cn/plays/{project}/'
        page = (dest / 'index.html').read_text()
        nav = f'<nav class="arcade-nav"><a href="../../">← 全部游戏</a><a href="{community}" target="_blank" rel="noopener">社区详情 ↗</a></nav>'
        page = page.replace('<body>', '<body>' + nav)
        page = page.replace('<head>', '<head><link rel="icon" href="data:,">')
        (dest / 'index.html').write_text(page)
        with (dest / 'style.css').open('a') as f:
            f.write('\n.arcade-nav{max-width:760px;margin:0 auto;padding:20px 24px 0;display:flex;justify-content:space-between;gap:12px}.arcade-nav a{color:inherit;text-underline-offset:5px}.arcade-nav a:focus-visible{outline:3px solid #e8bc71;outline-offset:4px}\n')
        cover = game.replace('_', '-') + '-cover-v2.png'
        (OUT / 'images').mkdir(exist_ok=True)
        shutil.copy2(ROOT / 'assets/images' / cover, OUT / 'images' / cover)
        cards.append(f'''<article class="game-card" id="{game}"><a class="poster" href="games/{game}/" aria-label="试玩{title}"><img src="images/{cover}" alt="{title}封面插画，非实机截图" width="1086" height="1448" loading="lazy"></a><div class="card-copy"><span class="category">{category} / 五关挑战</span><h2>{title}</h2><p>{pitch}</p><p class="controls">{controls}</p><div class="actions"><a class="button" href="games/{game}/">立即试玩 <span aria-hidden="true">↗</span></a><a class="community" href="{community}" target="_blank" rel="noopener">社区详情</a></div></div></article>''')
    template = (ROOT / 'tools/games/arcade/index.html').read_text()
    (OUT / 'index.html').write_text(template.replace('<!-- GAME_CARDS -->', '\n'.join(cards)))
    shutil.copy2(ROOT / 'tools/games/arcade/style.css', OUT / 'style.css')
    (OUT / 'manifest.json').write_text(json.dumps({'games': [g[0] for g in GAMES], 'communityProjects': [g[4] for g in GAMES]}, indent=2))
    archive = ROOT / 'build/games/pocket-arcade.zip'
    public_files = [OUT / 'index.html', OUT / 'style.css', OUT / 'manifest.json']
    public_files += [OUT / 'images' / (game.replace('_', '-') + '-cover-v2.png') for game, *_ in GAMES]
    for game, *_ in GAMES:
        public_files += [OUT / 'games' / game / name for name in ('index.html', 'style.css', 'game.js', 'game.wasm')]
    with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as bundle:
        for path in public_files:
            bundle.write(path, path.relative_to(OUT))
    print(f'Arcade ready: {OUT}')
    print(f'Static hosting bundle: {archive}')

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-games', action='store_true')
    args = parser.parse_args()
    build(args.build_games)
