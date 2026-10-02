set(GAME_ENTRY rp_device_run)
set(GAME_SOURCES
    "games/rooftop_runner/rp_game.c"
    "games/rooftop_runner/rp_render.c"
    "games/rooftop_runner/rp_device.c"
    "games/common/game_runtime.c"
    "games/common/game_audio.c"
)
set(GAME_INCLUDE_DIRS "games/rooftop_runner" "games/common" "../assets/fonts")
set(GAME_REQUIRES bsp esp_lcd esp_timer nvs_flash)
