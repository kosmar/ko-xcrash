/* SPDX-License-Identifier: GPL-3.0-only */
#include "fm_dj_gest.h"
#include <stdio.h>

static int fail;

static void check(int c, const char *m)
{
    if (!c) {
        fprintf(stderr, "dj_gest_arp_lifecycle: %s\n", m);
        fail = 1;
    }
}

int main(void)
{
    fm_dj_gest_st_t s;
    uint8_t arp = 5u;
    uint8_t loop_end = 0u;

    fail = 0;
    fm_dj_gest_st_reset(&s);
    check(!fm_dj_gest_st_active(&s), "empty inactive");
    s.ov = FM_DJ_GEST_OV_ARP;
    s.bars = 2u;
    s.saved_arp = 0u;
    check(fm_dj_gest_st_active(&s), "arp overlay active");
    s.bars = 0u;
    check(fm_dj_gest_st_active(&s), "arp ov still active at 0 bars until restore");
    check(fm_dj_gest_st_restore(&s, &arp, 0u, &loop_end), "restore arp overlay");
    check(arp == 0u, "arp mode restored to saved 0");
    check(!fm_dj_gest_st_active(&s), "cleared after restore");
    arp = 3u;
    s.ov = FM_DJ_GEST_OV_LOOP;
    s.bars = 1u;
    s.saved_loop = 0u;
    s.saved_arp = 3u;
    check(fm_dj_gest_st_restore(&s, &arp, 1u, &loop_end), "loop overlay restore");
    check(loop_end == 1u, "loop end pending");
    check(arp == 3u, "arp restored from snapshot on loop overlay end");
    {
        uint8_t pending = 1u;
        uint8_t endp = 0u;
        uint8_t bars = fm_dj_gest_plan_bars(FM_DJ_GEST_LOOP, 1u);

        while (bars > 0u)
            bars--;
        fm_dj_gest_loop_overlay_teardown(1u, &endp, &pending);
        check(endp == 1u && pending == 0u, "loop span teardown stops engaged looper");
        pending = 1u;
        endp = 0u;
        fm_dj_gest_loop_overlay_teardown(0u, &endp, &pending);
        check(endp == 0u && pending == 0u, "overlay end before engage drops pending");
    }
    s.kind = 3u;
    s.bars = 2u;
    s.flam_steps = 0u;
    s.ov = FM_DJ_GEST_OV_NONE;
    s.saved_arp = 1u;
    arp = 5u;
    check(fm_dj_gest_st_restore(&s, &arp, 0u, &loop_end), "solo gesture restore");
    check(arp == 1u, "gesture restores saved arp");
    return fail ? 1 : 0;
}
