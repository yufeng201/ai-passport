<p align="right">
  <strong>简体中文</strong> · <a href="README.md">English</a>
</p>

# 资源目录（Assets）

本目录集中存放可复用的资源（字库、图片、音乐等），按资源类型分子目录管理。每个资源放在其类型对应的子目录，并记录放置路径、命名方式、集成方式与来源/许可。二进制资源（字体、图片、音频）不属于纯 markdown 文档，请勿与文档混放。涉及版权/授权的资源需注明来源与许可。

## 字库（fonts）

可复用的字库文件与生成的字库源码放在 `fonts/`。

- 命名要能反映字族、字重、字级与格式。
- 记录来源、许可、字符范围、转换命令与目标放置路径。
- 添加字库前评估 Flash 与内部 RAM 影响；ESP32-C3 无 PSRAM。
- 不提交许可不允许分发的字库。

- `fonts/road_rage_noto_sc_subset.otf` 和 `fonts/road_rage_noto_sc_12.h`：208 个 Noto Sans CJK SC/ASCII 字形，三档实际字号分别栅格化，4bpp 抗锯齿（压缩字模 182,854 字节，另加字形指标），遵循 [SIL OFL](fonts/OFL.txt)。为兼容保留 `_12` 文件名，现包含三档字号。用 `python3 tools/games/road_rage/generate_font.py [--check]` 生成/检查；扩展字符使用 `--source <完整字体.otf>`。OTF 不链接到固件。

- `fonts/cloudbound_noto_sc_subset.otf` 和 `fonts/cloudbound_noto_sc_12.h`：188 个 Noto Sans CJK SC/ASCII 字形，三档实际字号分别栅格化，4bpp 抗锯齿（压缩字模 159,334 字节，另加字形指标），遵循 [SIL OFL](fonts/OFL.txt)。为兼容保留 `_12` 文件名，现包含三档字号。用 `python3 tools/games/cloudbound/generate_font.py [--check]` 生成/检查；扩展字符使用 `--source <完整字体.otf>`。OTF 不链接到固件。

## 图片（images）

可复用的源图与生成的显示资产放在 `images/`。

| 文件 | 尺寸与格式 | 用途与来源 |
| --- | --- | --- |
| [`images/home.jpg`](images/home.jpg) | 3840 × 2160，JPEG | 嵌入中英文项目 README 的产品主图，突出 AI Passport 产品形象与开放、人人可创作的理念。 |
| [`images/readme-hardware-specs.png`](images/readme-hardware-specs.png) | 2172 × 724，PNG RGBA | 保留为可选技术参考图，不再用于首页主视觉。于 2026-09-17 使用内置图像生成工具为本仓库生成；已根据文档中的硬件能力契约核对图中的六项标签与参数。 |
| [`images/logo-wordmark.png`](images/logo-wordmark.png) | 1648 × 336，PNG RGBA | 从仓库原始 `images/logo.png` 中精确裁切并去除背景的黑色字标；用于中英文项目 README 的浅色主题。 |
| [`images/logo-wordmark-dark.png`](images/logo-wordmark-dark.png) | 1648 × 336，PNG RGBA | 提取字标的白色版本；README 使用 `<picture>` 在 GitHub 深色主题下显示。 |

- 使用描述性命名，并记录尺寸、像素格式、转换步骤与目标路径。
- 优先采用适合 240 × 320 RGB565 显示的格式，并纳入 Flash 与内部 RAM 考量。
- 许可允许时保留可编辑源文件，并记录来源与许可。
- 图片中不得包含设备二维码秘密、凭证或个人数据。

## 音乐与音效（music）

可复用的音乐与音效源码放在 `music/`。

- 记录来源、许可、采样率、位深、声道、转换命令与目标路径。
- 与当前 BSP 音频路径匹配时优先采用 16 kHz、16 位单声道 PCM。
- 嵌入音频前评估 Flash 与内部 RAM 成本；长录音应流式或分块。
- 无再分发许可不提交媒体文件。

- `images/road-rage-cover.png`：《狂飙骑手》的竖版 3:4 像素风封面，使用内置 imagegen 工具生成。图片标注为封面插画，不是实机截图。

- `images/cloudbound-cover.png`：1086 × 1448 RGB PNG，《云间一跃》的竖版 3:4 封面。2026-10-01 使用内置 imagegen 工具生成，已确认任务完成并查看实际上传文件。标注“示意图 · ILLUSTRATION”，不是实机截图，不嵌入固件。生成提示：精美像素画，戴青色围巾的白色方块角色跃过草顶浮岛，背景包含云海、晚霞、星空和极光，标题为“云间一跃 / CLOUDBOUND”，带示意图标注。

