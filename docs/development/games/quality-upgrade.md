<p align="right"><a href="quality-upgrade.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# Five-game experience upgrade

## Evidence and constraints

The community screenshot provides downloads and likes, not impressions, detail visits or first-session behavior. Visible downloads are 13 for Road Rage, four for Alley Ninja and three for Cloudbound; usable figures for Brick Workshop and Rooftop Runner are unconfirmed. This sample cannot establish the cause of low downloads or predict a breakout hit.

8 MB refers to Flash, not working RAM. The previous Rooftop Runner merged image is 443,472 bytes, with a 377,936-byte application, rather than about 100 KB. The application partition provides approximately 7.94 MiB. Content can expand, but file size is not a quality metric. There is no PSRAM. This iteration retains two 320 × 40 display strips and a 160-sample audio block, without full-screen images or whole music tracks in RAM.

## Implemented experience changes

| Game | Source-level weakness | Change and hook | Three medal criteria |
| --- | --- | --- | --- |
| Road Rage | No extra reward for consecutive overtakes; unlocked stages did not persist | Three clean overtakes earn a three-second speed burst and points; damage resets the chain and boost. Chain meter, boost cue/sound, persistent stage unlocks and medals | Finish, stage objective, at least 80 health |
| Cloudbound | A miss forces a restart, making later jumps costly to practice | Tap A after a miss for one rescue per attempt, preserving landings and score while resetting the combo; B still restarts. Visible precision target and distinct cue | Finish, no rescue, final precision combo of at least three |
| Alley Ninja | Perfect blocks score points but do not strengthen the next attack | Second and subsequent consecutive perfect blocks open a one-second heavy-riposte window. The next successful attack deals two damage; a wrong-direction attack does not consume it. Damage cancels it. Combo cue and sparks | Finish, full health, final perfect-block combo of at least three |
| Brick Workshop | Little reward variation after the two slow abilities run out | Every six destroyed bricks replenish one slow ability, capped at three. Supply progress/cue/sound and break particles | Finish, all three lives, at least one slow ability remaining |
| Rooftop Runner | Limited motivation to replay after finishing five stages | Collect all eleven tokens for 300 extra points; flawless/collection medals and separate checkpoint/collection cues | Finish, no falls, all eleven tokens |

Each game preserves the best medal rating per stage; a weaker replay never replaces it. Titles display the selected stage's medals and results explain the goals. Existing button mappings remain. Seven short synthesized cues use attack/release envelopes, including collectible, perfect, guard, supply and victory melodies. Silent/audio-unavailable play remains supported. Five distinct quiet looping music themes are synthesized in 160-sample chunks and stop on pause, mute or return to title. There are no random maps or additional stages.

## Publication and growth evaluation

Update the existing projects: Road Rage 795, Cloudbound 807, Alley Ninja 809, Brick Workshop 831 and Rooftop Runner 843. Verify the authorized update target and current server review state; a pending revision cannot be overwritten or replaced by a duplicate project.

Retain names, download history and likes. Lead descriptions with the core hook, keep instructions and this revision's release notes independent, inspect the existing AI cover files, and add actual C-renderer previews labeled as previews rather than device screenshots.

Where available, compare impressions → detail visits → downloads after release. Without impressions, compare downloads and feedback over comparable periods without attributing changes to a single feature. No download or breakout-hit guarantee is made.

The consumed create-once grants cannot update existing projects. Each update requires a fresh official update prompt copied from that project's page, with credentials outside the repository. No upload without a matching grant.

## Validation and remaining checks

Each game's `build/games/<game>/upgrade-*.json/log` records its build, host tests, native/Wasm replay and strip-boundary checks. These are not device tests. Remaining checks include orientation, medal readability, sound comfort, button feel, persistence/cold boot and busy-frame timing. Flashing requires approval for each image. Browser smoke checks passed for starting all five games, Road Rage lane changes, Cloudbound failure/rescue, Alley Ninja slashes, Brick Workshop launching and Rooftop Runner jumping/audio-toggle state; no browser error logs were observed. Extended campaigns and long-hold interaction in the browser remain unverified.


## Verified firmware batch

| Game | Project | Full image bytes | Full image SHA-256 | ELF SHA-256 |
| --- | --- | --- | --- | --- |
| Road Rage | 795 | 452496 | `0d0ce13cda0109d9d0a8e8f488f04e5e90b85d4193aac27e28ea85299c8c7617` | `a0cbda3ad622ff13a82108f8fa806bdd9ab71e945d46adb0d0de6a8cce376cc3` |
| Cloudbound | 807 | 445728 | `1d9b29f2e5d6e2e6682a645b48d160fb3225d0301cccb39ff8af1bfa2417fa5e` | `e1f9bd3379ef919d71ee2a477a5496ed9a3c12e3291e85adbf516c9e93cbaf8b` |
| Alley Ninja | 809 | 446656 | `a926ee27460de7f9f108d2b7b9f21fa604e0eb283c8fa36d3261638f28e9e7fa` | `12350d2e1249b061051514c623c5f7d5471aa87a1c6fb0301164dbf9e7c72639` |
| Brick Workshop | 831 | 444112 | `36c65efa05a1586ded7cfc08aaded232ff374dcf829b2ae53a9f2f6b807f4315` | `8284ea9e3968829d6e32b6c2b99882f3da2a93bc9b3249a62c2a4a329f5665be` |
| Rooftop Runner | 843 | 445936 | `4e5351e318e503e0d73a6135f51881ff35e28a4bc0718f9a92cedcef1a7ecb26` | `4f49911b88654e2e06ff4ebaac807f978956c79b97024a64614b0e8ae770e927` |

Each bundle is under `build/firmware/<full-image-sha256>/`; use its merged image at offset `0x0`. All five complete gates, archive verification and publishing file validations passed. These binaries have not been flashed or uploaded.

The latest server refresh confirms Rooftop Runner project 843, revision 1767 is approved/published. This is the previous version, not this new batch. No pending-review conflict remains for that project; a fresh matching update grant is still required.
