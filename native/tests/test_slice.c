/* Native regression receipts, NOT DOS differential evidence. FNV-1a over the
 * complete indexed VRAM and 768 DAC bytes, seed 1, VGA 400, speed 1, 10000 steps
 * after the original rebuild-and-plot call. See native/README.md. */
#include "dazzle.h"
#include "platform/platform.h"
#include <assert.h>
#include <inttypes.h>

static void run(const char *name, uint32_t expected, uint64_t calls)
{
    dz_options = (struct dz_options){1, 1, 1, 1.0, NULL};
    assert(dz_initialize());
    video_adapter = 8; bios_resolution_mode = 0;
    assert(dz_slice_begin(name));
    for (int i = 0; i < 10000; ++i) dz_slice_step();
    uint32_t pixels = dz_framebuffer_hash(), palette = dz_palette_hash();
    printf("%s: framebuffer=%08" PRIx32 " palette=%08" PRIx32 " rng=%" PRIu64 " ticks=%" PRIu64 "\n",
        name, pixels, palette, dz_rand_calls(), dz_virtual_ticks());
    assert(pixels == expected && palette == UINT32_C(0x31b01fc5));
    assert(dz_rand_calls() == calls && dz_virtual_ticks() == 34);
    /* Cycling changes existing scanout while preserving the indexed image. */
    dz_delay_ms(60); assert(palette_elapsed_ticks >= palette_cycle_ticks);
    cycle_palette();
    assert(dz_palette_hash() != palette && dz_framebuffer_hash() == pixels);
    /* The close event uses the real slice's Escape dispatcher. */
    SDL_Event event = {0}; event.type = SDL_EVENT_QUIT; dz_input_event(&event);
    assert(input_pending && !exit_requested);
    dz_slice_step(); assert(exit_requested);
    dz_slice_end(); dz_shutdown();
}
int main(void)
{
    run("sierpinski", UINT32_C(0x6abc9dc5), 10017);
    run("lines", UINT32_C(0x1f227748), 38);
    return 0;
}
