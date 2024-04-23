
#include "audio.h"

#include "eeprom.h"
#include "eeprom_map.h"
#include "utils.h"

#define MAX_SOUND_DATA 0xc0

typedef struct {
    uint16_t offset;
    uint8_t length;
} sound_info_t;

pw_volume_t pw_audio_volume;
uint8_t PW_AUDIO_PERIODTAB[] = {0xf4, 0xe6, 0xd9, 0xcd, 0xc2, 0xb7, 0xac, 0xa3, 0x9a, 0x91, 0x89, 0x81, 0x7a, 0x73, 0x6c, 0x66, 0x61, 0x5b, 0x56, 0x51, 0x4d, 0x48, 0x44, 0x40, 0x3d, 0x39, 0x36, 0x33, 0x30, 0x2d, 0x2b, 0x28, 0x26, 0x24, 0x22, 0x20, 0x1e, 0x1c, 0x1a, 0x19, 0x17, 0x16};

pw_sound_frame_t sound_data_buffer[MAX_SOUND_DATA];

void pw_audio_play_sound(uint8_t sound_id) {
    sound_info_t sound_info;
    if (pw_audio_volume != VOLUME_NONE) {
        pw_eeprom_read(PW_EEPROM_ADDR_SOUND_OFFSET + sound_id * sizeof(sound_info_t), (uint8_t*)&sound_info, sizeof(sound_info_t));
	sound_info.offset = sound_info.offset;

	if (sound_info.length <= MAX_SOUND_DATA) {
            pw_eeprom_read(PW_EEPROM_ADDR_SOUND_DATA + sound_info.offset, (uint8_t*)sound_data_buffer, sound_info.length);

	    // TODO: Some sum being calculated here?
	    
	    pw_audio_play_sound_data(sound_data_buffer, sound_info.length);

	}
    }
}
