/* SPDX-License-Identifier: GPL-3.0-only */
#include "bass_data.h"
#include <stdio.h>
#include <string.h>

static int fail;

static void check(int c, const char *m)
{
    if (!c) {
        fprintf(stderr, "bass_bake_lock: %s\n", m);
        fail = 1;
    }
}

int main(void)
{
    bass_step_t steps[16];
    bass_step_t locked_snap;
    uint8_t i;
    uint8_t changed;

    fail = 0;
    bass_step_defaults(steps, 16u);
    for (i = 0u; i < 16u; i++) {
        steps[i].gate = 7u;
        steps[i].note = 99u;
        steps[i].accent = 1u;
        steps[i].slide = 1u;
        steps[i].ratchet = 3u;
    }
    steps[4].lock = 1u;
    locked_snap = steps[4];
    bass_genre_bake(steps, 16u, 0u, ERA_ACID);
    check(steps[4].gate == locked_snap.gate, "locked step gate untouched");
    check(steps[4].note == locked_snap.note, "locked step note untouched");
    check(steps[4].accent == locked_snap.accent, "locked step accent untouched");
    check(steps[4].slide == locked_snap.slide, "locked step slide untouched");
    check(steps[4].ratchet == locked_snap.ratchet, "locked step ratchet untouched");
    check(steps[4].lock == 1u, "locked flag preserved");
    changed = 0u;
    for (i = 0u; i < 16u; i++) {
        if (i == 4u)
            continue;
        if (steps[i].gate != 7u || steps[i].note != 99u || !steps[i].accent || !steps[i].slide || steps[i].ratchet != 3u)
            changed = 1u;
    }
    check(changed, "unlocked steps rebaked from style tables");
    bass_step_edit_locks_step(&steps[2]);
    check(steps[2].lock == 1u, "edit sets lock");
    bass_step_long_clear_unlock(steps, 16u);
    check(steps[2].lock == 0u && steps[4].lock == 0u, "long clear unlocks all");
    printf("bass_bake_lock: OK\n");
    return fail ? 1 : 0;
}
