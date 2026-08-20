#include "rng.h"

void rng_seed(Rng *r, unsigned long seed)
{
    /* xorshift32 requires a non-zero state */
    r->state = seed ? seed : 0x9e3779b9UL;
}

static unsigned long rng_next(Rng *r)
{
    unsigned long x = r->state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    r->state = x;
    return x;
}

byte rng_next_byte(Rng *r)
{
    return (byte)(rng_next(r) & 0xFF);
}

word rng_range(Rng *r, word max)
{
    if (max == 0) return 0;
    return (word)(rng_next(r) % max);
}
