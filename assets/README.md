<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Assets

This directory stores reusable fonts, images, music, and sound effects, organized by asset type.

Keep each asset in the matching subdirectory and document its destination, naming, integration method, and source/license. Do not mix binary assets with Markdown documentation.

## Fonts

Store reusable font files and generated font sources in `fonts/`.

- Use descriptive names that include the family, weight, size, and format when relevant.
- Document the source, license, character range, conversion command, and expected destination.
- Check Flash and internal-RAM impact before adding a font; the ESP32-C3 has no PSRAM.
- Do not commit fonts whose license does not permit redistribution.

- `fonts/road_rage_noto_sc_subset.otf`: 108-character subset of [Noto Sans CJK SC Regular](https://github.com/notofonts/noto-cjk/tree/main/Sans), under [SIL OFL 1.1](fonts/OFL.txt). Retained for reproducible conversion; not linked into firmware.
- `fonts/road_rage_noto_sc_12.h`: generated monochrome 12px Chinese glyphs in 12 × 14 cells, about 3 KiB of Flash records. Used directly by the shared game renderer; no LVGL font pool or runtime decoding. Copy and glyph coverage are checked by `python3 tools/games/road_rage/generate_font.py --check`; regenerate with `python3 tools/games/road_rage/generate_font.py` (Pillow 12.2.0). The source character list is `main/games/road_rage/rr_copy.h`. Extending it requires rebuilding the OTF subset using `--source <full-source-font.otf>` (fonttools 4.62.1), then rebuilding the preview and firmware.

- `fonts/cloudbound_noto_sc_subset.otf` and `fonts/cloudbound_noto_sc_12.h`: Cloudbound's 88-glyph Noto Sans CJK SC subset and 12px bitmap atlas (2,640 bytes of Flash records), under [SIL OFL](fonts/OFL.txt). Copy inventory: `main/games/cloudbound/cb_copy.h`; generation/check: `python3 tools/games/cloudbound/generate_font.py [--check]`. Rebuilding the subset requires `--source <full-source-font.otf>`, Pillow 12.2.0 and fonttools 4.62.1. The OTF is not linked into firmware.

## Images

Store reusable source images and generated display assets in `images/`.

| File | Dimensions and format | Use and source |
| --- | --- | --- |
| [`images/home.jpg`](images/home.jpg) | 3840 × 2160, JPEG | Product hero image embedded in both project README files to foreground AI Passport and its open, maker-oriented identity. |
| [`images/readme-hardware-specs.png`](images/readme-hardware-specs.png) | 2172 × 724, PNG RGBA | Optional technical infographic retained as a reference asset; it is no longer used as the homepage hero. Generated for this repository with the built-in image generation tool on 2026-09-17; the six labels and values were checked against the documented hardware contract. |
| [`images/logo-wordmark.png`](images/logo-wordmark.png) | 1648 × 336, PNG RGBA | Transparent black wordmark extracted from the repository's original `images/logo.png`; embedded in both project README files for light backgrounds. |
| [`images/logo-wordmark-dark.png`](images/logo-wordmark-dark.png) | 1648 × 336, PNG RGBA | White version of the extracted wordmark, used by the README `<picture>` element when GitHub is in dark mode. |

- Use descriptive names and document dimensions, pixel format, conversion steps, and destination.
- Prefer formats suitable for the 240 × 320 RGB565 display and account for Flash and internal RAM.
- Preserve editable sources where licensing permits, and record the source and license.
- Never commit device QR secrets, credentials, or personal data in images.

## Music and sound effects

Store reusable music and sound-effect sources in `music/`.

- Document the source, license, sample rate, bit depth, channels, conversion command, and destination.
- Prefer 16 kHz, 16-bit mono PCM when it matches the current BSP audio path.
- Check Flash and internal-RAM cost before embedding audio; stream or chunk long recordings.
- Do not commit media without redistribution permission.

- `images/road-rage-cover.png`: portrait 3:4 promotional pixel-art illustration for Road Rage, generated with the built-in imagegen tool. It is cover artwork, not a device screenshot.

- `images/cloudbound-cover.png`: 1086 × 1448 RGB PNG, exact portrait 3:4 cover for Cloudbound. Generated with the built-in imagegen tool on 2026-10-01; its completion was confirmed and this exact upload file was visually inspected. Labeled as an illustration in Chinese and English; not a device screenshot and not embedded in firmware. Prompt: polished pixel art of a white square adventurer with a teal scarf leaping between floating grass-topped islands, clouds, sunset, stars and aurora, with the titles the Chinese game title and “CLOUDBOUND” and an illustration label.

- `fonts/starport_gunner_noto_sc_subset.otf` and `fonts/starport_gunner_noto_sc_12.h`: Starport Gunner's 76-glyph Noto Sans CJK SC subset and 12px bitmap atlas (2280 bytes), under [SIL OFL](fonts/OFL.txt). Copy: `main/games/starport_gunner/sg_copy.h`; generate/check with `python3 tools/games/starport_gunner/generate_font.py [--check]`. The OTF is not embedded in firmware.

- `fonts/alley_ninja_noto_sc_subset.otf` and `fonts/alley_ninja_noto_sc_12.h`: 96 Noto Sans CJK SC glyphs at 12px (2,880 bytes), under [SIL OFL](fonts/OFL.txt). Copy: `main/games/alley_ninja/an_copy.h`; generator/check: `python3 tools/games/alley_ninja/generate_font.py [--check]`. The OTF is not embedded.

- `images/alley-ninja-cover.png`: 1086 × 1448 RGB PNG, exact 3:4 cover for Alley Ninja. Generated with the built-in imagegen tool on 2026-10-01; completion confirmed and the exact upload file visually inspected. Labeled as an illustration, not a device screenshot; not embedded in firmware. Prompt: polished pixel art of a teal masked ninja blocking one purple-and-gold armored opponent in a rainy neon alley, with the Chinese game title, “ALLEY NINJA” and an illustration label.

- `fonts/brick_workshop_noto_sc_subset.otf` and `fonts/brick_workshop_noto_sc_12.h`: 70 Noto Sans CJK SC glyphs, 12px bitmap records totaling 2,100 Flash bytes, licensed under [SIL OFL](fonts/OFL.txt). Copy: `main/games/brick_workshop/bw_copy.h`; generate/check: `python3 tools/games/brick_workshop/generate_font.py [--check]`. The OTF is not linked into firmware.

- `images/brick-workshop-cover.png`: 1086 × 1448 RGB PNG, portrait 3:4 cover for Brick Workshop. Generated with the built-in imagegen tool on 2026-10-01, explicitly completed and the exact copied upload file visually checked. Prompt: premium pixel-art glass-brick workshop in navy, pink, amber, teal and blue, one glowing ball, mechanical paddle and bumper, Chinese/English game title and a visible illustration label. This is promotional artwork, not a device screenshot. SHA256: `6eef5d2e821b85c2c6e7dec37b5c70485958712ca4c784b7c76e13e6fd561b98`.

- `fonts/rooftop_runner_noto_sc_subset.otf` and `fonts/rooftop_runner_noto_sc_12.h`: 85 Noto Sans CJK SC glyphs at 12px, 2,550 bytes of Flash records under [SIL OFL](fonts/OFL.txt). Copy: `main/games/rooftop_runner/rp_copy.h`; generate/check: `python3 tools/games/rooftop_runner/generate_font.py [--check]`. New characters require `--source <full-font.otf>`, Pillow 12.2.0 and fonttools 4.62.1. The OTF is not linked into firmware.

- `images/rooftop-runner-cover.png`: 1086 × 1448 PNG, exact portrait 3:4 cover for Rooftop Runner. Generated with the built-in imagegen tool on 2026-10-01; explicit completion confirmed and the exact copied upload file visually inspected. Promotional illustration, labeled in Chinese and English; not a device screenshot and not embedded in firmware. Prompt: premium pixel-art teal-clothed runner with a golden scarf jumping right between city rooftops at sunrise, collectible tokens, checkpoint flags, a subtle rooftop obstacle, Chinese/English game titles and an illustration label. SHA256: `8f198098acad0b885f7e08d6acfa80ba00002b9f7f615ba99958312160b56d34`.

Current five-game upgrade font atlases: Road Rage 113 glyphs / 3,390 bytes; Cloudbound 93 / 2,790; Alley Ninja 103 / 3,090; Brick Workshop 78 / 2,340; Rooftop Runner 90 / 2,700. Re-generated from the licensed full Noto source after adding medal and ability copy. Original looping melodies and seven enveloped sound cues are implemented in `main/games/common/game_audio.c`; no prerecorded tracks or external audio license is required. Music is synthesized into the existing 160-sample worker buffer.


## Five-game code previews

The five `images/*-preview.png` files for Road Rage, Cloudbound, Alley Ninja, Brick Workshop and Rooftop Runner are 960 × 1280 portrait contact sheets. They were rendered synchronously from each native C renderer using representative model fixtures, then composed with Pillow. They are labeled as code-renderer previews, not on-device captures. The exact upload files have been visually checked; covers are separately labeled illustrations.
