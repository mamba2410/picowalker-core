#ifndef PW_AUDIO_H
#define PW_AUDIO_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "picowalker_structures.h"

#define SOUND_NAVIGATE_MENU 0
#define SOUND_NAVIGATE_BACK 1
#define SOUND_CURSOR_MOVE 2
#define SOUND_POKERADAR_FOUND_STH 3
#define SOUND_SELECTION_MISS 4
#define SOUND_DOWSING_FOUND_ITEM 5
#define SOUND_BATTLE_UNKNOWN_6          6   // Success Sound?
#define SOUND_BATTLE_CAUGHT             7
#define SOUND_BATTLE_UNKNOWN_8          8  // Mono-tone sound . . . . .
#define SOUND_BATTLE_UNKNOWN_9          9  // Special Sound? 
#define SOUND_BATTLE_ENCOUNTER          10
#define SOUND_BATTLE_HIT                11
#define SOUND_BATTLE_EVADE              12
#define SOUND_BATTLE_CRITICAL           13
#define SOUND_BATTLE_FLED               14
#define SOUND_BATTLE_POKEBALL_THROW     15


extern void pw_audio_init();
extern void pw_audio_play_sound_data(const pw_sound_frame_t* sound_data, size_t sz);
extern bool pw_audio_is_playing_sound();

/*
 *  pw-core definitions
 */
extern pw_volume_t pw_audio_volume;
extern const uint8_t PW_AUDIO_PERIODTAB[];

void pw_audio_play_sound(uint8_t sound_id);
void pw_audio_set_volume(uint8_t vol);

#endif /* PW_AUDIO_H */
