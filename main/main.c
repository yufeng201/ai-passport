/* The selected manifest owns startup; reference demo screens are not registered. */
#ifndef PASSPORT_GAME_ENTRY
#error "Select a game through main/CMakeLists.txt"
#endif
void PASSPORT_GAME_ENTRY(void);
void app_main(void)
{
    PASSPORT_GAME_ENTRY();
}
