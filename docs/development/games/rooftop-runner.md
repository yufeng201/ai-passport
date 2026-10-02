<p align="right"><a href="rooftop-runner.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# Rooftop Runner

An offline landscape platform runner with the creator's requested physical controls: A moves forward, B moves backward and C jumps. Traverse rooftops, collect glowing tokens and reach the finish. Five courses add height changes, spikes and moving saws, with rooftop checkpoints for quick recovery.

## Start playing

1. Hold the device horizontally; no network setup is needed. Tap B on the title screen to start. A/C select unlocked courses.
2. Hold A to run forward or B to run backward. Release to stop immediately on the ground. Move toward a roof's edge, release the movement key and promptly tap C to jump in that direction. After standing still, C jumps vertically.
3. Airborne movement continues until landing, and A/B can adjust its direction. Landing stops horizontal movement when no direction key is held. Simultaneous button presses are never required.
4. Hold C for about 0.8 seconds to pause; tap B to resume. Hold A while paused to return to the title. Missing a roof or touching a hazard automatically returns you to the latest checkpoint, preserving collected tokens and the current score.
5. Tap B after finishing to advance; the fifth course offers a replay. Hold B on results to return to the title. Hold A on the title to toggle sound. Unlocked courses, best course score and sound setting are saved.

Checkpoint position and collected tokens last for the current attempt only; returning to the title and starting again resets that course. Storage errors retain unlocks and records for the session without erasing existing data. Stages can be replayed freely; there is no lives limit or penalty that prevents completion.

## Five courses

Each course contains 12 fixed roofs. Widths shrink and gaps widen with each stage. The first roof is wider for learning; roofs 1, 4, 7 and 10 provide checkpoints. Flags mark checkpoints and the finish.

| Course | Terrain | Obstacles |
| --- | --- | --- |
| Morning rooftop | Level roofs, 30/36px gaps | None |
| Varied district | Roof heights vary by up to 16px, 36/42px gaps | Visible spikes |
| Sharp-corner trial | Narrower roofs, 42/48px gaps | Spikes and tighter jump placement |
| Moving mechanisms | 48/54px gaps | Moving saws |
| Sunrise corridor | Narrowest roofs, 54/60px gaps | Moving saws and combined height changes |

Direction switching uses one ADC button at a time. A released movement key arms its direction for a C press within 350ms. C release launches one fixed jump; holding C pauses and never also jumps on release. Leaving a roof edge allows a 100ms late-jump window. A short C release within 100ms before touchdown is buffered for one jump on landing; it cannot jump again in mid-air. Pausing or respawning clears both aids. The first two courses mark a safe takeoff area with a bright roof edge and a brief hint. The character retains its facing direction after stopping, with a stronger outline and brighter shoes. Ground speed is 3px per 20ms tick; jump velocity begins at -10px/tick with 0.5px/tick gravity. A level jump lasts about 780ms and covers 117px. Waiting does not charge a stronger jump, and airborne C never double-jumps. Every gap is traversable without an optional skill. Tokens award 100 points; finishing awards 500. Collected-token bits prevent duplicate rewards after a fall.

## Implementation and validation

- Independent model, Chinese copy, city renderer, device adapter, manifest and Wasm bridge live in `main/games/rooftop_runner/`. It does not reuse the demo menu or another game's visual layout.
- Model state is 348 bytes, with no gameplay allocation. Device rendering reuses two 320 × 40 RGB565 DMA strips, 50 KiB total, without a full-screen framebuffer or Wi-Fi/BLE/LVGL initialization. Browser framebuffer memory is excluded from firmware.
- Camera follows the player with forward visibility and at most 8px movement per simulation tick. Camera coordinates never enter collision calculations. Buildings use parallax, with course-specific sky palettes, animated running/scarf motion, tokens and flags.
- Four fixed-point microsteps per 20ms tick resolve downward roof crossings and side walls. A falling character cannot snap upward onto a roof. Pause freezes motion, obstacles and feedback; held inputs are cleared and a pause key's release cannot jump.
- Independent NVS namespace: `rooftop_runner`. Chinese font: 90 Noto Sans CJK SC glyphs, 2,700 bytes of Flash records under SIL OFL. OTF subset retained for generation and excluded from firmware. Font tools require Pillow 12.2.0 and fonttools 4.62.1.

```bash
PASSPORT_GAME=rooftop_runner ./tools/validate.sh
python3 tools/games/rooftop_runner/build_preview.py
./tools/games/rooftop_runner/check.sh
python3 -m http.server 8770 --bind 127.0.0.1 --directory build/games/rooftop_runner/preview
```

Activate ESP-IDF 5.5.3 for firmware builds. Browser controls: A/right arrow forward, B/left arrow backward, C/up arrow/space jump, and B/enter for menu confirmation. Pointer buttons mirror the device. Blur or canceled pointers pause; browser sound starts off and progress is session-only. Rendered PNGs under `build/games/rooftop_runner/` come from actual drawing code, not a physical device.

