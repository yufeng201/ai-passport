<p align="right"><a href="growth-package.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# Five-game discovery and onboarding package

This increment improves the path from discovering a game to trying it and learning its controls. It does not claim increased downloads before measurement.

## Gameplay onboarding

Stage-one hints follow the action without changing physics, difficulty, controls or saved progress. Road Rage explains lane changes and optional combat during the first six seconds. Cloudbound highlights a valid release while charging for the first three landings. Alley Ninja identifies guarding and the correct counterattack side for the first two opponents. Brick Workshop explains paddle movement before two bricks are destroyed. Rooftop Runner guides movement, edge release and airborne steering through its first two rooftops. Existing warnings and later-stage controls remain available.

## Promotional covers

The five new smooth non-pixel `assets/images/*-cover-v2.png` illustrations are the primary covers for the arcade and future community updates. They foreground characters, action, lighting and distinctive environments. All five exact files were visually inspected again on 2026-10-02; each is labeled as illustration artwork rather than a device screenshot. Do not use `*-preview.png` contact sheets or `*-promo.png` renderer layouts as the primary cover. Keep those as optional gameplay detail images. Older illustration covers are preserved. The arcade bundle explicitly includes only the five selected covers, even if older preview files remain in the build directory.

The five `assets/images/*-promo.png` files remain 960 x 1280 renderer previews with short gameplay hooks. Regenerate these optional detail images with:

```bash
python3 tools/games/render_promotion.py
```

The generator requires Pillow, a native C compiler and the licensed full Noto Sans CJK SC Medium font at `build/road_rage/NotoSansCJKsc-Medium.otf`. It waits for each native renderer to exit successfully before composing its cover. Inspect the exact output files after regeneration. Firmware fonts remain independent subsets.

## Portable arcade

```bash
python3 tools/games/build_arcade.py --build-games
python3 -m http.server 8780 --bind 127.0.0.1 --directory build/games/arcade
```

Open `http://127.0.0.1:8780/`. The arcade offers five games, clear controls, browser trials, return navigation and links to the five existing community projects. It copies only four public files per game and the five illustration covers. The output has no firmware, authorizations, tokens or build logs. It can be hosted as static files, but this increment does not deploy it. Browser progress lasts only for the current session; the community's approved firmware may differ from the preview. Sound is opt-in. Rounded artwork uses normal browser scaling rather than nearest-neighbor magnification.

The builder also produces `build/games/pocket-arcade.zip` from explicitly listed public files, excluding QA screenshots and build logs.

## Release and measurement

Keep the five existing community identities and engagement. Pending review versions must not be overwritten. New cover or firmware uploads require a new matching update grant after any conflict is resolved. The arcade does not submit updates or flash hardware.

After a public update, record daily increments in views and downloads over equivalent seven-day windows. They are event counts, not unique users or an attribution experiment. First validate cover clarity and onboarding, then vary one promotion element at a time. This package adds no hidden telemetry, manufactured engagement, public leaderboard or retention claims.

## Validation record (2026-10-02)

Build: PASS; Host tests: PASS; Device tests: NOT RUN.

All five complete gates, merged firmware archives, native/Wasm parity, desktop/mobile layouts and start/return navigation passed. No game device was identified; physical controls, sound and frame timing require acceptance. Browser QA used local Chrome with Playwright because the Browser plugin was not available.

| Game | Full image bytes | Full SHA-256 | ELF SHA-256 |
| --- | --- | --- | --- |
| road_rage | 651312 | `79d1d938acbc11a66207420e84a541718574f1f460bce189ec04540022f0d584` | `39dce7d63520bef3a2b5836c41a90d28b0b7d1f4d3ce25e3984fba47c8f22b87` |
| cloudbound | 628480 | `2b93348a2437a5b86f468bd9b810e88d60dbf7fe2322e5f9bba8df7dc13ead95` | `c9c8ce864d2912125e2bd6ea9558d80736de0543557fa7ead59454a42ce0411e` |
| alley_ninja | 642048 | `d3ca7edb3d8f96648e6c8687baad80303adaa69dd3dc71bb27bc250d3b8c6add` | `da0aa3c96ec8670bc355e5508f237da2d349f866ac201011a436368b65f8168a` |
| brick_workshop | 604880 | `0bf2c1241321c255705f0f46215efc3ea44c73e35dba3055770d3a8600bf0ff1` | `d2e267e2477c88fc29cd349b52968b25387fcc32d680fb924d8d533bd0eed96f` |
| rooftop_runner | 618368 | `94568649c4f3c41d001299821944553789d41c4a7198121466e158829e8b5ba5` | `0dc1c1534b5b5033505ed134c1b849e35c9926b9a87f1444949a49d904f5c1b9` |

Each archive is under `build/firmware/<Full SHA-256>/`, containing the merged image for `0x0` and its matching ELF. This increment has not been flashed, deployed or published.
