set(GAME_ENTRY bw_device_run)
set(GAME_SOURCES
    "games/brick_workshop/bw_game.c"
    "games/brick_workshop/bw_render.c"
    "games/brick_workshop/bw_device.c"
    "games/common/game_runtime.c"
    "games/common/game_audio.c"
)
set(GAME_INCLUDE_DIRS "games/brick_workshop" "games/common" "../assets/fonts")
set(GAME_REQUIRES bsp esp_lcd esp_timer nvs_flash)
