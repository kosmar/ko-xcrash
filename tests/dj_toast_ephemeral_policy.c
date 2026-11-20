/* SPDX-License-Identifier: GPL-3.0-only */
#include "perf_toast_policy.h"
#include <stdio.h>

static int fail;

static void check(int c, const char *m)
{
    if (!c) {
        fprintf(stderr, "dj_toast_ephemeral_policy: %s\n", m);
        fail = 1;
    }
}

int main(void)
{
    perf_toast_slot_t stack[3];
    static const char *const AN[5] = {"JACK", "CHOP", "DOT", "ROLL", "LOOP"};
    uint8_t n;

    fail = 0;
    check(perf_toast_policy_gesture_ephemeral_msg("FLAM!"), "FLAM ephemeral");
    check(perf_toast_policy_gesture_ephemeral_msg("BREAK"), "BREAK ephemeral");
    check(perf_toast_policy_gesture_ephemeral_msg("FILL"), "FILL ephemeral");
    check(perf_toast_policy_gesture_ephemeral_msg("BASS FLARE"), "BASS FLARE ephemeral");
    check(perf_toast_policy_dj_absorbs_gesture_msg("FLAM!", 1u), "DJ absorbs FLAM");
    check(perf_toast_policy_dj_absorbs_gesture_msg("FILL", 1u), "DJ absorbs FILL");
    check(perf_toast_policy_dj_absorbs_gesture_msg("DRUM FLARE", 1u), "DJ absorbs flare");
    check(!perf_toast_policy_dj_absorbs_gesture_msg("FLAM!", 0u), "no absorb DJ off");
    check(!perf_toast_policy_dj_absorbs_gesture_msg("DRUM FLARE", 0u), "no absorb DJ off flare");

    stack[0].msg[0] = 'D';
    stack[0].msg[1] = 'J';
    stack[0].msg[2] = 0;
    stack[1].msg[0] = 'F';
    stack[1].msg[1] = 'I';
    stack[1].msg[2] = 'L';
    stack[1].msg[3] = 'L';
    stack[1].msg[4] = 0;
    n = 2u;
    check(perf_toast_policy_count_sticky(stack, n, AN, 5u) == 2u, "two sticky rows without dedupe");
    n = perf_toast_policy_evict_other_sticky(stack, n, "DJ", AN, 5u);
    check(n == 1u, "DJ wins over absorbed FILL sticky row");
    check(perf_toast_policy_same_msg(stack[0].msg, "DJ"), "DJ row kept");

    return fail ? 1 : 0;
}
