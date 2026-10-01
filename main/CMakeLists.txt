# Only the selected game's manifest enters the firmware component.
set(PASSPORT_GAME "road_rage" CACHE STRING "Standalone game to build")
if(NOT PASSPORT_GAME MATCHES "^[a-z][a-z0-9_]*$")
    message(FATAL_ERROR "Invalid PASSPORT_GAME name: ${PASSPORT_GAME}")
endif()
set(game_manifest "${CMAKE_CURRENT_LIST_DIR}/games/${PASSPORT_GAME}/game.cmake")
if(NOT EXISTS "${game_manifest}")
    message(FATAL_ERROR "Game has no build manifest: ${PASSPORT_GAME}")
endif()
include("${game_manifest}")
idf_component_register(
    SRCS "main.c" ${GAME_SOURCES}
    INCLUDE_DIRS "." ${GAME_INCLUDE_DIRS}
    REQUIRES ${GAME_REQUIRES}
    # IDF pre-scans requirements before arbitrary cached game options are visible.
    # Storage is a private platform capability of the shared runtime.
    PRIV_REQUIRES nvs_flash
)
target_compile_definitions(${COMPONENT_LIB} PRIVATE PASSPORT_GAME_ENTRY=${GAME_ENTRY})
set_source_files_properties(${GAME_SOURCES} PROPERTIES COMPILE_OPTIONS "-O2")
