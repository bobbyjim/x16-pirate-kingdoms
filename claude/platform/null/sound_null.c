/* Null sound HAL: sound_play() does nothing. */

#include "../../present/sound.h"

void sound_init(void) {}
void sound_play(SoundId id) { (void)id; }
