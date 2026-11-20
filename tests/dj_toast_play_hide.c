/* SPDX-License-Identifier: GPL-3.0-only */
#include "perf_toast_policy.h"
#include <stdio.h>

static int fail;

static void check(int c, const char *m)
{
    if (!c) {
        fprintf(stderr, "dj_toast_play_hide: %s\n", m);
        fail = 1;
    }
}

int main(void)
{
    fail = 0;
    check(perf_toast_policy_dj_main_live(1u, 0u, 0), "stopped: bare DJ ok");
    check(!perf_toast_policy_dj_main_live(1u, 1u, 0), "playing: hide idle DJ");
    check(perf_toast_policy_dj_main_live(1u, 1u, 1), "playing: show DJ with side");
    check(!perf_toast_policy_dj_main_live(0u, 0u, 0), "dj off");
    return fail ? 1 : 0;
}
