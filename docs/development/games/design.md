<p align="right"><a href="design.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# Landscape Three-Button Game Designs

This proposal replaces the previous predominantly single-button lineup. Timed single-button play is one option; the other games use three buttons for shooting, defense, continuous movement, and puzzles. [Cloudbound](cloudbound.md) has completed implementation and build validation; [Starport Gunner](starport-gunner.md) is implemented; [Alley Ninja](alley-ninja.md) is implemented; [Brick Workshop](brick-workshop.md) is implemented; Clockwork Maze awaits implementation. [Road Rage](road-rage.md) remains independently buildable.

## Shared Conventions

- Use 320 × 240 landscape, Chinese UI, offline play, and roughly one-to-three-minute sessions. A/C are the outer buttons and B the center button. Show physical positions on the title screen and provide a planned A/C swap option.
- The three buttons share one ADC. Sequential use of all three is supported; simultaneous presses must never be required. Aiming then firing, or releasing movement to use a skill, needs sufficient time.
- Short actions use press or immediate release edges without click/double-click delay. Define short and long actions per game; a long action must suppress the short action on release.
- Short B starts, retries, or advances. On results, long B returns to the title. On pause, short B resumes and long A returns to the title. In-game pause buttons appear below.
- Retry resets only the current stage, preserving best scores and unlocked progress. Saving at results is planned; storage failure must not prevent play.
- Each game has a standalone firmware, cover, instructions, and review submission. Share input, drawing, and sound infrastructure while keeping each game's state and visual layout distinct.

## Five Games

| Order | Game | Core buttons | Genre and experience | Pause |
| --- | --- | --- | --- | --- |
| 1 | Cloudbound | Hold B to charge; release to jump | Single-button distance judgment on floating cloud platforms | Long C |
| 2 | Starport Gunner | A/C aim; short B fires; hold B charges a shot | Three-button aiming and shooting to defend a starport | Long C |
| 3 | Alley Ninja | Short A/C slash left/right; hold B to guard | Directional judgment and defense-to-attack transitions in neon alleys | Long C |
| 4 | Brick Workshop | Hold A/C to move the paddle; short B launches/slows the ball | Continuous movement and rebound physics against colorful bricks | Long B |
| 5 | Clockwork Maze | Short A/C step left/right; short B interacts; long B undoes | Single-room mechanical puzzles with observable routes | Long C |

### 1. Cloudbound

Keep one genuinely single-button game. Holding B crouches the character and fills a cyan-to-gold charge arc; releasing launches a jump. At maximum charge, power stays capped without launching automatically. Center landings build combos; edge landings still continue. Missing retries the current stage.

The side view shows the current and next platforms with clear landing space. Backgrounds progress through morning mist, clear skies, sunset, stars, and aurora. Stage one offers a predicted landing point, then assistance gradually recedes.

Five stages introduce distance variation, smaller platforms, height differences, and combinations, requiring 12 landings each. Validate platforms against actual jump equations and retain at least a 150 ms successful charge window. No invisible wind. Ignore fresh jumps during flight and require a new press after landing.

### 2. Starport Gunner

Operate a fixed turret at the bottom. Short A/C adjusts one aiming increment. The barrel retains its direction after release, allowing B to fire without simultaneous aiming. Short B fires an ordinary shot; holding and releasing B fires a charged piercing shot, capped at maximum power.

Ships enter from above on predictable routes; incoming impact locations flash in advance. Intercept enemy projectiles to preserve the starport's three armor units. Ordinary shots remain available while the charged shot cools down, with clear availability indicators. Holding C pauses without adding a short aiming action.

Five stages introduce slow straight-moving ships, lateral ships, armored ships, crossing routes, and a flagship. Increase route complexity and coordination while preserving time to aim, release, and fire. Stars, a harbor silhouette, muzzle glow, and small explosions create clear vertical layers; particles must not hide trajectories.

### 3. Alley Ninja

The character stands in the center with enemies approaching from either side. Short A slashes left; short C slashes right. Attacks have explicit reach and cannot hit distant enemies. Hold B to guard; release to recover stamina. Pressing B promptly after an enemy windup performs a perfect guard, followed by a released-button A/C counterattack.

Guarding consumes stamina and cannot provide permanent invulnerability. Show guard breaks clearly. Enemy attacks arrive sequentially, leaving time to switch from defense to attack; no simultaneous left/right demands. Slashes use immediate release; long C pauses without a trailing right slash.

Five stages introduce single-side enemies, alternating sides, guard counters, varied windups, and a boss. Challenge comes from reading attacks, not hidden hit tests or frantic tapping. Use deep-blue alleys, cyan/pink neon, and white blade trails. Windups need motion and symbols, not only color or audio.

### 4. Brick Workshop

Bricks occupy the upper landscape screen and the paddle sits below. Hold A/C for continuous lateral movement; release stops promptly. Contact location determines rebound angle. Short B launches a waiting ball or uses a limited slow field during flight, giving time to read the trajectory and return to movement. This optional skill must never require simultaneous input to finish.

Cap ball speed, start gently, and give three attempts per stage. After a lost ball, wait for an explicit launch. Long B pauses and must not release a skill afterward. Use glowing glass bricks, a mechanical paddle, short trails, and a clearly visible ball; flashes must not obscure it.

Five stages introduce regular walls, durable bricks, fixed rebound obstacles, moving brick groups, and combinations. Layouts must contain no unreachable bricks; use an anti-stagnation mechanism when needed to break repeating trajectories. Continuous collision detection prevents the ball from tunneling through the paddle at high speed or low frame rates.

### 5. Clockwork Maze

Each stage is one horizontal room showing the character, mechanisms, and exit together. Short A/C steps left/right. Short B operates a nearby switch, ladder, or portal; a symbol identifies the currently interactable object. Long B undoes the last step rather than forcing a full restart after an accidental action.

Walking toward a crate pushes it without an interaction combination. Interactable objects must not overlap ambiguously. Stage one has a switch and door; later stages introduce crate pressure plates, lifts, one-way doors, and combined mechanisms. Use five handcrafted stages initially and verify solutions by state search, not random maze generation.

No timer, pursuers, or reaction challenge: this provides a slower alternative to the other four games. Copper-green mechanisms, warm lights, and stone walls distinguish open/closed doors and active/inactive devices. Decoration must not occupy essential interaction space.

## Resource, Visual, and Acceptance Targets

These are development budgets and acceptance targets, not measurements of new games.

- Reuse RGB565 strip drawing: two 320 × 40 buffers total 50 KiB, with no device full-screen buffer. Budget no more than 16 KiB logical state per game, fixed object pools, and no per-frame allocation.
- Initially budget no more than 256 KiB additional Flash per game for art/sound. Prefer pixel sprites and procedural drawing; linking and partition validation determine capacity.
- Use fixed 20 ms logic steps and target at least 25 FPS. Input uses event timestamps. Pause freezes timing, charging, and physics; resume requires releasing old input. Paddle collision and jump judgments match visible geometry.
- Show only stage, score/goal, health, and essential skill state. Keep critical content clear of corner masks. Check new Chinese glyph coverage before reviewing actual rendered/uploaded images.
- Complete each game in order: full flow, host tests, visual inspection, firmware build, then bilingual descriptions, separate instructions, and a portrait 3:4 cover. Record device checks separately from builds and report the actual review status after submission.
- The first prototype is not fully validated and cannot be delivered or published as this proposal. Adjust or replace its logic per game while preserving Road Rage's standalone build capability.
