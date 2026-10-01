set(GAME_ENTRY cb_device_run)
set(GAME_SOURCES
    "games/cloudbound/cb_game.c"
    "games/cloudbound/cb_render.c"
    "games/cloudbound/cb_device.c"
    "games/common/game_runtime.c"
    "games/common/game_audio.c"
)
set(GAME_INCLUDE_DIRS "games/cloudbound" "games/common" "../assets/fonts")
set(GAME_REQUIRES bsp esp_lcd esp_timer nvs_flash)
