#ifndef _RNG_H_
#define _RNG_H_

#include "common.h"

/* Small deterministic PRNG (xorshift32) so a given seed + command sequence
   always reproduces identical simulation results. Never use libc rand()
   in engine code -- it isn't guaranteed reproducible across platforms. */

typedef struct {
    unsigned long state;
} Rng;

void rng_seed(Rng *r, unsigned long seed);
byte rng_next_byte(Rng *r);          /* 0-255 */
word rng_range(Rng *r, word max);    /* 0..max-1 */

#endif
