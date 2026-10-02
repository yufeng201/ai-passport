set(GAME_ENTRY an_device_run)
set(GAME_SOURCES
    "games/alley_ninja/an_game.c"
    "games/alley_ninja/an_render.c"
    "games/alley_ninja/an_device.c"
    "games/common/game_runtime.c"
    "games/common/game_audio.c"
)
set(GAME_INCLUDE_DIRS "games/alley_ninja" "games/common" "../assets/fonts")
set(GAME_REQUIRES bsp esp_lcd esp_timer nvs_flash)
