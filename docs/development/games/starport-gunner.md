<p align="right"><a href="starport-gunner.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# Starport Gunner

An offline horizontal defense shooter. Aim the fixed turret with A/C, release the direction button, and fire with B. The cannon keeps its angle, so simultaneous button presses are never required. Five stages introduce cruising ships, armor, crossing routes and a flagship. Intercept incoming missiles while protecting three harbor shields.

## Start playing

1. Hold the device horizontally; no Wi-Fi setup is needed. Tap B to start. A/C on the title screen select an unlocked stage.
2. Tap A/C to move the cannon one aim position left/right. Release B after a short press to fire an ordinary shot. Hold B for at least half a second, then release to fire a piercing shot when available. Maximum charge takes 1.2 seconds and never fires automatically.
3. Crosses and dotted lines mark hostile missile warnings before they descend. Shoot missiles or ships; golden bars show armored ship and flagship health. The harbor has three shields. Piercing shots recharge over 2.4 seconds; ordinary shots remain available during recharge.
4. Hold C for about 0.8 seconds to pause; releasing it does not turn the cannon. Tap B to resume, or hold A while paused to return to the title screen. Pause cancels an unreleased charge and freezes the battle.
5. Tap B after defeat to retry, or after clearing stages one through four to advance. Clearing stage five offers a replay. Hold B on a results screen to return to the title. Hold A on the title screen to toggle sound. Unlocked stages, best stage score and sound setting are saved; storage failure leaves the game playable with session-only records.

Use one button at a time. Transition keys must be released before a new action. Holding a direction does not repeatedly turn the cannon. A long B during piercing recharge releases an ordinary shot; the charge caption states which shot is available.

## Five stages

| Stage | Encounter | Progression |
| --- | --- | --- |
| Quiet lane | Six light ships | Vertical entry and stationary firing positions |
| Cruising arrivals | Seven light ships | Slow horizontal patrol |
| Armor breach | Eight mixed ships | Two-hit armor |
| Crossing orbits | Nine mixed ships | Faster opposing patrols and three-hit armor |
| Flagship defense | Four escorts and one flagship | A 16-health flagship with faster attacks |

At most three ships are active together. Enemy attack warnings last 800 ms, followed by missiles descending at 100 px/s. Every missed missile damages the harbor regardless of impact position; a 700 ms damage grace period prevents simultaneous impacts from removing all shields. Ordinary shots deal one damage. Piercing shots deal three damage and can hit up to three objects, but never repeatedly damage the same ship while overlapping it. Destroying light/armored ships awards 100/200 points, the flagship 1,200 points, and intercepting missiles 25 points. A stage clears after every scheduled ship is destroyed, provided at least one shield remains.

## Architecture and budgets

- `main/games/starport_gunner/`: deterministic model, its own Chinese copy and strip renderer, device adapter, build manifest and browser bridge.
- The selected firmware reuses `main/games/common/game_runtime.c` and audio; no other game's model is linked.
- State: 1,312 bytes. Fixed pools: six ship slots, 24 player projectiles, 12 hostile missiles. No allocation in game updates.
- Shared display buffers: two 320 × 40 RGB565 strips, 50 KiB total. No full-screen device buffer; no Wi-Fi, BLE or LVGL initialization.
- Font: 76 Chinese glyphs at 12 px, 2,280 bytes of Flash glyph records, Noto Sans CJK SC under SIL OFL. The OTF subset is not linked into firmware. Check/regenerate with `python3 tools/games/starport_gunner/generate_font.py [--check]`; new characters require the full source font via `--source`.
- Physics uses fixed 20 ms steps and swept projectile/target rectangles. Input classification uses original queued timestamps, with stale simulation timestamps ignored. Pause and long-release behavior use the same shared event-owner rules as Cloudbound.
- The independent NVS namespace is `starport_gunner`; the shared three-field progress format and storage failure policy are unchanged. Replacing the full firmware can reset existing records.

## Build and preview

With ESP-IDF 5.5.3 activated:

```bash
PASSPORT_GAME=starport_gunner ./tools/validate.sh
python3 tools/games/starport_gunner/build_preview.py
./tools/games/starport_gunner/check.sh
python3 -m http.server 8767 --bind 127.0.0.1 --directory build/games/starport_gunner/preview
```

Open the local preview at `http://127.0.0.1:8767`. A/left and C/right adjust aim; B/space/enter fire or charge. Pointer controls support the same edges. Blur or canceled input pauses the battle. Browser audio is separately controlled and starts off; browser progress is session-only. The preview runs the same C model and RGB565 renderer. Native/Wasm replay equality does not prove browser input or hardware behavior.

`tests/games/test_starport_gunner.c` checks 40 complete stages using eight seeds and a pilot with one-button edges and 100 ms between aim changes. It also checks charge caps, cooldown, pause, late input, timer wrap, shield damage, missile interception, swept collisions, single-hit piercing, retry progress and five strip sizes across 25 scenes. The native/Wasm replay checks state and full RGB565 frames at 21 checkpoints over 2,000 steps. Rendered images under `build/games/starport_gunner/` are native illustrations of the actual renderer, not device screenshots.

## Validation

Build, host and hardware results are recorded after the complete gate. Hardware orientation, readability, button feel, sound, power-cycle persistence, frame rate and heap remain unverified until this exact firmware is tested on a device. Target performance remains at least 25 FPS, 30 KiB free internal heap and a 16 KiB largest free block; these are goals, not measured results.

## Validation record (2026-10-01)

- Build: PASS, complete `PASSPORT_GAME=starport_gunner ./tools/validate.sh` gate and archive verification.
- Host tests: PASS, 40 complete stages, input/collision/strip regressions, 21 native/Wasm checkpoints and undefined-behavior sanitizer.
- Device tests: NOT RUN. No game device was detected during the handoff.
- Unverified: actual browser interactions, hardware orientation/readability/buttons/audio, power-cycle storage and busy-scene frame rate/heap.

Verified archive: `build/firmware/d65604d673f66e633e7b5d67009344c455c00fa62edb37ec6d2861961c7e44e1/`. The merged image is 443,216 bytes and flashes at `0x0`; use this exact archive, not the shared filename after another game's build. Publishing and device flashing require their respective authorization.

```text
full SHA256: d65604d673f66e633e7b5d67009344c455c00fa62edb37ec6d2861961c7e44e1
ELF SHA256:  9f281b54277a298fe33fbeac19b3b54a992198a47bdf7bf56d51fbf822ab7e01
```
