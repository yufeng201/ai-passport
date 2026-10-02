#include "rr_render.h"
#include "game_audio.h"
static rr_game_t game;
/* Browser-only full framebuffer; excluded from the firmware source list. */
static uint16_t frame[RR_WIDTH * RR_HEIGHT];
void game_init(uint32_t seed) { rr_init(&game, seed); }
void game_input(int input) { if (input >= RR_LEFT && input <= RR_HOME) rr_input(&game, (rr_input_t)input); }
void game_tick(uint32_t ms) { rr_tick(&game, ms); }
uint16_t *game_frame(void) { rr_render_strip(&game, frame, 0, RR_HEIGHT); return frame; }
uint32_t game_hash(void) { return rr_state_hash(&game); }
int game_phase(void) { return game.phase; }

int game_health(void) { return game.health; }
int game_attack(void) { return game.attack_ms; }
int game_knockouts(void) { return game.knockouts; }
int game_sound_sample(int effect, unsigned sample) { return game.muted?0:game_audio_sample(effect, sample); }

int game_boost(void) { return game.boost_ms; }

int game_music_theme(void) { return game.phase==RR_RACING&&!game.muted?1:0; }
int game_music(unsigned theme,unsigned sample) {return game_music_sample((int)theme,sample);}
