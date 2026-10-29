#include "dazzle.h"
#include "ref/native_util.h"

int main(void)
{
    static const int32_t limits[3][7] = {
        {0, 1, 2, 3, 32767, -1, -2},
        {0, 1, 32767, 32768, 40000, 65535},
        {1, 32767, 32768, 65535, INT32_MAX}
    };
    static const unsigned lengths[] = {7, 6, 5};
    FILE *file = fopen(ref_path("data/rng.txt"), "r");
    char line[16000];
    unsigned values = 0, vectors = 0;
    int32_t captured_time = 0;
    assert(file);
    while (fgets(line, sizeof line, file)) {
        unsigned seed, expected, count;
        if (sscanf(line, "rand seed=%x:", &seed) == 1) {
            char *cursor = strchr(line, ':') + 1, *end;
            dz_srand(seed);
            count = 0;
            do {
                expected = (unsigned)strtoul(cursor, &end, 10);
                int actual = dz_rand();
                if (actual != (int)expected) {
                    fprintf(stderr, "seed=%04x index=%u DOS=%u native=%d\n", seed, count, expected, actual);
                    return 1;
                }
                ++count; cursor = end + 1;
            } while (*end == ',');
            assert(count == 2000 && dz_rand_calls() == 2000);
            values += count; ++vectors;
        } else if (!strncmp(line, "random_", 7)) {
            unsigned group = !strncmp(line, "random_signed:", 14) ? 0 :
                !strncmp(line, "random_unsigned:", 16) ? 1 : 2;
            char *cursor = strchr(line, ':') + 1, *end;
            dz_srand(12345);
            for (unsigned i = 0; i < lengths[group]; ++i) {
                int actual = dz_random(limits[group][i]);
                int expected_signed = (int)strtol(cursor, &end, 10);
                assert(actual == expected_signed);
                cursor = end + 1; ++values;
            }
            assert(*end == '\r' || *end == '\n' || !*end);
        } else if (sscanf(line, "time_value=%" SCNd32, &captured_time) == 1) {
            /* Replay the captured time, never consult the host wall clock. */
        } else if (sscanf(line, "time_seed_first=%u", &expected) == 1) {
            dz_srand((uint16_t)captured_time); assert(dz_rand() == (int)expected);
        } else if (sscanf(line, "srand_width_seed_10001=%u", &expected) == 1) {
            dz_srand(0x10001); assert(dz_rand() == (int)expected);
        } else if (sscanf(line, "srand_width_seed_0001=%u", &expected) == 1) {
            dz_srand(1); assert(dz_rand() == (int)expected);
        } else if (sscanf(line, "initial_without_srand=%u", &expected) == 1) {
            assert(dz_rand() == (int)expected && dz_rand_calls() == 1);
        }
    }
    assert(vectors == 4 && values == 8018);
    fclose(file);
    printf("%u DOS RNG results equal (including signed/unsigned/long random)\n", values);
    return 0;
}