Host tests complete 100 stages across five courses, varying launch position and the sequential C hold from 20–100ms, without falls. Five additional backward routes verify every course and bounded camera movement. Regression checks cover immediate stop, direction reversal, standing/running jumps, no double jump, high-speed landing, side collision, no upward snapping, checkpoint recovery, retained rewards, pause, stale timestamps, clock wrap, unlocks and replay. Five strip heights are compared to full rendering across 25 scenes with memory guards. Native/Wasm replay compares 21 model and RGB565 checkpoints across 2,000 steps. Hardware behavior is validated separately.

## Previous flashed build (2026-10-01)

- Build: PASS, complete `PASSPORT_GAME=rooftop_runner ./tools/validate.sh` gate, merge and verified archive.
- Host tests: PASS, 100 completed stages, five backward routes, collision/input/strip regressions, 21 native/Wasm checkpoints and undefined-behavior sanitizer.
- Device tests: PASS for flashing and bounded startup observation only. The verified merged image was flashed to `/dev/cu.usbmodem101`; a ROM-mode uncompressed retry passed device hash verification after the initial stub write failed verification. A 20-second log matches the archived ELF prefix and reports `game=rooftop_runner`, without an observed panic. The monitor was closed.
- Unverified: browser interaction (automation blocked by unavailable security-policy checks), device orientation/readability/button feel/sound, cold-boot storage, frame rate and free heap.

Verified archive: `build/firmware/eb246b165cb7ee69fafd9c64f992783fcdde495d7727bafcca5f482a5ef1b453/`. Merged size: 442,624 bytes, for flashing from `0x0`. Use its merged image and matching archived ELF. Merged flashing can reset stored settings and game records, and requires specific image/port consent. Targets remain 25 FPS, 30 KiB free internal heap and a 16 KiB largest block; these are not hardware measurements.

```text
full SHA256: eb246b165cb7ee69fafd9c64f992783fcdde495d7727bafcca5f482a5ef1b453
ELF SHA256:  2afd5ab94e87d965d037b8952615fcc36c0ce50b2ead14d9f15e39674649b730
```

Observed startup/title runtime: frame time 21–22ms, free internal heap 233,488 bytes, minimum 227,728 bytes and largest block 114,688 bytes. These do not establish busy-scene performance, visible UI, physical controls, sound or cold-boot persistence.

## Optimized build (2026-10-01)

- Build: PASS, complete `PASSPORT_GAME=rooftop_runner ./tools/validate.sh` gate and verified archive.
- Host tests: PASS, 100 complete stages, five backward routes, late-jump and pre-landing buffer checks, pause cancellation, facing, strip regressions, 21 native/Wasm checkpoints and undefined-behavior sanitizer.
- Device tests: NOT RUN for this optimized build; the earlier build's measurements above do not validate this revision.
- Unverified: optimized button feel, visible hints, sound, frame rate and storage on hardware; browser interaction remains unverified.

The optimized model is 344 bytes; the font contains 85 glyphs and 2,550 bytes of Flash records. Verified archive: `build/firmware/e9ac07bd0afd7ee85213f817c3b2b6c4da5927aa9aa4e84673ed71e4d7f7b3db/`. Merged size: 443,472 bytes, for flashing from `0x0`. Flashing can reset stored records and needs consent for this exact revision and target port.

```text
full SHA256: e9ac07bd0afd7ee85213f817c3b2b6c4da5927aa9aa4e84673ed71e4d7f7b3db
ELF SHA256:  a282b0d39e32d29755a1f98e1c968662aa9a6a69cdf1f9744f5d181c28bc7808
```


## Current five-game upgrade

Added flawless and full-collection medals, persistent best stage medals and a 300-point full-collection bonus. Added city music and distinct collection/checkpoint cues, retaining the existing edge-jump grace and pre-landing jump buffer.

1. Hold the device horizontally; no network setup is needed. Tap B on the title to start; A/C select an unlocked course.
2. Hold A to move forward or B backward; release to stop. Near an edge, release the direction button and immediately tap C to jump that way. Tap C while standing still for a vertical jump.
3. Adjust direction in the air with A/B, avoid hazards and collect glowing rewards. Falls or collisions return you to the latest checkpoint while keeping rewards collected this attempt. A full collection earns extra points.
4. Hold C to pause, tap B to resume, and hold A while paused to return to the title. Hold A on the title to toggle sound.
5. At the finish, tap B for the next course; the fifth can be replayed. Unlocks, best score and stage medals save automatically. Aim for a flawless run or full collection.

Current artifact identities and validation limits are recorded in [the batch report](quality-upgrade.md). Older build and device records below describe their original revisions.
