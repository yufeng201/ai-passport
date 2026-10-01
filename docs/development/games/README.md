<p align="right"><a href="README.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# Games

Each game owns its model, rendering, copy, controls, browser bridge, and device entry. Common code contains capabilities shared independently of game rules. Reference hardware demos remain outside this tree.

## Directory Layout

```text
main/
  main.c                         selected game's startup
  CMakeLists.txt                 selection and component registration
  games/
    common/                      reusable game capabilities
      game_audio.c/.h            bounded synthesized audio
      game_runtime.c/.h          display, input, audio, progress storage
    cloudbound/                  charge-and-jump game and manifest
    road_rage/
      game.cmake                 firmware source/dependency manifest
      rr_game.c/.h               pure gameplay and geometry
      rr_render.c/.h             strip renderer
      rr_controls.h              short/long press normalization
      rr_copy.h                  Chinese UI inventory
      rr_device.c                BSP adapter and runtime
      rr_wasm.c                  browser-only bridge

tools/games/road_rage/            preview, fonts, parity, offline rendering
tests/games/test_road_rage.c      host regression tests
assets/fonts/                    licensed fonts and generated glyphs
assets/images/                   covers and reusable images
docs/development/games/          designs, manuals, and acceptance
build/games/<game>/               ignored preview and development outputs
build/firmware/<sha256>/         verified delivery/debug bundles
```

Road Rage and Cloudbound have independent build manifests. New games will use separate `starport_gunner/`, `alley_ninja/`, `brick_workshop/`, and `clockwork_maze/` directories, with matching tools and tests when implemented. Do not register unfinished games or allocate their state in another game's firmware. Add common abstractions when actual use justifies them; game-specific timing and rules stay in their own modules.

## Build Selection

The default remains Road Rage. The gate accepts a game name through `PASSPORT_GAME` and passes it explicitly to CMake:

```bash
PASSPORT_GAME=road_rage ./tools/validate.sh
```

Activate ESP-IDF 5.5.3 first. An unknown or malformed name fails configuration instead of silently building another game. Direct IDF builds may select it with `idf.py -D PASSPORT_GAME=road_rage build`; use the full gate for delivery. A manifest defines `GAME_ENTRY`, `GAME_SOURCES`, `GAME_INCLUDE_DIRS`, and `GAME_REQUIRES`. Shared sources are linked only when the selected manifest needs them; browser framebuffers and other games do not enter the device source list.

Run `./tools/validate.sh --static` for repository/host checks, and `./tools/games/road_rage/check.sh` for the focused native/Wasm check after rebuilding the preview. Tests do not establish physical screen or button acceptance. Generated firmware, screenshots, logs, and publishing drafts remain under ignored `build/`; publisher credentials must stay outside the repository.

## Games and Plans

- [Cloudbound](cloudbound.md): five-stage charge-and-jump game.
- [Road Rage](road-rage.md): delivered motorcycle racing and combat game.
- [Five new game designs](design.md): development order, controls, art, difficulty, and acceptance targets.
