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
