set(GAME_ENTRY sg_device_run)
set(GAME_SOURCES
    "games/starport_gunner/sg_game.c"
    "games/starport_gunner/sg_render.c"
    "games/starport_gunner/sg_device.c"
    "games/common/game_runtime.c"
    "games/common/game_audio.c"
)
set(GAME_INCLUDE_DIRS "games/starport_gunner" "games/common" "../assets/fonts")
set(GAME_REQUIRES bsp esp_lcd esp_timer nvs_flash)
