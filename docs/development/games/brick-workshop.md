<p align="right"><a href="brick-workshop.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# Brick Workshop

An offline landscape brick-breaking game. Move the paddle with A/C, catch the glowing ball and clear five colorful workshops. The contact point changes the rebound direction. Durable bricks, a central bumper and moving rows gradually add difficulty; the optional slow field gives extra time to reposition.

## Start playing

1. Hold the device horizontally; no network setup is required. Tap B on the title screen to start. A/C select unlocked stages.
2. Hold A to move left or C to move right; release to stop. Tap B to launch the waiting ball. Catch it with the paddle and aim rebounds by changing the contact point.
3. Tap B during flight for a three-second slow field, available twice per stage. Only one button is used at a time. Every stage can be cleared without the optional field.
4. Hold B for about 0.8 seconds to pause; tap B to resume. Hold A while paused to return to the title. A missed ball costs one of three lives; move into position and tap B to launch again.
5. Tap B after a stage to advance, or after defeat to retry. Stage five offers a replay. Hold B on results to return to the title. Hold A on the title to toggle sound. Unlocked stages, best stage score and sound setting are saved; storage failures leave session-only progress.

## Five stages

| Stage | Layout | Challenge |
| --- | --- | --- |
| First light | 12 bricks, two rows | Learn paddle steering |
| Double glass | 18 bricks, three rows | Some bricks need two hits; faster ball |
| Rebound frame | 18 bricks and a central bumper | Read redirected bounces |
| Flowing windows | 18 bricks in moving rows | Track lateral motion and a faster ball |
| Spectrum workshop | 24 bricks, moving rows and bumper | Mixed one-, two- and three-hit bricks |

Each successful brick hit awards 50 points. Missing a ball preserves cleared bricks and score but ends the active slow field. Retrying resets that stage's board, score, three lives and two slow-field charges. The ball always waits for an explicit launch. After six seconds without a brick hit, a wall, ceiling or paddle rebound steers toward a surviving brick; the ball never teleports, and bumper rebounds do not receive this assist. A visible guidance message explains the correction.

## Implementation and checks

The independent `main/games/brick_workshop/` module contains deterministic gameplay, Chinese copy, the strip renderer, device adapter, build manifest and browser bridge. Only the selected game enters firmware; the default remains Road Rage. State is 532 bytes with no gameplay allocations. Shared runtime uses two 320 × 40 RGB565 DMA strips (50 KiB total) without a device full-screen framebuffer or Wi-Fi/BLE/LVGL initialization. NVS namespace is `brick_workshop`.

The Chinese font contains 78 Noto Sans CJK SC glyphs, 2,340 bytes of Flash records, under SIL OFL. The source subset is retained for conversion and excluded from firmware. Font tools require Pillow 12.2.0 and fonttools 4.62.1; extending the copy requires `generate_font.py --source <full-source-font.otf>`.

Simulation runs at 20 ms with eight fixed-point microsteps per tick. Paddle collisions additionally test the swept top crossing. Moving-brick overlap is resolved onto the nearest face; a brick loses at most one health per tick. Release stops paddle movement immediately. Pause freezes ball, moving rows, slow field and feedback, clears held inputs, and suppresses a pause key's release action. Stale clocks are ignored and timestamp wrap is covered.

```bash
PASSPORT_GAME=brick_workshop ./tools/validate.sh
python3 tools/games/brick_workshop/build_preview.py
./tools/games/brick_workshop/check.sh
python3 -m http.server 8769 --bind 127.0.0.1 --directory build/games/brick_workshop/preview
```

Activate ESP-IDF 5.5.3 for firmware builds. Browser controls are A/left, C/right, B/space/enter and pointer buttons. Blur or canceled pointers pause; sound starts off and browser progress is session-only. Browser framebuffer memory is excluded from device builds. Rendered PNG files under `build/games/brick_workshop/` show actual drawing code, not hardware screenshots.

Tests cover 60 complete stages across 12 seeds with legal single-button input and no slow-field use, high-speed paddle crossing, misses, wall/brick/bumper reflection, armor, moving-brick overlap, pause, timestamps, unlocks and replay transitions. Five strip sizes are compared against complete rendering across 25 scenes with memory guards. Native/Wasm replay compares 21 state and RGB565-frame checkpoints over 2,000 steps. Hardware acceptance remains separate.

## Validation record (2026-10-01)

- Build: PASS, complete `PASSPORT_GAME=brick_workshop ./tools/validate.sh` gate, merge and verified archive.
- Host tests: PASS, 60 complete stages, collision/input/strip regressions, 21 native/Wasm checkpoints and undefined-behavior sanitizer.
- Device tests: NOT RUN; no game device was detected.
- Unverified: real browser interaction, device orientation/readability/button feel/sound, power-cycle persistence, busy-scene frame rate and free heap.

Verified archive: `build/firmware/8c47f73682e58ca55b7b3e3771f00795ccb2f739b7301a7eb000745ec6c090c5/`. Merged size: 441,328 bytes, flashing from `0x0`. Use this merged image and its matching archived ELF. Merged flashing can reset stored records and requires specific image/port consent. Target performance remains 25 FPS, 30 KiB free internal heap and a 16 KiB largest block; these are targets, not hardware measurements.

```text
full SHA256: 8c47f73682e58ca55b7b3e3771f00795ccb2f739b7301a7eb000745ec6c090c5
ELF SHA256:  04ee3bc80ca307c74d8234d5f4e454432eb2cf22b920e7bbd907759af1288e5d
```


## Current five-game upgrade

Added slow-ability refills after six destroyed bricks, a supply meter and a three-use cap. Added break particles, supply cues, electronic looping music and persistent best stage medals.

1. Hold the device horizontally; no network setup is needed. Tap B on the title to start; A/C select an unlocked workshop.
2. Hold A/C to move the paddle left/right and tap B to launch. Catch the falling ball and bounce it into the bricks; misses cost lives.
3. Tap B while the ball is flying to activate a slow field. Every six destroyed bricks replenish one use, capped at three; watch the supply meter.
4. Hold B to pause, tap B to resume, and hold A while paused to return to the title. Hold A on the title to toggle sound.
5. Clear the wall and tap B for the next workshop, or after defeat to retry. The fifth can be replayed. Unlocks, best score and stage medals save automatically.

Current artifact identities and validation limits are recorded in [the batch report](quality-upgrade.md). Older build and device records below describe their original revisions.


## Smooth illustration rendering

Typography now uses Noto Sans Chinese and Latin glyphs rendered separately at each native size, with 4-bit coverage blended into RGB565. Shapes use clipped analytic strokes/ellipses with four coverage samples. Rounded UI panels, curved character silhouettes and vector medals replace deliberately enlarged pixel artwork; Road Rage no longer uses its bitmap motorcycle sprite. No supersampled framebuffer or new per-frame allocation is introduced. Physical resolution and real-device frame time still need acceptance testing. Browser Wasm memory is 512 KiB to accommodate immutable font data and its full preview frame; that does not change device strip memory. Current builds use `smooth-gate.log` and `smooth-receipt.json`; older upgrade receipts describe the prior graphics.
