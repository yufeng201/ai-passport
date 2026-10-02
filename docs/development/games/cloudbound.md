<p align="right"><a href="cloudbound.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# Cloudbound

An offline landscape charge-and-jump game. Hold the middle B button to build power and release to leap to the next floating island. Center landings build a perfect combo; edge landings still continue. Five stages travel through mist, clear skies, sunset, night, and aurora, increasing distance variation, narrowing platforms, and introducing height changes. Complete 12 landings to advance.

## Getting Started

1. Power on and hold the device horizontally. No network setup is needed. Tap B on the title screen to start; A/C selects unlocked stages.
2. While standing on a platform, hold B to charge and release to jump. Maximum charge stays capped without launching automatically. Stage one shows trajectory and landing guidance; later stages require judging distance.
3. Wait for flight and camera movement to finish before holding B again. Hold C for about 0.8 seconds to pause, tap B to resume, or hold A while paused to return to the title.
4. Tap B after success to advance, or after failure to retry the current stage. Hold B on results to return to the title; hold A on the title to toggle sound. Firmware saves unlocks, the best single-stage score, and sound preference. Unavailable storage still allows play, but records then last only until power-off.

Use one button at a time. Start, resume, and retry trigger on release without double-click delay. Releasing after a long action does not trigger a short action. Pause cancels an unreleased charge to prevent an accidental jump on resume; pausing during flight retains its trajectory and position.

## Stages and Rules

| Stage | Scene | Change |
| --- | --- | --- |
| Morning Mist | Blue mist, clouds, sunlight | Wide platforms, limited distance variation, full trajectory guide |
| Clear Skies | Cyan-blue sky | Greater distance variation, no trajectory guide |
| Sunset Crossing | Purple-pink sky, golden sunset | Narrower platforms |
| Moonlit Heights | Deep-blue stars, crescent moon | Higher and lower platforms |
| Aurora Journey | Green aurora, stars | Narrowest platforms combined with height changes |

Each landing scores 100 points. Landing within 8 px of center extends the perfect combo and adds the combo count multiplied by 20 points. An ordinary landing resets the combo. Failure retries only the current stage, preserving unlocks and best score. After all stages, retry stage five or return to the title to select a stage.

Charge maps 100–1,200 ms to a deterministic horizontal distance. Flight follows a fixed 600 ms arc with no randomly moving platforms or hidden wind. Every jump requires a fresh press; input during flight never queues a later jump. The camera moves smoothly for 300 ms after landing. Platform generation uses the same landing function and bounds that retain at least a 150 ms successful charge window.

## Directories and Build

- `main/games/cloudbound/`: independent model, Chinese copy, RGB565 strip renderer, device entry, browser bridge.
- `main/games/common/`: runtime and synthesized audio; pure gameplay does not depend on BSP or ESP-IDF.
- `tools/games/cloudbound/`: font coverage, browser build, parity, and offline image generation.
- `tests/games/test_cloudbound.c`: edge timing, timer wrap, reachability, 64 complete five-stage simulations, pause/retry, rendering bounds.

Activate ESP-IDF 5.5.3 and explicitly choose the game:

```bash
PASSPORT_GAME=cloudbound ./tools/validate.sh
```

The default remains `road_rage`. Merged firmware and matching ELF are archived under `build/firmware/<sha256>/`. Flash the merged file from that exact run's archive; builds of different games replace the common delivery file, so its name alone does not identify its contents. USB flashing and community submission require their respective authorization.

## Browser Preview

```bash
python3 tools/games/cloudbound/build_preview.py
./tools/games/cloudbound/check.sh
python3 -m http.server 8766 --bind 127.0.0.1 --directory build/games/cloudbound/preview
```

Open the [local preview](http://127.0.0.1:8766). B/Space/Enter charges; A/Left and C/Right match the outer physical buttons. Page buttons can also be held. Focus loss or pointer cancellation pauses and clears old input. Audio defaults off. The browser shares the C model and rendering, but refresh clears webpage progress; firmware uses device storage. On macOS, set a matching `SDKROOT` for this check if needed without changing system defaults.

## Resources and Acceptance

The model occupies 164 bytes. Device rendering uses two 320 × 40 RGB565 DMA buffers totaling 50 KiB, with no device full-screen buffer or LVGL, Wi-Fi, or BLE initialization. Chinese UI uses 93 glyphs at 12 px, totaling 2,790 bytes of Flash records. The OTF subset is only a generation source and is not linked. Fonts use Noto Sans CJK SC under SIL OFL; ordinary builds need no font-conversion dependencies.

Button callbacks only enqueue. The owner handles logic, storage, and display; audio has an independent task. Input overflow pauses the game. Display transfer failure stops and retains in-flight DMA memory. Storage initialization/write failures fall back to session progress without automatically erasing NVS; audio failures permit silent play.

Host checks cover 320 complete stages, 3,840 landings, and valid charge windows for each platform. Visual review covers title, play, pause, success, and failure across five stages. Native C and Wasm compare state hashes and RGB565 images for identical replay; this does not establish browser interaction or hardware acceptance. Targets are at least 25 FPS, 30 KiB free internal heap, and a 16 KiB largest free block. Physical orientation, Chinese readability, input feel, sound, power-cycle persistence, and busy-scene performance require device checks.

## Validation Record (2026-10-01)

- Build: PASS — complete gate with `PASSPORT_GAME=cloudbound`.
- Host tests: PASS — five-stage campaigns, queued-edge timing, geometry, strips, native/Wasm parity, and undefined-behavior sanitizer.
- Device tests: NOT RUN — USB flashing awaits authorization for this firmware.
- Unverified: actual browser interaction (automation policy check unavailable), physical orientation/readability/input/audio, power-cycle persistence, busy-scene frame rate and heap.

Verified archive: `build/firmware/1e752c18379241b1ee85ddc66734b16ce5ab693ab24fbcf8bf092b90100abaf9/`. Merged image: 442,944 bytes.

```text
full SHA256: 1e752c18379241b1ee85ddc66734b16ce5ab693ab24fbcf8bf092b90100abaf9
ELF SHA256:  cb7e93c97b6c77ca38abbbfb9b0a9642a21313f43ee48bbe1fa2b30069284dd2
```


## Current five-game upgrade

Added one rescue per attempt, precision landing markers, three medal goals and persistent medals. Added gentle looping music, precision cues and reward particles; B’s charge-and-release controls are retained.

1. Hold the device horizontally; no network setup is needed. Tap B on the title to start; A/C select an unlocked course.
2. Hold B to charge and release to jump forward. Judge the gap and aim for the next island’s glowing center to earn consecutive precision bonuses.
3. After a miss, tap A for one rescue, retaining progress and score but clearing the combo. Tap B to restart.
4. Hold C to pause, tap B to resume, and hold A while paused to return to the title. Hold A on the title to toggle sound.
5. Complete twelve landings and tap B for the next course; the fifth can be replayed. Unlocks, best score and stage medals save automatically.

Current artifact identities and validation limits are recorded in [the batch report](quality-upgrade.md). Older build and device records below describe their original revisions.
