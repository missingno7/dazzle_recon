/* Borland C++ 3.0 RTL diagnostic: records the actual CL.LIB rand/random results. */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static void dump_rand(FILE *out, unsigned seed)
{
    unsigned i;
    srand(seed);
    fprintf(out, "rand seed=%04X:", seed);
    for (i = 0; i < 2000; ++i)
        fprintf(out, "%s%u", i ? "," : "", (unsigned)rand());
    fputc('\n', out);
}

int main(void)
{
    static unsigned seeds[] = {0, 1, 12345, 0xffff};
    static int signed_limits[] = {0, 1, 2, 3, 32767, -1, -2};
    static unsigned unsigned_limits[] = {0, 1, 32767, 32768, 40000, 65535};
    static long long_limits[] = {1L, 32767L, 32768L, 65535L, 2147483647L};
    FILE *out;
    unsigned i;
    int never_seeded;
    long clock_value;

    out = fopen("RNG.TXT", "w");
    if (!out) return 2;
    never_seeded = rand();
    fprintf(out, "initial_without_srand=%d\n", never_seeded);
    for (i = 0; i < sizeof(seeds) / sizeof(seeds[0]); ++i)
        dump_rand(out, seeds[i]);

    srand(1);
    fprintf(out, "srand_width_seed_0001=%u\n", (unsigned)rand());
    srand(0x10001UL);
    fprintf(out, "srand_width_seed_10001=%u\n", (unsigned)rand());
    clock_value = time(NULL);
    fprintf(out, "time_value=%ld\n", clock_value);
    srand((unsigned)clock_value);
    fprintf(out, "time_seed_first=%u\n", (unsigned)rand());

    srand(12345);
    fprintf(out, "random_signed:");
    for (i = 0; i < sizeof(signed_limits) / sizeof(signed_limits[0]); ++i)
        fprintf(out, "%s%d", i ? "," : "", random(signed_limits[i]));
    fputc('\n', out);
    srand(12345);
    fprintf(out, "random_unsigned:");
    for (i = 0; i < sizeof(unsigned_limits) / sizeof(unsigned_limits[0]); ++i)
        fprintf(out, "%s%d", i ? "," : "", random(unsigned_limits[i]));
    fputc('\n', out);
    srand(12345);
    fprintf(out, "random_long:");
    for (i = 0; i < sizeof(long_limits) / sizeof(long_limits[0]); ++i)
        fprintf(out, "%s%d", i ? "," : "", random(long_limits[i]));
    fputc('\n', out);
    fclose(out);
    printf("wrote RNG.TXT\n");
    return 0;
}
