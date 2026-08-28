#ifndef _SOUND_H_
#define _SOUND_H_

/* Platform sound primitives. Same build-time swap as gfx.h
   (platform/null | platform/debug | platform/x16). The catalogue is a
   fixed enum of abstract cues; which Note maps to which cue is the
   presentation layer's call (present/present.c), not the engine's and not
   this header's. */

typedef enum {
    SOUND_BOOM = 0,  /* something was destroyed (settlement collapse)      */
    SOUND_PING,      /* something connected (link formed, caravan arrived) */
    SOUND_CHIME,     /* something was founded (colony, resettled ruin)     */
    SOUND_ALERT,     /* something is under threat (event, link disrupted)  */
    SOUND_COUNT
} SoundId;

void sound_init(void);
void sound_play(SoundId id);

#endif
