<p align="right"><strong>简体中文</strong> · <a href="growth-package.md">English</a></p>

# 五款游戏展示与上手优化

本轮优化从发现游戏、试玩到学会操作的过程。在获得数据前，不声称下载量已经提升。

## 游戏内上手提示

第一关提示随动作变化，不改变物理、难度、按键和已有进度。骑手在前六秒解释换道与可选攻击；云间在前三次落地前蓄力时标示合适的松手窗口；忍者在前两名敌人中提示格挡和正确反击方向；弹砖在击碎两砖前提示移动接球；跑酷在前两段屋顶提示移动、边缘松手与空中调整。保留已有警告和后续关卡操作。

## 宣传封面

试玩入口和后续社区更新统一使用新生成的五张非像素 `assets/images/*-cover-v2.png` 插画作为主封面，突出主角、动作、光影和各自的场景。2026-10-02 已重新打开检查五份实际文件，均标注为插画示意而非实机截图。不要把 `*-preview.png` 拼图或 `*-promo.png` 绘图排版用作主封面；它们只作为可选玩法附图。保留旧插画封面。试玩打包只明确收录五张选定封面，即使构建目录还留有旧预览图也不会混入。

五份 `assets/images/*-promo.png` 保留为 960 x 1280 玩法绘图预览及简短操作介绍。重新生成这些可选附图：

```bash
python3 tools/games/render_promotion.py
```

生成器需要 Pillow、原生 C 编译器和有许可的完整 Noto Sans CJK SC Medium 字体，路径为 `build/road_rage/NotoSansCJKsc-Medium.otf`。先等待每个原生渲染程序成功退出，再排版封面。重新生成后要查看实际输出文件。固件字体仍使用独立子集。

## 可移植试玩入口

```bash
python3 tools/games/build_arcade.py --build-games
python3 -m http.server 8780 --bind 127.0.0.1 --directory build/games/arcade
```

打开 `http://127.0.0.1:8780/`。入口包含五款游戏、清晰操作、浏览器试玩、返回导航和已有社区项目链接。每款只复制四份公开试玩文件及五张插画封面；产物没有固件、授权、凭据或构建日志，可作为静态文件托管，本轮尚未部署。浏览器进度只保留本次会话；社区已审核固件可能与试玩版本不同。声音由玩家主动开启。圆润画面使用浏览器正常缩放，不再刻意放大像素。

构建同时生成 `build/games/pocket-arcade.zip`，仅含明确列出的公开文件，不包含 QA 截图或构建日志。

## 发布与效果衡量

保留五个已有社区项目及下载、点赞。有待审核版本时不能覆盖；冲突解决后，上传新封面或固件需要对应的新更新授权。试玩入口不会提交版本或烧录设备。

公开更新后，按相同七天窗口记录浏览和下载每日增量。它们是事件次数，不代表独立用户或归因实验。先验证封面和引导清晰，再一次调整一项宣传因素。本轮没有添加隐藏统计、虚构互动、公开排行榜，也不声称已验证留存。

## 验证记录（2026-10-02）

Build: PASS; Host tests: PASS; Device tests: NOT RUN.

五款完整门禁、合并固件归档、原生/Wasm 一致性、桌面与手机布局，以及开始/返回导航均通过。当前未识别到游戏设备；实体按键手感、声音和帧率仍需验收。浏览器验证使用本机 Chrome 与 Playwright。

| Game | Full image bytes | Full SHA-256 | ELF SHA-256 |
| --- | --- | --- | --- |
| road_rage | 651312 | `79d1d938acbc11a66207420e84a541718574f1f460bce189ec04540022f0d584` | `39dce7d63520bef3a2b5836c41a90d28b0b7d1f4d3ce25e3984fba47c8f22b87` |
| cloudbound | 628480 | `2b93348a2437a5b86f468bd9b810e88d60dbf7fe2322e5f9bba8df7dc13ead95` | `c9c8ce864d2912125e2bd6ea9558d80736de0543557fa7ead59454a42ce0411e` |
| alley_ninja | 642048 | `d3ca7edb3d8f96648e6c8687baad80303adaa69dd3dc71bb27bc250d3b8c6add` | `da0aa3c96ec8670bc355e5508f237da2d349f866ac201011a436368b65f8168a` |
| brick_workshop | 604880 | `0bf2c1241321c255705f0f46215efc3ea44c73e35dba3055770d3a8600bf0ff1` | `d2e267e2477c88fc29cd349b52968b25387fcc32d680fb924d8d533bd0eed96f` |
| rooftop_runner | 618368 | `94568649c4f3c41d001299821944553789d41c4a7198121466e158829e8b5ba5` | `0dc1c1534b5b5033505ed134c1b849e35c9926b9a87f1444949a49d904f5c1b9` |

各归档位于 `build/firmware/<Full SHA-256>/`，内含用于 `0x0` 的完整镜像及匹配 ELF。本轮未烧录、部署或发布。
