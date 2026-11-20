/* SPDX-License-Identifier: GPL-3.0-only */
#include "perf_toast_policy.h"
#include <stdio.h>
#include <string.h>

static int fail;

static void check(int c, const char *m)
{
    if (!c) {
        fprintf(stderr, "dj_toast_loop_policy: %s\n", m);
        fail = 1;
    }
}

int main(void)
{
    fail = 0;
    check(perf_toast_policy_dj_loop_audible(1u, 2u, 2u, 1u, 0u), "loop audible when active");
    check(perf_toast_policy_dj_loop_audible(1u, 2u, 2u, 0u, 1u), "loop audible when pending");
    check(!perf_toast_policy_dj_loop_audible(1u, 2u, 2u, 0u, 0u), "gest window alone not audible");
    check(!perf_toast_policy_dj_loop_audible(1u, 2u, 0u, 1u, 0u), "no toast after gest bars end");
    check(!perf_toast_policy_dj_loop_audible(1u, 0u, 2u, 1u, 0u), "wrong overlay kind");

    check(perf_toast_policy_key_toast_is_loop_len(PERF_TOAST_KEY_TOAST_LOOP_LEN_LN), "loop len lane id");
    check(!perf_toast_policy_key_toast_is_loop_len(4u), "voice lane not loop len");
    {
        char side[20];

        perf_toast_policy_dj_loop_side(side, sizeof side, "3/1");
        check(!strcmp(side, "LOOP 3/1"), "loop side prefixes LOOP");
    }
    return fail ? 1 : 0;
}