- `fonts/starport_gunner_noto_sc_subset.otf` 与 `fonts/starport_gunner_noto_sc_12.h`：星港炮手的 76 字 Noto Sans CJK SC 子集和 12px 位图字形（2280 字节），遵守 [SIL OFL](fonts/OFL.txt)。文案为 `main/games/starport_gunner/sg_copy.h`，生成／检查为 `python3 tools/games/starport_gunner/generate_font.py [--check]`，OTF 不嵌入固件。

- `fonts/alley_ninja_noto_sc_subset.otf` 和 `fonts/alley_ninja_noto_sc_12.h`：198 个 Noto Sans CJK SC/ASCII 字形，三档实际字号分别栅格化，4bpp 抗锯齿（压缩字模 171,094 字节，另加字形指标），遵循 [SIL OFL](fonts/OFL.txt)。为兼容保留 `_12` 文件名，现包含三档字号。用 `python3 tools/games/alley_ninja/generate_font.py [--check]` 生成/检查；扩展字符使用 `--source <完整字体.otf>`。OTF 不链接到固件。

- `images/alley-ninja-cover.png`：1086 × 1448 RGB PNG，《夜巷忍者》的竖版 3:4 封面。2026-10-01 使用内置 imagegen 工具生成，已确认任务完成并查看实际上传文件。标有示意图，不是实机截图，不嵌入固件。生成提示：精美像素画，青色蒙面忍者格挡一名紫金护甲敌人，雨夜霓虹街巷，标题“夜巷忍者 / ALLEY NINJA”，带示意图标注。

- `fonts/brick_workshop_noto_sc_subset.otf` 和 `fonts/brick_workshop_noto_sc_12.h`：173 个 Noto Sans CJK SC/ASCII 字形，三档实际字号分别栅格化，4bpp 抗锯齿（压缩字模 141,694 字节，另加字形指标），遵循 [SIL OFL](fonts/OFL.txt)。为兼容保留 `_12` 文件名，现包含三档字号。用 `python3 tools/games/brick_workshop/generate_font.py [--check]` 生成/检查；扩展字符使用 `--source <完整字体.otf>`。OTF 不链接到固件。

- `images/brick-workshop-cover.png`：弹砖工坊竖版 3:4 封面，1086 × 1448 RGB PNG。2026-10-01 使用内置 imagegen 工具生成，确认任务完成后复制并查看同一份上传文件。提示词：深蓝背景、粉/金/青/蓝玻璃砖、发光球、机械挡板与支架的精美像素工坊，中英文标题并明确标注示意图。属于宣传插画，并非实机截图。SHA256：`6eef5d2e821b85c2c6e7dec37b5c70485958712ca4c784b7c76e13e6fd561b98`。

- `fonts/rooftop_runner_noto_sc_subset.otf` 和 `fonts/rooftop_runner_noto_sc_12.h`：184 个 Noto Sans CJK SC/ASCII 字形，三档实际字号分别栅格化，4bpp 抗锯齿（压缩字模 155,869 字节，另加字形指标），遵循 [SIL OFL](fonts/OFL.txt)。为兼容保留 `_12` 文件名，现包含三档字号。用 `python3 tools/games/rooftop_runner/generate_font.py [--check]` 生成/检查；扩展字符使用 `--source <完整字体.otf>`。OTF 不链接到固件。

- `images/rooftop-runner-cover.png`：1086 × 1448 PNG，跃影疾行的竖版 3:4 封面。2026-10-01 使用内置 imagegen 工具生成，已确认任务完成，并查看实际复制的上传文件。标注中英文示意图字样，不是实机截图，不嵌入固件。提示词：精美像素风，青色衣服与金色围巾的跑者在日出城市屋顶间向右跳跃，发光奖励、检查点旗帜、少量屋顶障碍、中英文游戏标题和示意图标记。SHA256：`8f198098acad0b885f7e08d6acfa80ba00002b9f7f615ba99958312160b56d34`。

当前五款体验升级字体：狂飙骑手 113 字 / 3,390 字节；云间一跃 93 / 2,790；夜巷忍者 103 / 3,090；弹砖工坊 78 / 2,340；跃影疾行 90 / 2,700。新增勋章与能力文案后，使用有许可的完整 Noto 源字体重新生成。原创循环旋律与七类包络音效实现于 `main/games/common/game_audio.c`，没有使用录制音乐或外部音频素材，在原有的 160 样本工作缓冲中合成。


## 五款游戏代码绘图预览

骑手、云间、忍者、弹砖和跑酷五份 `images/*-preview.png` 为 960 × 1280 竖版组合图。使用各游戏原生 C 绘图代码及代表性模型状态同步生成，再用 Pillow 排版；已标注代码绘图预览、非实机截图。实际上传文件已逐一查看，封面单独标注示意图。

五款新版字体子集采用官方 [Noto CJK 仓库](https://github.com/notofonts/noto-cjk/tree/main/Sans/OTF/SimplifiedChinese) 的 Medium 字重，遵循 SIL OFL，不分发系统字体。
