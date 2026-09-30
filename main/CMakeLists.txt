idf_component_register(
    SRCS "main.c"
         "road_rage/rr_game.c"
         "road_rage/rr_render.c"
         "road_rage/rr_sound.c"
         "road_rage/rr_device.c"
    INCLUDE_DIRS "." "road_rage" "../assets/fonts"
    REQUIRES bsp esp_lcd esp_timer
)

# Optimize the hot integer renderer/model without changing global debug settings.
set_source_files_properties(
    "road_rage/rr_game.c" "road_rage/rr_render.c" "road_rage/rr_sound.c" "road_rage/rr_device.c"
    PROPERTIES COMPILE_OPTIONS "-O2"
)
