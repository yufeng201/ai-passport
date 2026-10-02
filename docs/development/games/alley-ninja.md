<p align="right"><a href="alley-ninja.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# Alley Ninja

An offline horizontal action game about reading attacks and choosing the correct side. Tap A/C to slash left/right, hold B to guard, then release B before counterattacking. Enemies approach in sequence, so no simultaneous button presses are required. Five stages progress from a single approach direction to armored opponents, varied timing and a flanking boss.

## Start playing

1. Hold the device horizontally; no network setup is needed. Tap B on the title screen to begin. A/C select unlocked stages.
2. Tap A for a left slash or C for a right slash when the opponent is close. Distant or wrong-side attacks miss. An exclamation mark and shrinking red bar announce an incoming attack.
3. Hold B to guard. Starting within the last 160 ms before impact earns a perfect block and bonus points; an ordinary block also opens the counterattack window. Release B and tap the correct slash button while the teal counter message is visible. Armor cannot be cut before blocking.
4. Watch the stamina bar. Holding guard drains stamina; releasing B restores it. Exhaustion temporarily breaks guard. Hold C for about 0.8 seconds to pause, tap B to resume, or hold A while paused to return to the title. Pause clears held guard and freezes the battle; releasing a pause key never causes a slash.
5. Tap B after defeat to retry or after stages one through four to advance. Stage five offers a replay. Hold B on results to return to the title; hold A on the title to toggle sound. Unlocks, best stage score and sound setting are saved, with session-only records if storage fails.

Each action uses one button. Directional slashes happen on release without a double-click delay and have a 240 ms cooldown. Holding a direction does not repeat attacks. A held, exhausted guard never rearms automatically; release B, let stamina recover, then press again.

## Five stages and combat

| Stage | Encounters | Difficulty |
| --- | --- | --- |
| First night | Five light opponents from the right | A 1.2-second attack warning |
| Both sides | Six alternating opponents | A 1.1-second warning |
| Iron trial | Seven alternating opponents | Two-health armor; block before countering |
| Offbeat district | Eight opponents | Faster approaches and 0.8/1.0/1.2-second warnings |
| Shadow guardian | Four escorts and a boss | Five-health boss that changes approach side between attacks |

Only one enemy attacks at a time. An ordinary block costs 24 stamina; a perfect block costs eight. Guarding drains 25 stamina per second and unguarded recovery restores 50 per second. Starting a guard requires 24 stamina. Exhaustion disables attacks and guard for 800 ms. Successful blocks expose armor for one second; the enemy recovers for 1.1 seconds, allowing sequential release and counter inputs. The attack range is 58 px from the player. Light/armored enemies award 100/200 points; the boss awards 1,000. Consecutive perfect blocks award 20 times the current perfect-block combo (capped at 99); scores cap at 1,000,000 to keep extended sessions within the saved-record format. Failure/retry affects the current battle only and preserves unlocks and best score.

## Implementation and validation

- `main/games/alley_ninja/`: pure deterministic combat, Chinese copy, independent neon-alley renderer, device adapter, manifest and browser bridge.
- Shared runtime/audio remain in `main/games/common/`; only the selected game's sources enter its firmware. The default remains Road Rage.
- State is 188 bytes; no dynamic allocation in combat. The device keeps two 320 × 40 RGB565 buffers, 50 KiB total, without a full-screen framebuffer or Wi-Fi/BLE/LVGL initialization.
- Chinese font: 103 Noto Sans CJK SC glyphs at 12 px, 3,090 bytes of Flash records, SIL OFL. The OTF is retained for conversion but not embedded. Generator: `python3 tools/games/alley_ninja/generate_font.py [--check]`; new characters require `--source` with a full source font.
- Independent NVS namespace: `alley_ninja`. Reuses the shared best/unlocked/muted format; storage errors preserve play without erasing existing data.
- Fixed 20 ms simulation steps; original input timestamps classify long holds, stale frame clocks do not cause timer wrap. Pause freezes enemy timing, stamina, cooldown and feedback.

```bash
PASSPORT_GAME=alley_ninja ./tools/validate.sh
python3 tools/games/alley_ninja/build_preview.py
./tools/games/alley_ninja/check.sh
python3 -m http.server 8768 --bind 127.0.0.1 --directory build/games/alley_ninja/preview
```

Activate ESP-IDF 5.5.3 for firmware builds. The browser preview uses A/left, C/right and B/space/enter, plus pointer buttons; blur and canceled pointers pause. Browser sound is separately controlled and initially off. Browser progress is session-only. Native rendering images under `build/games/alley_ninja/` show actual drawing code, not device screenshots.

The tests simulate 16 seeds across five stages with both ordinary and perfect blocks: 160 complete stages. Every stage can be cleared with ordinary blocks, not just perfect timing. They check range/side, armor, cooldown, stamina exhaustion and recovery, pause, held transitions, late timestamps, clock wrap, progress retention, and five strip sizes over 25 scenes. Native/Wasm replay compares 21 state and RGB565-frame checkpoints across 2,000 steps. Build results, device results and remaining checks are recorded separately after the complete gate.

## Validation record (2026-10-01)

- Build: PASS, complete `PASSPORT_GAME=alley_ninja ./tools/validate.sh` gate and verified archive.
- Host tests: PASS, 160 complete stages, normal/perfect guards, scoring limits, input and strip regressions, 21 native/Wasm checkpoints and undefined-behavior sanitizer.
- Device tests: NOT RUN; no game device was detected.
- Unverified: real browser interactions, device orientation/readability/button feel/sound, power-cycle progress, busy-scene frame rate and heap.

Verified archive: `build/firmware/3fa903dfd322077c3e17f6b90abd405a702cbae2915ec01afbfb342653878b8e/`. Merged size: 443,952 bytes, flashing from `0x0`. Use the archived merged image and its matching ELF; another game's build replaces the shared delivery filename. Flashing can reset records and requires confirmation of this specific image and port. Target performance remains 25 FPS, 30 KiB free internal heap and a 16 KiB largest block, not hardware measurements.

```text
full SHA256: 3fa903dfd322077c3e17f6b90abd405a702cbae2915ec01afbfb342653878b8e
ELF SHA256:  0f2a14107486921053229b6e67eaf2e7feb8c6e94621469adb940b7726796a0b
```


## Current five-game upgrade

Added heavy ripostes after consecutive perfect blocks, combo cues and sparks. Added persistent three-goal medals, alley music and distinct perfect/block sounds. Wrong-direction attacks do not consume the riposte; damage cancels it.

1. Hold the device horizontally; no network setup is needed. Tap B on the title to start; A/C select an unlocked battle.
2. Tap A to slash left or C to slash right. Hold B to guard as an enemy approaches, then release B and counter. Armored foes must be blocked first to expose a weakness.
3. Consecutive perfect blocks grant a brief heavy-riposte window. Release B and slash toward the enemy. Watch stamina; releasing your guard restores it.
4. Hold C to pause and tap B to resume. Hold A while paused to return to the title, or on the title to toggle sound.
5. Tap B after victory for the next battle or after defeat to retry. Unlocks, best score and best stage medals save automatically.

Current artifact identities and validation limits are recorded in [the batch report](quality-upgrade.md). Older build and device records below describe their original revisions.
