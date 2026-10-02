#!/usr/bin/env python3
"""Render finished 3:4 promotional artwork from native gameplay fixtures."""
from pathlib import Path
import os
import subprocess
from PIL import Image, ImageDraw, ImageFont
ROOT = Path(__file__).resolve().parents[2]
GAMES = [
 ('road_rage','rr','狂飙骑手','连超加速 · 贴身对决',(22,37,43),(255,194,92)),
 ('cloudbound','cb','云间一跃','按住 · 松手 · 精准落地',(31,49,65),(157,228,214)),
 ('alley_ninja','an','夜巷忍者','完美格挡 · 连段反击',(22,25,46),(175,228,214)),
 ('brick_workshop','bw','弹砖工坊','控制反弹 · 点亮彩窗',(18,39,51),(255,194,116)),
 ('rooftop_runner','rp','跃影疾行','越过屋顶 · 无失误挑战',(30,47,60),(255,205,135)),
]
for game,prefix,title,pitch,bg,accent in GAMES:
    out=ROOT/f'build/games/{game}';out.mkdir(parents=True,exist_ok=True)
    binary=out/'render_promotion'
    subprocess.run([os.environ.get('CC','cc'),'-std=c11','-O2','-Wall','-Wextra','-Werror',f'-I{ROOT}/main/games/{game}',f'-I{ROOT}/assets/fonts',str(ROOT/f'tools/games/{game}/render_frames.c'),str(ROOT/f'main/games/{game}/{prefix}_game.c'),str(ROOT/f'main/games/{game}/{prefix}_render.c'),'-o',str(binary)],check=True)
    # Completion is the native renderer's successful exit, followed by composition.
    subprocess.run([str(binary)],cwd=ROOT,check=True)
    image=Image.new('RGB',(960,1280),bg);d=ImageDraw.Draw(image)
    # Licensed full Noto font outside the repository; never a system font.
    font_path=ROOT/'build/road_rage/NotoSansCJKsc-Medium.otf'
    if not font_path.exists():raise SystemExit('Full Noto Medium source needed for promotional captions')
    def font(size):return ImageFont.truetype(str(font_path),size)
    d.text((64,52),'POCKET ARCADE / '+str([g[0] for g in GAMES].index(game)+1).zfill(2),font=font(20),fill=accent)
    d.text((60,116),title,font=font(86),fill='#f5f2e7')
    d.text((64,238),pitch,font=font(30),fill=accent)
    frame=Image.open(out/'promo.ppm').convert('RGB').resize((960,720),Image.Resampling.LANCZOS)
    image.paste(frame,(0,340))
    d=ImageDraw.Draw(image);d.line((64,1108,896,1108),fill=accent,width=2)
    d.text((64,1132),'三键操作 / 五关挑战 / 离线游玩',font=font(27),fill='#f5f2e7')
    d.text((64,1224),'玩法绘图预览 · 非实机截图',font=font(21),fill='#b3c3c8')
    dest=ROOT/f'assets/images/{game.replace("_","-")}-promo.png';image.save(dest)
    print(f'Render complete: {dest}')
