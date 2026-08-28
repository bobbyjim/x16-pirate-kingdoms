/* Debug sound HAL: prints the cue name instead of playing it. Host-only;
   linked into pk-cli. Swap for platform/null to silence it. */

#include <stdio.h>
#include "../../present/sound.h"

static const char *SOUND_NAMES[SOUND_COUNT] = {
    "BOOM", "PING", "CHIME", "ALERT"
};

void sound_init(void) { printf("[sound] init\n"); }

void sound_play(SoundId id)
{
    const char *name = ((int)id >= 0 && id < SOUND_COUNT) ? SOUND_NAMES[id] : "?";
    printf("[sound] %s\n", name);
}
