/* SPDX-License-Identifier: GPL-3.0-only */
#include "bass_env.h"
#include <stdio.h>

static int fail;

static void check(int c, const char *m)
{
    if (!c) {
        fprintf(stderr, "bass_dahdsr: %s\n", m);
        fail = 1;
    }
}

static int near(uint32_t a, uint32_t b, uint32_t tol)
{
    return a + tol >= b && a <= b + tol;
}

int main(void)
{
    bass_dahdsr_cfg_t cfg;
    bass_dahdsr_rt_t rt;
    int32_t lvl;
    uint32_t i, n_ad;

    fail = 0;
    bass_dahdsr_defaults(&cfg);
    bass_dahdsr_init(&rt);
    check(bass_dahdsr_step(&rt, &cfg) == 0, "fresh init step is silent");
    check(bass_hard_vca(10000, &rt) == 0, "fresh init hard VCA closed");
    check(rt.stage == BASS_ENV_IDLE, "fresh init stage idle");
    cfg.t.delay = 0u;
    cfg.t.attack = 80u;
    cfg.t.hold = 0u;
    cfg.t.decay = 8u;
    cfg.t.sustain = 64u;
    cfg.t.release = 40u;
    cfg.pol.retrigger = 0u;

    bass_dahdsr_init(&rt);
    bass_dahdsr_trig(&rt, &cfg, 100u, 0u);
    check(rt.gate_open == 1u, "trig opens gate");
    check(rt.hard_pass == 1u, "trig opens hard VCA");

    /* attack ramps over bass_dahdsr_atk_samples(); stepping its full length + decay must land in decay/sustain */
    n_ad = (uint32_t)bass_dahdsr_atk_samples(cfg.t.attack) + bass_dahdsr_decay_samples(cfg.t.decay) + 64u;
    lvl = 0;
    for (i = 0u; i < n_ad; i++) {
        int32_t s = bass_dahdsr_step(&rt, &cfg);

        if (i + 1u == (uint32_t)bass_dahdsr_atk_samples(cfg.t.attack) / 2u)
            check(s > rt.peak_q15 / 3 && s < rt.peak_q15, "attack is mid-ramp at half time");
        if (s > lvl)
            lvl = s;
    }
    check(lvl == rt.peak_q15, "attack reaches peak");
    check(lvl > 8000, "attack reaches audible Q15");
    check(rt.stage == BASS_ENV_SUSTAIN || rt.stage == BASS_ENV_DECAY, "reaches sustain/decay");
    check(bass_dahdsr_atk_samples(0u) == 0u && bass_dahdsr_atk_samples(1u) <= 4u, "attack map fine at low end");
    check(bass_dahdsr_atk_samples(127u) == 44100u, "attack map tops out at 1 s");
    for (i = 1u; i < 128u; i++)
        check(bass_dahdsr_atk_samples((uint8_t)i) >= bass_dahdsr_atk_samples((uint8_t)(i - 1u)),
              "attack map monotonic");

    bass_dahdsr_gate(&rt, &cfg, 0u);
    check(rt.gate_open == 0u, "gate off clears gate_open");
    for (i = 0u; i < bass_dahdsr_release_samples(cfg.t.release) + 64u; i++)
        bass_dahdsr_step(&rt, &cfg);
    check(rt.stage == BASS_ENV_IDLE, "release finishes to idle");
    check(rt.level_q15 == 0, "level zero after release");
    check(bass_hard_vca(10000, &rt) == 0, "hard VCA silent when idle");

    bass_dahdsr_trig(&rt, &cfg, 80u, 1u);
    check(bass_hard_vca(5000, &rt) == 5000, "hard VCA passes while gate open");
    bass_dahdsr_gate(&rt, &cfg, 0u);
    for (i = 0u; i < bass_dahdsr_release_samples(cfg.t.release) + 64u; i++)
        (void)bass_dahdsr_step(&rt, &cfg);
    check(bass_hard_vca(5000, &rt) == 0, "hard VCA zero after release");

    cfg.pol.invert = 1u;
    bass_dahdsr_init(&rt);
    bass_dahdsr_trig(&rt, &cfg, 100u, 0u);
    for (i = 0u; i < (uint32_t)bass_dahdsr_atk_samples(cfg.t.attack) * 3u / 4u; i++)
        lvl = bass_dahdsr_step(&rt, &cfg);
    check(lvl < 32767 - 8000, "invert lowers mod level during rise");

    /* ATK 0: peak on the trigger sample, no onset delay */
    cfg.pol.invert = 0u;
    cfg.t.attack = 0u;
    bass_dahdsr_init(&rt);
    bass_dahdsr_trig(&rt, &cfg, 100u, 0u);
    check(bass_dahdsr_step(&rt, &cfg) == rt.peak_q15, "attack 0 is instant");

    /* time maps: delay/hold 0 … 1 s, decay 2 ms … 4 s, release 2 ms … 6 s, all monotonic */
    check(bass_dahdsr_1s_samples(0u) == 0u && bass_dahdsr_1s_samples(127u) == 44100u, "delay/hold 0 … 1 s");
    check(near(bass_dahdsr_decay_samples(0u), 88u, 2u), "decay 0 ≈ 2 ms");
    check(near(bass_dahdsr_decay_samples(127u), 176400u, 1800u), "decay 127 ≈ 4 s");
    check(near(bass_dahdsr_release_samples(0u), 88u, 2u), "release 0 ≈ 2 ms");
    check(near(bass_dahdsr_release_samples(127u), 264600u, 2700u), "release 127 ≈ 6 s");
    for (i = 1u; i < 128u; i++) {
        check(bass_dahdsr_1s_samples((uint8_t)i) >= bass_dahdsr_1s_samples((uint8_t)(i - 1u)), "delay/hold monotonic");
        check(bass_dahdsr_decay_samples((uint8_t)i) > bass_dahdsr_decay_samples((uint8_t)(i - 1u)), "decay monotonic");
        check(bass_dahdsr_release_samples((uint8_t)i) > bass_dahdsr_release_samples((uint8_t)(i - 1u)),
              "release monotonic");
    }

    /* delay/hold run their real length */
    cfg.t.delay = 64u;
    cfg.t.attack = 0u;
    cfg.t.hold = 64u;
    cfg.t.decay = 0u;
    cfg.t.sustain = 0u;
    bass_dahdsr_init(&rt);
    bass_dahdsr_trig(&rt, &cfg, 127u, 0u);
    i = 0u;
    while (rt.stage == BASS_ENV_DELAY && i < 100000u) {
        (void)bass_dahdsr_step(&rt, &cfg);
        i++;
    }
    check(near(i, bass_dahdsr_1s_samples(64u), 2u), "delay lasts its mapped time");
    i = 0u;
    while (rt.stage == BASS_ENV_HOLD && i < 100000u) {
        (void)bass_dahdsr_step(&rt, &cfg);
        i++;
    }
    check(near(i, bass_dahdsr_1s_samples(64u), 2u), "hold lasts its mapped time");

    /* measured decay (full peak → sustain 0) and release (full → idle) track the map */
    cfg.t.delay = 0u;
    cfg.t.hold = 0u;
    {
        static const uint8_t vs[] = {0u, 32u, 64u, 80u, 96u, 127u};
        uint32_t k, prev_d = 0u, prev_r = 0u;

        for (k = 0u; k < sizeof vs; k++) {
            uint32_t nd, nr, want;
            int32_t prev;
            int mono = 1;

            cfg.t.decay = vs[k];
            cfg.t.release = vs[k];
            cfg.t.sustain = 0u;
            bass_dahdsr_init(&rt);
            bass_dahdsr_trig(&rt, &cfg, 127u, 0u);
            (void)bass_dahdsr_step(&rt, &cfg); /* hold → decay */
            nd = 0u;
            prev = rt.level_q15;
            while (rt.stage == BASS_ENV_DECAY && nd < 400000u) {
                (void)bass_dahdsr_step(&rt, &cfg);
                if (rt.level_q15 > prev)
                    mono = 0;
                prev = rt.level_q15;
                nd++;
            }
            want = bass_dahdsr_decay_samples(vs[k]);
            check(rt.stage == BASS_ENV_SUSTAIN, "decay ends in sustain");
            check(mono, "decay curve monotonic");
            check(nd > want * 9u / 10u && nd < want * 11u / 10u + 4u, "decay time ±10 % of map");
            check(nd > prev_d, "measured decay grows with value");
            prev_d = nd;

            cfg.t.sustain = 127u;
            cfg.t.decay = 0u;
            bass_dahdsr_init(&rt);
            bass_dahdsr_trig(&rt, &cfg, 127u, 0u);
            for (i = 0u; i < 400u && rt.stage != BASS_ENV_SUSTAIN; i++)
                (void)bass_dahdsr_step(&rt, &cfg);
            check(rt.stage == BASS_ENV_SUSTAIN, "decay 0 reaches sustain 127 fast");
            bass_dahdsr_gate(&rt, &cfg, 0u);
            nr = 0u;
            prev = rt.level_q15;
            while (rt.stage != BASS_ENV_IDLE && nr < 600000u) {
                (void)bass_dahdsr_step(&rt, &cfg);
                if (rt.stage != BASS_ENV_IDLE && rt.level_q15 > prev)
                    mono = 0;
                if (rt.stage == BASS_ENV_IDLE && prev > BASS_DAHDSR_TOL_Q15 + 1)
                    mono = 0; /* last jump to 0 must stay below −60 dBFS */
                prev = rt.level_q15;
                nr++;
            }
            want = bass_dahdsr_release_samples(vs[k]);
            check(rt.stage == BASS_ENV_IDLE && rt.level_q15 == 0 && rt.hard_pass == 0u, "release ends in idle");
            check(mono, "release monotonic, click-free end");
            check(nr > want * 9u / 10u && nr < want * 11u / 10u + 4u, "release time ±10 % of map");
            check(nr > prev_r, "measured release grows with value");
            prev_r = nr;
        }
    }

    /* release from mid-decay and from delay also lands in idle */
    cfg.t.decay = 127u;
    cfg.t.release = 0u;
    cfg.t.sustain = 0u;
    bass_dahdsr_init(&rt);
    bass_dahdsr_trig(&rt, &cfg, 127u, 0u);
    for (i = 0u; i < 2000u; i++)
        (void)bass_dahdsr_step(&rt, &cfg);
    bass_dahdsr_gate(&rt, &cfg, 0u);
    for (i = 0u; i < 2000u; i++)
        (void)bass_dahdsr_step(&rt, &cfg);
    check(rt.stage == BASS_ENV_IDLE && rt.hard_pass == 0u, "release from decay ends in idle");
    cfg.t.delay = 127u;
    bass_dahdsr_trig(&rt, &cfg, 127u, 0u);
    bass_dahdsr_gate(&rt, &cfg, 0u);
    (void)bass_dahdsr_step(&rt, &cfg);
    check(rt.stage == BASS_ENV_IDLE, "release during delay ends in idle");

    return fail ? 1 : 0;
}
