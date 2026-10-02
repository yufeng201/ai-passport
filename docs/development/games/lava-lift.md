<p align="right"><a href="lava-lift.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# Lava Lift

Escape a rising lava shaft on a sliding lift. The first key moves the lift left, the second right, and the third jumps. Hold a direction to move; the rider follows the lift even while airborne, so no simultaneous key presses are required. Lava and the lift rise automatically; scrolling walls show the ascent.

## Play

Press C to start. Steer the entire lift through the gold-edged openings in rock shelves. Jump over approaching lava monsters; descending onto a monster defeats it. Rock contact or a monster touching the rider costs one of three lives and grants 1.4 seconds of invulnerability. Reach height 1200 (approximately 60 seconds) to escape. C restarts after defeat or victory. Firmware saves the best height through the shared runtime.

The device screen uses English labels and icons. The browser instructions are in Chinese. Input cancellation or a simulation gap longer than 200 ms pauses play; C resumes. Browser focus loss clears held keys and pauses immediately. There is no manual pause shortcut on the three gameplay keys.

## Build and preview

Select this game explicitly; the repository default is unchanged:

```bash
PASSPORT_GAME=lava_lift ./tools/validate.sh
python3 tools/games/lava_lift/build_preview.py
./tools/games/lava_lift/check.sh
python3 -m http.server 8771 --bind 127.0.0.1 --directory build/games/lava_lift/preview
```

Activate ESP-IDF 5.5.3 before the full gate. Preview compilation uses the existing Wasm toolchain. The browser runs the same C model and renderer as firmware; keyboard arrows move and Space jumps. Browser records last only for the current session. Device rendering uses two shared DMA strips, without a full-frame buffer or LVGL. Audio failures allow silent gameplay. The existing licensed Cloudbound Noto glyph asset supplies the English text.

## Acceptance

Host tests cover movement bounds, jump timing, rock damage, invulnerability, monster attacks and stomps, pause, restart, victory, update cadence, and strip/full-frame rendering equality. Native/Wasm replay compares state and pixel hashes at 21 checkpoints. Actual screen orientation, physical button order, response time, sound and complete-course difficulty require device testing. Compilation and browser testing do not establish hardware acceptance.
