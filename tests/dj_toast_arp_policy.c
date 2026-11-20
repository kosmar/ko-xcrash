/* SPDX-License-Identifier: GPL-3.0-only */
#include "perf_toast_policy.h"
#include <stdio.h>
#include <string.h>

static int fail;

static void check(int c, const char *m)
{
    if (!c) {
        fprintf(stderr, "dj_toast_arp_policy: %s\n", m);
        fail = 1;
    }
}

int main(void)
{
    static const char *const ARP[] = {"JACK", "CHOP", "DOT", "ROLL", "LOOP"};
    perf_toast_slot_t st[3];

    fail = 0;
    check(perf_toast_policy_manual_arp_ok(0u, 3u), "dj off: manual arp toast ok");
    check(perf_toast_policy_manual_arp_ok(1u, 3u), "dj on: manual arp toast still pages");
    check(!perf_toast_policy_manual_arp_ok(0u, 0u), "arp 0 blocked");
    check(!perf_toast_policy_manual_arp_ok(1u, 0u), "dj on: arp 0 blocked");
    check(perf_toast_policy_dj_arp_overlay_live(1u, 1u, 2u, 4u), "dj arp overlay live");
    check(!perf_toast_policy_dj_arp_overlay_live(1u, 1u, 0u, 4u), "dj arp overlay ended");
    check(!perf_toast_policy_dj_arp_overlay_live(1u, 0u, 2u, 4u), "dj arp overlay flag off");
    check(perf_toast_policy_dj_loop_overlay_live(1u, 2u, 1u), "dj loop overlay live");
    check(!perf_toast_policy_dj_loop_overlay_live(1u, 2u, 0u), "dj loop overlay ended");

    st[0].msg[0] = 'L';
    st[0].msg[1] = 'O';
    st[0].msg[2] = 'O';
    st[0].msg[3] = 'P';
    st[0].msg[4] = 0;
    strcpy(st[1].msg, "DRUM FLARE");
    st[2].msg[0] = 'L';
    st[2].msg[1] = 'O';
    st[2].msg[2] = 'O';
    st[2].msg[3] = 'P';
    st[2].msg[4] = 0;
    check(perf_toast_policy_strip_dj_arp_labels(st, 3u, "LOOP", ARP, 5u) == 3u, "allow LOOP keeps gestures + LOOP rows");
    check(perf_toast_policy_same_msg(st[1].msg, "DRUM FLARE"), "gesture retained");
    st[0].msg[0] = 'R';
    st[0].msg[1] = 'O';
    st[0].msg[2] = 'L';
    st[0].msg[3] = 'L';
    st[0].msg[4] = 0;
    st[1].msg[0] = 'L';
    st[1].msg[1] = 'O';
    st[1].msg[2] = 'O';
    st[1].msg[3] = 'P';
    st[1].msg[4] = 0;
    check(perf_toast_policy_strip_dj_arp_labels(st, 2u, "LOOP", ARP, 5u) == 1u, "stale ROLL removed when LOOP live");
    check(perf_toast_policy_same_msg(st[0].msg, "LOOP"), "LOOP retained");
    strcpy(st[0].msg, "DRUM FLARE");
    st[1].msg[0] = 'L';
    st[1].msg[1] = 'O';
    st[1].msg[2] = 'O';
    st[1].msg[3] = 'P';
    st[1].msg[4] = 0;
    check(perf_toast_policy_strip_dj_arp_labels(st, 2u, 0, ARP, 5u) == 1u, "dj on no overlay strips arp labels");
    check(perf_toast_policy_same_msg(st[0].msg, "DRUM FLARE"), "only gesture left");
    check(perf_toast_policy_count_sticky(st, 1u, ARP, 5u) == 1u, "one sticky perf row");
    return fail ? 1 : 0;
}
