#ifndef PW_AUDIO_H
#define PW_AUDIO_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define SOUND_NAVIGATE_MENU 0
#define SOUND_NAVIGATE_BACK 1
#define SOUND_CURSOR_MOVE 2
#define SOUND_POKERADAR_FOUND_STH 3
#define SOUND_SELECTION_MISS 4
#define SOUND_DOWSING_FOUND_ITEM 5
#define SOUND_POKEMON_CAUGHT 7
#define SOUND_POKEMON_ENCOUNTER 10
#define SOUND_MINIGAME_FAIL 14
#define SOUND_POKEBALL_THROW 15

typedef struct {
    uint8_t info;
    uint8_t period_idx;
} pw_sound_frame_t;

typedef enum {
    VOLUME_NONE=0,
    VOLUME_HALF=1,
    VOLUME_FULL=2
} pw_volume_t;


extern void pw_audio_init();
extern void pw_audio_play_sound_data(const pw_sound_frame_t* sound_data, size_t sz);
extern bool pw_audio_is_playing_sound();

/*
 *  pw-core definitions
 */
extern pw_volume_t pw_audio_volume;
extern uint8_t PW_AUDIO_PERIODTAB[];

void pw_audio_play_sound(uint8_t sound_id);

#endif /* PW_AUDIO_H */
