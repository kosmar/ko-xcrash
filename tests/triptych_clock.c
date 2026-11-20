/* SPDX-License-Identifier: GPL-3.0-only */
/* Internal MIDI clock: 60 s at 120 and 187 BPM, 1 ms and 7 ms poll steps.
 * Build: mkdir -p build/host && cc -O2 -Wall -Ifirmware/src -o build/host/triptych_clock \\
 *        tests/triptych_clock.c && build/host/triptych_clock */
#include <stdint.h>
#include <stdio.h>

#include "midi_ctrl.h"

static int fails;

#define CHECK(cond, ...)                 \
    do {                                 \
        if (!(cond)) {                   \
            fails++;                     \
            fprintf(stderr, __VA_ARGS__); \
            fprintf(stderr, "\n");       \
        }                                 \
    } while (0)

#define MINUTE_FRAMES MIDI_CLOCK_MIN_FRAMES

static uint32_t frames_for_ms(uint32_t ms)
{
    return (ms * MIDI_CLOCK_HZ) / 1000u;
}

static uint32_t run_minute_frames(midi_cfg_t *c, midi_state_t *s, uint32_t step_frames)
{
    uint32_t now, total = 0u;

    midi_state_init(s, c);
    s->playing = 1u;
    s->ext_sync = 0u;
    s->int_last_frames = 0u;
    for (now = step_frames; now <= MINUTE_FRAMES; now += step_frames) {
        midi_clock_poll_frames(c, s, now);
        total += midi_clock_poll(c, s, (now * 1000u) / MIDI_CLOCK_HZ);
    }
    return total;
}

static void assert_tempo_frames(uint8_t tempo_val, uint32_t step_ms)
{
    midi_cfg_t c;
    midi_state_t s;
    uint32_t bpm, want, got, step_frames;

    midi_cfg_defaults(&c);
    c.val[P_TEMPO] = tempo_val;
    c.base_val[P_TEMPO] = tempo_val;
    c.val[P_SWING] = 0u;
    c.base_val[P_SWING] = 0u;
    bpm = 60u + (uint32_t)tempo_val;
    want = bpm * 24u;
    step_frames = frames_for_ms(step_ms);
    if (step_frames < 1u)
        step_frames = 1u;
    got = run_minute_frames(&c, &s, step_frames);
    CHECK(got >= want - 1u && got <= want + 1u,
          "BPM %u step %u ms (%u frames): got %u ticks, want %u +/- 1", bpm, step_ms, step_frames,
          got, want);
}

int main(void)
{
    assert_tempo_frames(60u, 1u);  /* 120 BPM */
    assert_tempo_frames(60u, 7u);
    assert_tempo_frames(127u, 1u); /* 187 BPM */
    assert_tempo_frames(127u, 7u);

    if (fails) {
        fprintf(stderr, "triptych_clock: %d failure(s)\n", fails);
        return 1;
    }
    printf("triptych_clock: OK\n");
    return 0;
}
