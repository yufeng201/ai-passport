set(GAME_ENTRY ll_device_run)
set(GAME_SOURCES "games/lava_lift/ll_game.c" "games/lava_lift/ll_render.c" "games/lava_lift/ll_device.c" "games/common/game_runtime.c" "games/common/game_audio.c")
set(GAME_INCLUDE_DIRS "games/lava_lift" "games/common" "../assets/fonts")
set(GAME_REQUIRES bsp esp_lcd esp_timer nvs_flash)
