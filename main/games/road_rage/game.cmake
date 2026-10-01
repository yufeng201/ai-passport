# This manifest selects one standalone firmware. Browser-only sources are excluded.
set(GAME_ENTRY rr_device_run)
set(GAME_SOURCES
    "games/road_rage/rr_game.c"
    "games/road_rage/rr_render.c"
    "games/road_rage/rr_device.c"
    "games/common/game_audio.c"
)
set(GAME_INCLUDE_DIRS "games/road_rage" "games/common" "../assets/fonts")
set(GAME_REQUIRES bsp esp_lcd esp_timer)
