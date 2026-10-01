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

- `fonts/road_rage_noto_sc_subset.otf`：[Noto Sans CJK SC Regular](https://github.com/notofonts/noto-cjk/tree/main/Sans) 的 108 字子集，使用 [SIL OFL 1.1](fonts/OFL.txt) 许可。保留用于重现转换，不链接进固件。
- `fonts/road_rage_noto_sc_12.h`：生成的 12px 单色中文字形，每格 12 × 14，Flash 字形记录约 3 KiB。共享游戏渲染器直接使用，无 LVGL 字体池或运行时解码。文案和字形覆盖检查：`python3 tools/games/road_rage/generate_font.py --check`；重生成：`python3 tools/games/road_rage/generate_font.py`（Pillow 12.2.0）。字符清单为 `main/games/road_rage/rr_copy.h`。新增用字需用 `--source <完整源字体.otf>` 重建 OTF 子集（fonttools 4.62.1），然后重新构建预览与固件。

- `fonts/cloudbound_noto_sc_subset.otf` 与 `fonts/cloudbound_noto_sc_12.h`：云间一跃的 88 字 Noto Sans CJK SC 子集和 12px 位图字形，Flash 字形记录 2,640 字节，遵守 [SIL OFL](fonts/OFL.txt)。文案清单为 `main/games/cloudbound/cb_copy.h`；生成／检查：`python3 tools/games/cloudbound/generate_font.py [--check]`。重建子集需要 `--source <完整源字体.otf>`、Pillow 12.2.0 与 fonttools 4.62.1。OTF 不链接到固件。

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
