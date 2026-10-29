/* Borland C++ 3.0 RTL rand/random sequence (docs/native.md). */
#include "dzplat.h"
static uint32_t random_seed = 1;
static uint64_t random_calls;

/* The RTL srand parameter is unsigned (16 bits), although its LCG state is 32.
 * native/tests/ref/data/rng.txt confirms 0001 and 10001 seed identically. */
void dz_srand(uint32_t seed) { random_seed = (uint16_t)seed; random_calls = 0; }
int16_t dz_rand(void)
{
    random_seed = random_seed * UINT32_C(22695477) + 1u;
    ++random_calls;
    return (int16_t)((random_seed >> 16) & 0x7fffu);
}
uint64_t dz_rand_calls(void) { return random_calls; }
uint32_t dz_random_state(void) { return random_seed; }
