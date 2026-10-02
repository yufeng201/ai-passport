<p align="right"><strong>简体中文</strong> · <a href="README.md">English</a></p>

# 游戏

每款游戏独立维护玩法、绘图、文案、按键、浏览器桥接和设备入口。公共目录存放不依赖具体游戏规则的可复用能力。原有硬件测试 demo 保留在这套目录之外。

## 目录布局

```text
main/
  main.c                         启动选中的游戏
  CMakeLists.txt                 游戏选择与组件注册
  games/
    common/                      游戏公共能力
      game_audio.c/.h            有界合成音效
      game_runtime.c/.h          显示、输入、音频与进度保存
    rooftop_runner/              A 前进、B 后退、C 跳跃的跑酷游戏
    brick_workshop/              挡板反弹与彩砖游戏
    alley_ninja/                 格挡反击动作游戏
    starport_gunner/              防守射击游戏与构建清单
    cloudbound/                  蓄力跳跃游戏与构建清单
    road_rage/
      game.cmake                 固件源码与依赖清单
      rr_game.c/.h               纯玩法与几何计算
      rr_render.c/.h             分块绘图
      rr_controls.h              长短按处理
      rr_copy.h                  中文界面文案清单
      rr_device.c                BSP 适配与运行循环
      rr_wasm.c                  仅浏览器使用的桥接

tools/games/road_rage/            预览、字体、一致性检查、离线绘图
tests/games/test_road_rage.c      主机回归测试
assets/fonts/                    有许可的字体和生成字形
assets/images/                   封面与可复用图片
docs/development/games/          设计、使用说明与验收
build/games/<game>/               已忽略的预览与开发产物
build/firmware/<sha256>/         已校验的固件和调试归档
```

狂飙骑手、云间一跃、星港炮手、夜巷忍者与弹砖工坊各自拥有独立构建清单。剩余机关迷城计划使用独立的 `clockwork_maze/` 目录，完成时增加对应工具和测试。未完成的游戏不注册构建，也不在另一款固件中分配状态。公共抽象由实际复用需求推动，具体计时规则和玩法留在各自模块。

## 选择构建

默认仍是狂飙骑手。统一验证入口通过 `PASSPORT_GAME` 接收游戏名，并明确传给 CMake：

```bash
PASSPORT_GAME=road_rage ./tools/validate.sh
```

先激活 ESP-IDF 5.5.3。未知或非法名字会在配置阶段失败，不会悄悄构建另一款游戏。直接 IDF 构建可使用 `idf.py -D PASSPORT_GAME=road_rage build`，正式交付仍使用完整门禁。各款 `game.cmake` 定义 `GAME_ENTRY`、`GAME_SOURCES`、`GAME_INCLUDE_DIRS`、`GAME_REQUIRES`。仅链接该游戏实际需要的公共源码；浏览器整帧缓冲和其他游戏不进入设备源码列表。

`./tools/validate.sh --static` 检查仓库和主机逻辑；重新构建预览后，`./tools/games/road_rage/check.sh` 检查原生 C 与 Wasm 一致性。这些检查不能代替实机屏幕和按键验收。生成固件、截图、日志、发布草稿放在已忽略的 `build/`，发布凭据必须保存在仓库外。

## 游戏与计划

- [云间一跃](cloudbound.zh_CN.md)：五关蓄力跳跃游戏。
- [狂飙骑手](road-rage.zh_CN.md)：已交付的摩托竞速与对抗游戏。
- [五款新游戏设计](design.zh_CN.md)：开发顺序、按键、画风、难度与验收目标。
- [星港炮手](starport-gunner.zh_CN.md)：五关防守射击游戏。
- [夜巷忍者](alley-ninja.zh_CN.md)：五关格挡反击动作游戏。

- [弹砖工坊](brick-workshop.zh_CN.md)：五关挡板反弹与彩砖挑战。

- [跃影疾行](rooftop-runner.zh_CN.md)：A 前进、B 后退、C 跳跃的五关屋顶跑酷。

体验升级分析与验收标准：[五款游戏升级](quality-upgrade.zh_CN.md)。

- [熔岩升梯](lava-lift.zh_CN.md)：移动上升平台穿过岩壁，跳跃躲避岩浆小怪。
