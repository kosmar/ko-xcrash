/* SPDX-License-Identifier: GPL-3.0-only */
/* KO BASS tempo + INT+TX clock, driven sample by sample through the real app_audio():
 *  - step period averaged over swing pairs must equal FS*60/(bpm*4) exactly (no rounding drift)
 *  - INT+TX F8 spacing must average FS*60/(bpm*24)
 *  - clock-mode switches while playing: FC when leaving INT+TX, FA at the next pair start when
 *    entering it, EXT stops the internal transport with no hanging gate.
 * Build: cc -DDINGHY_HOST -DGENRE_BASS -Ifirmware/src -Ifirmware/gen tests/bass_tempo.c */
#define main bass_tempo_host_main
#include "fm_app.c"
#undef main

static int fail;
static const char *midi_log = "build/host/bass_tempo_midi.log";
static long midi_log_pos;

static void check(int c, const char *m)
{
    if (!c) {
        fprintf(stderr, "bass_tempo: FAIL %s\n", m);
        fail = 1;
    }
}

/* counts "MIDI out XX" lines printed since the last call */
static void midi_log_take(uint32_t *fa, uint32_t *fc, uint32_t *f8)
{
    FILE *f;
    char line[128];

    fflush(stdout);
    *fa = *fc = *f8 = 0u;
    f = fopen(midi_log, "r");
    if (!f)
        return;
    fseek(f, midi_log_pos, SEEK_SET);
    while (fgets(line, sizeof line, f)) {
        if (strstr(line, "MIDI out FA"))
            (*fa)++;
        else if (strstr(line, "MIDI out FC"))
            (*fc)++;
        else if (strstr(line, "MIDI out F8"))
            (*f8)++;
    }
    midi_log_pos = ftell(f);
    fclose(f);
}

static void run_samples(uint32_t n)
{
    static int32_t blk[2];
    uint32_t i;

    for (i = 0; i < n; i++)
        app_audio(blk, 1u);
}

static void play_from_start(uint8_t bpm, uint8_t swing)
{
    g.playing = 0u;
    g.cfg.p.bpm = bpm;
    g.cfg.p.swing = swing;
    dub_morph_sync_field(&g.cfg.p.bpm);
    dub_morph_sync_field(&g.cfg.p.swing);
    bass_clock_mode_set(BASS_CLOCK_SEND);
    sync_voice_params();
    g.playing = 1u;
    transport_reset();
}

static void tempo_case(uint8_t bpm, uint8_t swing)
{
    const double exact = (double)DUB_FS * 60.0 / ((double)bpm * 4.0);
    const uint32_t steps = 32u;
    uint32_t t = 0u, ticks = 0u, t0 = 0u, t_end = 0u, prev_acc;
    uint32_t pulses = 0u, p_first = 0u, p_last = 0u;
    uint8_t prev_pc;
    static int32_t blk[2];
    double step_err, f8_mean, f8_want;

    play_from_start(bpm, swing);
    prev_acc = g.sample_acc;
    prev_pc = g.bass_rt.clk_pc;
    while (ticks <= steps && t < (uint32_t)(exact * (steps + 4u))) {
        uint8_t pc;
        uint32_t sent;

        app_audio(blk, 1u);
        t++;
        if (g.sample_acc < prev_acc + 1u) {
            if (ticks == 0u)
                t0 = t;
            if (ticks == steps)
                t_end = t;
            ticks++;
        }
        prev_acc = g.sample_acc;
        pc = g.bass_rt.clk_pc;
        sent = pc >= prev_pc ? (uint32_t)(pc - prev_pc) : (uint32_t)(BASS_CLK_PAIR - prev_pc + pc);
        if (prev_pc == BASS_CLK_PAIR && pc < BASS_CLK_PAIR)
            sent = pc;
        if (sent) {
            if (!pulses)
                p_first = t;
            p_last = t;
            pulses += sent;
        }
        prev_pc = pc;
    }
    check(g.voice.p.bpm == bpm, "voice bpm differs from cfg bpm (LFO/morph on tempo?)");
    step_err = (double)(t_end - t0) - exact * steps;
    f8_want = (double)DUB_FS * 60.0 / ((double)bpm * 24.0);
    f8_mean = pulses > 1u ? (double)(p_last - p_first) / (double)(pulses - 1u) : 0.0;
    fprintf(stderr,
            "bass_tempo: %3u BPM swing %2u  %u steps = %u smp (exact %.1f, err %+.2f)  "
            "F8 %u, mean %.3f smp = %.3f ms (want %.3f ms)\n",
            bpm, swing, steps, t_end - t0, exact * steps, step_err, pulses, f8_mean,
            f8_mean * 1000.0 / DUB_FS, f8_want * 1000.0 / DUB_FS);
    check(ticks > steps, "too few steps");
    check(step_err > -1.5 && step_err < 1.5, "step period drifts from BPM");
    check(f8_mean > f8_want * 0.999 && f8_mean < f8_want * 1.001, "INT+TX F8 spacing off");
}

static void mode_switch_case(void)
{
    const uint32_t spp = DUB_FS * 60u / (120u * 4u);
    uint32_t fa, fc, f8;

    play_from_start(120u, 50u);
    run_samples(spp * 3u);
    midi_log_take(&fa, &fc, &f8);
    check(f8 > 0u, "INT+TX sends F8");

    bass_clock_mode_set(BASS_CLOCK_INT);
    midi_log_take(&fa, &fc, &f8);
    check(fc == 1u, "INT+TX -> INT while playing must send one FC");
    check(g.playing == 1u, "INT keeps playing");
    run_samples(spp * 3u);
    midi_log_take(&fa, &fc, &f8);
    check(f8 == 0u && fa == 0u && fc == 0u, "INT sends no clock");

    bass_clock_mode_set(BASS_CLOCK_SEND);
    midi_log_take(&fa, &fc, &f8);
    check(fa == 0u && f8 == 0u, "INT -> INT+TX waits for the next pair start");
    run_samples(spp * 3u);
    midi_log_take(&fa, &fc, &f8);
    check(fa == 1u, "INT -> INT+TX sends one FA");
    check(f8 >= BASS_CLK_PPS, "INT+TX resumes F8 after FA");

    run_samples(spp / 3u);
    g.bass_rt.gate_left = 1000u;
    bass_clock_mode_set(BASS_CLOCK_EXT);
    midi_log_take(&fa, &fc, &f8);
    check(fc == 1u, "INT+TX -> EXT while playing must send one FC");
    check(g.playing == 0u, "EXT stops the internal transport");
    check(g.bass_rt.gate_left == 0u && g.synth.sub_on == 0u, "EXT switch leaves no hanging gate");
    run_samples(spp * 2u);
    midi_log_take(&fa, &fc, &f8);
    check(f8 == 0u, "EXT sends no clock");
    fprintf(stderr, "bass_tempo: clock-mode switches %s\n", fail ? "FAILED" : "OK");
}

int main(void)
{
    if (!freopen(midi_log, "w", stdout)) {
        perror(midi_log);
        return 1;
    }
    app_init();
    tempo_case(120u, 50u);
    tempo_case(120u, 62u);
    tempo_case(124u, 50u);
    tempo_case(124u, 62u);
    tempo_case(174u, 58u);
    mode_switch_case();
    if (fail)
        return 1;
    fprintf(stderr, "bass_tempo: OK\n");
    return 0;
}
