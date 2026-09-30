<p align="right"><a href="dusk-riders.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# Road Rage

A standalone, offline motorcycle combat racer for AI Passport: a 320 × 240 landscape sunset coast, three lanes, automatic acceleration, traffic avoidance and close-range attacks. The game boots directly into its own title screen. The original test menu is not part of the application. No Wi-Fi, BLE, account or mini-program connection is required.

## Play

Finish each stage without losing all armor. Five stages unlock in order, with traffic gradually increasing. Attack is optional; each stage has an optional bonus challenge. Motorcycles can be struck when the Chinese attack marker appears above an adjacent rider within reach; cars must be dodged. The marker is hidden during cooldown, with a B ready/cooling indicator and recovery bar at the top right. Collision uses the projected sprite rectangles, including intermediate lane positions, and foreground traffic occludes the player according to depth. Knocking out a rider earns 120 points; passing earns 40. Reaching the finish awards five points per remaining armor unit. Position improves with passes and knockouts, starting at 8/8. Collision removes 20 armor for motorcycles or 30 for cars, slows the player and grants a 1.2-second damage grace period. Damage takes precedence over finishing in the same tick.

| Stage | Distance | Maximum speed | Spawn interval + random 0..399 ms | Car share | Optional bonus |
| --- | --- | --- | --- | --- | --- |
| Coast practice | 1.6 km | 155 km/h | 1,400 ms | 20% | Pass 5 vehicles |
| City chase | 1.8 km | 165 km/h | 1,250 ms | 20% | Knock out 3 riders |
| Desert traffic | 2.0 km | 175 km/h | 1,100 ms | 30% | Finish with 50 armor |
| Rainy mountain | 2.2 km | 185 km/h | 950 ms | 30% | Finish within 80 seconds |
| Final showdown | 2.4 km | 195 km/h | 800 ms | 40% | Finish in the top 3 |

Collision damage and lane response remain consistent across stages. Finishing earns one star, the bonus earns another, and finishing with at least 80 armor earns the third. Bonus completion is never required to advance. Each new stage/retry restores armor. Unlock progress survives return to title but is currently session-only (reset on power cycle).

| Physical key | Short action | Long action (500 ms) |
| --- | --- | --- |
| UP / A (outer key) | Left lane / previous unlocked stage | Return to title when not racing |
| DOWN / B (middle key) | Start, optional strike, resume, next stage or retry on RELEASE | Pause/resume; no short action on release |
| OK / C (outer key) | Right lane / next unlocked stage on PRESS | None |

The two outer keys handle the frequent movement actions. Controls use one key at a time. B attacks immediately on release without waiting for single/double-click classification; repeated taps still respect attack cooldown. The game times press/release edges to emit a long pause once without attacking on release. The title includes the long-B pause hint. Check landscape direction on the actual board; change the mirror direction in `main/road_rage/rr_device.c` if held the opposite way up. The browser maps Left/A, Right/C (D also supported), and B/Space/Enter to the same actions. Pointer cancel/focus loss releases input; focus loss pauses play. Accessible buttons perform their labeled short actions. Audio is off by default in the browser and can be enabled with its audio button; firmware plays quiet action cues when the codec is available.

## Files and preview

- `main/road_rage/`: portable C model, integer RGB565 renderer, Chinese copy, synthesized sounds, Wasm bridge and ESP-IDF device adapter.
- `tools/road_rage/preview/`: browser shell with no separate gameplay model.
- `assets/fonts/`: licensed subset font and generated monochrome glyph atlas. English decorative text uses an original 5 × 7 bitmap alphabet.
- `tests/test_road_rage.c`: state transitions, 16 complete five-stage campaigns without attacks, release/repeated-tap/long-hold limits, visible-contact and attack-cue regressions, strip boundaries, full/strip render equality and seeded replay.

Build a standalone Wasm module with a wasm32-capable clang (tested with wasi-sdk 34.0; the build script discovers `build/toolchains/wasi-sdk-*/bin/clang` or accepts `WASI_CLANG`):

```bash
python3 tools/road_rage/build_preview.py
./tools/road_rage/check.sh
python3 -m http.server 8765 --bind 127.0.0.1 --directory build/road_rage/preview
```

Open [the local preview](http://127.0.0.1:8765). The build generates a manifest binding the C sources, font assets, shell and Wasm bytes; `--check` rejects stale files. `check.sh` requires a native C compiler, Python 3 and Node.js (tested with Node 22). It compares 21 logical-state and RGB565 frame checkpoints over 1,400 identical native/Wasm steps. Preview output stays under ignored `build/`. On a macOS host with incompatible default SDK/linker versions, activate a matching Xcode SDK via `SDKROOT` for native checks; do not change system defaults.

## Device budget and build

The device uses two 320 × 40 RGB565 DMA strips (50 KiB total), six fixed traffic slots, a small state structure and a 320-byte audio chunk. It does not allocate a full-screen framebuffer or initialize LVGL, Wi-Fi or BLE. The browser-only 150 KiB framebuffer is excluded from firmware. Chinese font data remains in Flash. The hot renderer/model are compiled with O2; off-strip text/sprites/lines are culled before rasterization. A panel-completion semaphore protects each strip from being overwritten in flight, while rendering the other overlaps transfer; a transfer failure stops rendering rather than reusing DMA memory. Rendering/copying and battery polls run outside button callbacks; audio has its own worker. Missing battery readings show an unavailable marker, and codec failures degrade to silent gameplay.

Activate ESP-IDF 5.5.3 and run `./tools/validate.sh`. The gate includes the game host tests and glyph coverage, builds firmware and retains a merged `build/FoloToy-AI-Passport-full.bin` plus its matching debug archive. Font regeneration needs Pillow 12.2.0; only rebuilding the OTF subset also requires fonttools 4.62.1. No font-generation dependency is needed for normal builds.

## Acceptance contract

| Scenario | Expected behavior | Evidence |
| --- | --- | --- |
| Title/start | Chinese title, legible controls, one B click starts | Host transition and visual review; physical key test |
| Three lanes | One PRESS moves one lane, bounded edges, smooth visual movement | Host limits; browser and physical input test |
| Strike/collision | Adjacent attack marker, visible knockdown, cooldown; cars cannot be struck | Controlled host fixture and normal play |
| Pause/interruption | Time stops; no held input after cancel; B resumes; paused long A returns | Host freeze test; browser focus/pointer test |
| Finish/failure/replay | Persistent result, correct score, B advances or retries | Controlled fixture plus one complete normal run |
| Chinese/battery | All copy has glyphs; unavailable and late battery readings remain legible | Atlas coverage; native-resolution review; board check |
| Performance | Target at least 25 presented frames/s; no recurring stalls or watchdogs | Warm up 10 s, then sample 60 s of busy racing on device |
| Memory/input | Target at least 30 KiB free internal heap and 16 KiB largest block; lane response under 100 ms | Board logs and physical timing; B click separately includes release timing |

Performance and memory values are acceptance targets, not measured results. Device logs emit render/transfer time and heap probes every five seconds. Browser counters and offline frame images do not establish physical fluidity. Installing firmware and accepting browser gameplay are separate steps; flashing requires the user's current approval. Merged flashing at 0x0 may reset stored data; follow the repository firmware-layout policy for data-preserving installation. No original-firmware backup is required.
