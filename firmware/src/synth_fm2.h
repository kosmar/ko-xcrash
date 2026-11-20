/* SPDX-License-Identifier: GPL-3.0-only */
/* 4-voice 2-operator FM + stereo delay (Q15 integer, no float). Include midi_ctrl.h first. */
#ifndef SYNTH_FM2_H
#define SYNTH_FM2_H

#include <stdint.h>

#ifndef FM2_DX7_TIMBRE
#define FM2_DX7_TIMBRE 1
#endif

#define NV 4u
#define ECHO_LEN 8192u /* ~185 ms at 44.1 kHz, 16 KB in .pool */

#if defined(DINGHY_HOST) || defined(DINGHY_WEB)
static int16_t echo_buf[ECHO_LEN];
#else
static int16_t echo_buf[ECHO_LEN] __attribute__((section(".pool")));
#endif

static const uint32_t INC_C9[12] = {
    815363807u, 863847862u, 915214929u, 969636441u, 1027294024u, 1088380105u,
    1153098554u, 1221665363u, 1294309365u, 1371273005u, 1452813141u, 1539201906u};

static struct {
    volatile uint32_t c_inc, m_inc;
    volatile int32_t gate;
    int32_t amp, mod_env;
    uint32_t c_ph, m_ph, age;
    uint8_t note, vel;
} voice[NV];

static uint32_t voice_c_ph_r[NV];
static uint32_t voice_sub_ph[NV];
static uint32_t voice_base_inc[NV];
static int32_t voice_timbre_ofs[NV];
static uint32_t voice_decay_mul_q8[NV];
static int32_t voice_amp_q8[NV];
static int32_t voice_mod_q8[NV];
static int32_t voice_fb1[NV];
static int32_t voice_fb2[NV];
static uint8_t voice_stage[NV]; /* 0 = release, 1 = attack, 2 = decay */
static uint32_t echo_wr;
static uint32_t synth_ages;
static int32_t fm2_drive_lp_l;
static int32_t fm2_drive_lp_r;
static uint32_t echo_dly_cur = 2048u;
static int32_t echo_lp;

static int32_t fm2_sine(uint32_t ph)
{
    int32_t x = (int32_t)ph >> 16, ax = x < 0 ? -x : x, y, ay;

    y = (x * (32768 - ax)) >> 13;
    ay = y < 0 ? -y : y;
    y += ((((y * ay) >> 15) - y) * 7373) >> 15;
    return y > 32767 ? 32767 : y < -32767 ? -32767 : y;
}

/* Strictly monotonic Q15 warm saturation: ±32767 → ±32767, no foldback. */
static int32_t fm2_warm_sat(int32_t x)
{
    int32_t x2;

    if (x > 32767)
        x = 32767;
    else if (x < -32767)
        x = -32767;
    x2 = (x * x) >> 15;
    return (x * ((98304 - x2) >> 1)) >> 15;
}

static int32_t fm2_soft_clip(int32_t x, uint32_t drive)
{
    int32_t g, makeup, ax;

    /* −6 dB bus headroom before drive (4-voice Cyclic chords). */
    x >>= 1;

    g = 256 + ((int32_t)drive << 1);
    /* Hold perceived level as harmonic density increases. */
    makeup = 288 - ((int32_t)drive >> 1);

    ax = x < 0 ? -x : x;
    if (ax > 262144)
        x = x > 0 ? 262144 : -262144;
    x = ((x >> 4) * g) >> 4;
    if (x > 32767)
        x = 32767;
    else if (x < -32767)
        x = -32767;

    x = fm2_warm_sat(x);
    x = (x * makeup) >> 8;
    if (x > 32767)
        x = 32767;
    else if (x < -32767)
        x = -32767;
    return x;
}

static void synth_note_on_ex(uint32_t voice_hint, uint32_t note, uint32_t vel, int32_t timbre_ofs,
                             int32_t decay_mul_q8, const midi_cfg_t *c, uint32_t *last_note_out)
{
    uint32_t v, pick = 0, best = 0xFFFFFFFFu;
    uint32_t base_inc, mul_q8, cm, mm_q8;

    if (note > 127u || !vel)
        return;

    if (voice_hint < NV && (!voice[voice_hint].gate || voice[voice_hint].note == (uint8_t)note))
        pick = voice_hint;
    else {
        for (v = 0; v < NV; v++) {
            uint32_t score = voice[v].note == note && voice[v].gate ? 0u
                           : voice[v].gate ? 0x80000000u + voice[v].age
                                           : (uint32_t)voice[v].amp + 1u;
            if (score < best) {
                best = score;
                pick = v;
            }
        }
    }

    midi_ratio_cm_q8(c->val[P_RATIO], &cm, &mm_q8);

    mul_q8 = (uint32_t)decay_mul_q8;
    if (mul_q8 < 16u)
        mul_q8 = 16u;
    else if (mul_q8 > 1024u)
        mul_q8 = 1024u;

    base_inc = INC_C9[note % 12u] >> (10u - note / 12u);
    voice[pick].note = (uint8_t)note;
    voice[pick].vel = (uint8_t)vel;
    voice[pick].age = ++synth_ages;
    voice_base_inc[pick] = base_inc;
    voice[pick].c_inc = base_inc * cm;
    voice[pick].m_inc = (base_inc >> 8) * mm_q8;
    /* Headroom for 4-voice polyphony + sub + echo without clipping */
    voice[pick].gate = (int32_t)(vel * 64u);
    /* Keep oscillators continuous when reusing a ringing voice (DRIVE magnifies phase/FB steps). */
    if (voice[pick].amp <= 64 && voice_amp_q8[pick] <= (64 << 8)) {
        voice[pick].c_ph = 0;
        voice[pick].m_ph = 0;
        voice_c_ph_r[pick] = 0;
        voice_sub_ph[pick] = 0;
        voice_fb1[pick] = 0;
        voice_fb2[pick] = 0;
        voice_amp_q8[pick] = 0;
        voice_mod_q8[pick] = 0;
    } else {
        voice_amp_q8[pick] >>= 1;
        voice_mod_q8[pick] >>= 1;
        voice_fb1[pick] >>= 1;
        voice_fb2[pick] >>= 1;
    }
    voice_timbre_ofs[pick] = timbre_ofs;
    voice_decay_mul_q8[pick] = mul_q8;
    voice_stage[pick] = 1u; /* start in Attack stage */

    if (last_note_out)
        *last_note_out = note;
}

static void synth_note_on(uint32_t note, uint32_t vel, const midi_cfg_t *c, uint32_t *last_note_out)
{
    synth_note_on_ex(0xFFFFFFFFu, note, vel, 0, 256, c, last_note_out);
}

static void synth_all_notes_off(void)
{
    uint32_t v;

    for (v = 0; v < NV; v++)
        voice[v].gate = 0;
}

static void synth_note_off_voice(uint32_t v)
{
    if (v < NV)
        voice[v].gate = 0;
}

static void synth_note_off(uint32_t note)
{
    uint32_t v;

    for (v = 0; v < NV; v++)
        if (voice[v].note == note)
            voice[v].gate = 0;
}

static int synth_is_quiet(void)
{
    uint32_t v;

    for (v = 0; v < NV; v++)
        if (voice[v].gate || voice[v].amp)
            return 0;
    return 1;
}

static uint32_t synth_lit_keys(uint32_t base_note)
{
    uint32_t v, lit = 0;

    for (v = 0; v < NV; v++) {
        uint32_t n;

        if (!voice[v].gate && voice[v].amp <= 48)
            continue;
        n = voice[v].note;
        if (n == 0)
            continue;
        while (n < base_note)
            n += 12u;
        while (n >= base_note + 27u)
            n -= 12u;
        if (n >= base_note && n < base_note + 27u)
            lit |= 1u << (n - base_note);
    }
    return lit;
}

static void synth_render(int32_t *out, uint32_t n, const midi_cfg_t *c)
{
    static const uint32_t ATK_Q8[16] = {
        65536u, 32768u, 16384u, 8192u, 4096u, 2048u, 1200u, 700u,
        420u, 260u, 170u, 115u, 80u, 58u, 42u, 30u};
    static const uint32_t DEC_Q8[16] = {
        2800u, 1800u, 1100u, 700u, 450u, 300u, 200u, 135u,
        90u, 62u, 44u, 32u, 22u, 15u, 10u, 5u};
    uint32_t cm, mm_q8_u;
    int32_t mm_q8;
    uint32_t atk_q8 = ATK_Q8[(c->val[P_ATTACK] >> 3) & 15u];
    uint32_t dec_q8 = DEC_Q8[(c->val[P_DECAY] >> 3) & 15u];
    uint32_t dec_val = (uint32_t)c->val[P_DECAY];
    int32_t timbre = (int32_t)c->val[P_TIMBRE];
    uint32_t drive = (uint32_t)c->val[P_DRIVE];
    int32_t det = (int32_t)c->val[P_DETUNE] - 64;
    uint32_t echo_mix = (uint32_t)c->val[P_ECHO_MIX];
    uint32_t echo_dly = 1024u + ((uint32_t)c->val[P_ECHO_TIME] * 54u);

    uint32_t i, v;

    midi_ratio_cm_q8(c->val[P_RATIO], &cm, &mm_q8_u);
    mm_q8 = (int32_t)mm_q8_u;

    if (mm_q8 < 64)
        mm_q8 = 64;
    if (echo_dly >= ECHO_LEN)
        echo_dly = ECHO_LEN - 1u;

    for (i = 0; i < n; i++) {
        int32_t mix_l = 0, mix_r = 0;

        for (v = 0; v < NV; v++) {
            uint32_t c_ph = voice[v].c_ph;
            uint32_t m_ph = voice[v].m_ph;
            uint32_t c_ph_r = voice_c_ph_r[v];
            uint32_t b_inc = voice_base_inc[v];
            uint32_t c_inc = b_inc ? b_inc * cm : voice[v].c_inc;
            uint32_t m_inc = b_inc ? ((b_inc >> 8) * (uint32_t)mm_q8) : voice[v].m_inc;
            int32_t amp_q8 = voice_amp_q8[v];
            int32_t mod_q8 = voice_mod_q8[v];
            int32_t g = voice[v].gate;
            uint8_t stg = voice_stage[v];
            int32_t amp, mod_env, mod, pmod, sig_l, sig_r, s;
            uint32_t det_add, vdec_q8;

            if (!amp_q8 && !g)
                continue;

            vdec_q8 = (dec_q8 * voice_decay_mul_q8[v]) >> 8;
            if (vdec_q8 < 1u)
                vdec_q8 = 1u;

            if (!g) {
                /* Note released: decay to zero */
                stg = 0u;
                amp_q8 -= (int32_t)(vdec_q8 * 2u);
                if (amp_q8 < 0)
                    amp_q8 = 0;
                mod_q8 -= (int32_t)(vdec_q8 * 3u);
                if (mod_q8 < 0)
                    mod_q8 = 0;
            } else if (stg == 1u) {
                /* Attack stage: ramp up to peak (gate << 8), then enter Decay */
                int32_t peak_q8 = g << 8;
                int32_t atk_inc = (int32_t)atk_q8;
                int32_t remain = peak_q8 - amp_q8;

                /* Min ~96 samples to peak so note-ons never Dirac the drive stage. */
                if (remain > 0 && atk_inc > 0) {
                    int32_t cap = (remain + 95) / 96;

                    if (atk_inc > cap)
                        atk_inc = cap;
                }

                amp_q8 += atk_inc;
#if FM2_DX7_TIMBRE
                /* OP2 punchier attack transient (tine strike) */
                mod_q8 += atk_inc << 1;
#else
                mod_q8 += atk_inc + (atk_inc >> 1);
#endif
                if (mod_q8 > peak_q8)
                    mod_q8 = peak_q8;
                if (amp_q8 >= peak_q8) {
                    amp_q8 = peak_q8;
                    stg = 2u;
                }
            } else {
                /* Decay stage (while note/chord is held): decay toward sustain floor */
                int32_t sus_q8 = dec_val > 96u ? ((g * (int32_t)(dec_val - 96u)) << 2) : 0;
#if FM2_DX7_TIMBRE
                int32_t msus_q8 = sus_q8 - (sus_q8 >> 3); /* ~87.5% of amp sustain on mod */
#else
                int32_t msus_q8 = sus_q8 >> 1;
#endif

                if (amp_q8 > sus_q8) {
                    amp_q8 -= (int32_t)vdec_q8;
                    if (amp_q8 < sus_q8)
                        amp_q8 = sus_q8;
                }
                if (mod_q8 > msus_q8) {
#if FM2_DX7_TIMBRE
                    mod_q8 -= (int32_t)(vdec_q8 << 1);
#else
                    mod_q8 -= (int32_t)(vdec_q8 + (vdec_q8 >> 1));
#endif
                    if (mod_q8 < msus_q8)
                        mod_q8 = msus_q8;
                }
            }

            amp = amp_q8 >> 8;
            mod_env = mod_q8 >> 8;

            {
#if FM2_DX7_TIMBRE
                /* DX7 phase mod: morphing TIMBRE zones + key-scale + OP2/OP3 + feedback */
                int32_t vt = timbre + voice_timbre_ofs[v];
                int32_t note_i = (int32_t)voice[v].note;
                int32_t kscale_q8, fb_k, fm_depth, sub_mix, op3_mix, body_mix, fb_sig, sub_sig;
                int32_t mod_env_fm, sustain_ratio, z, nyq_q8;
                int32_t op3, mod_raw, pmod_i;
                uint32_t op3_ph;
                uint32_t sub_ph = voice_sub_ph[v];

                if (vt < 0)
                    vt = 0;
                if (vt > 127)
                    vt = 127;

                /* Key-level scaling: rich bass, crystalline highs (256 @ 48 → 110 @ 84) */
                if (note_i < 48)
                    kscale_q8 = 256 + (48 - note_i) * 4;
                else if (note_i <= 84)
                    kscale_q8 = 256 - ((note_i - 48) * 146) / 36;
                else
                    kscale_q8 = 110 - (note_i - 84) * 3;
                if (kscale_q8 < 80)
                    kscale_q8 = 80;
                else if (kscale_q8 > 320)
                    kscale_q8 = 320;

                sustain_ratio = dec_val > 64u ? (int32_t)(dec_val - 64u) : 32;
                mod_env_fm = mod_env + ((amp * (192 + sustain_ratio)) >> 9);
                if (mod_env_fm > 255)
                    mod_env_fm = 255;

                if (vt <= 40) {
                    /* 0..40: immediate warm FM — linear + smooth index, no feedback */
                    sub_mix = ((40 - vt) * 3) >> 1;
                    fm_depth = (((vt << 4) + ((vt * vt) >> 1)) * kscale_q8) >> 12;
                    fb_k = 0;
                    op3_mix = 0;
                    body_mix = 0;
                } else if (vt <= 80) {
                    /* 41..80: tine bark → stable self-feedback saw warmth */
                    z = vt - 40;
                    sub_mix = vt < 55 ? ((55 - vt) >> 2) : 0;
                    fm_depth = ((520 + z * (24 + z)) * kscale_q8) >> 12;
                    fb_k = (z * 160) / 40;
                    op3_mix = (z * 5) >> 2;
                    body_mix = z >> 3;
                } else {
                    /* 81..127: DX7 cascade — OP3 + parallel 2nd-harmonic body */
                    z = vt - 80;
                    sub_mix = 0;
                    fm_depth = ((920 + z * (16 + (z >> 1))) * kscale_q8) >> 12;
                    fb_k = 150 + (z >> 2);
                    if (fb_k > 160)
                        fb_k = 160;
                    op3_mix = 10 + (z >> 1);
                    body_mix = 6 + (z >> 2);
                }

                /* Nyquist guard: soften index when modulator runs high vs carrier */
                nyq_q8 = 256;
                if (m_inc > 0x08000000u) {
                    uint32_t hi = (m_inc - 0x08000000u) >> 20;

                    if (hi > 200u)
                        hi = 200u;
                    nyq_q8 = (int32_t)(256u - hi);
                }
                fm_depth = (fm_depth * nyq_q8) >> 8;
                op3_mix = (op3_mix * nyq_q8) >> 8;
                body_mix = (body_mix * nyq_q8) >> 8;

                fb_sig = (voice_fb1[v] + voice_fb2[v]) >> 1;
                m_ph += m_inc;
                mod_raw = fm2_sine(m_ph + (uint32_t)((int32_t)fb_sig * (int32_t)fb_k << 6));
                voice_fb2[v] = voice_fb1[v];
                voice_fb1[v] = mod_raw;

                if (op3_mix) {
                    if (m_inc > c_inc)
                        op3_ph = c_ph << 1;
                    else
                        op3_ph = m_ph << 1;
                    op3 = fm2_sine(op3_ph + (uint32_t)((int32_t)mod_raw * (int32_t)(fb_k >> 2) << 6));
                    mod_raw += (op3 * op3_mix) >> 8;
                }

                mod = (mod_raw * mod_env_fm) >> 15;
                pmod_i = ((mod >> 2) * fm_depth) << 13;
                if (pmod_i > 0x18000000)
                    pmod_i = 0x18000000;
                else if (pmod_i < -0x18000000)
                    pmod_i = -0x18000000;
                pmod = pmod_i;

                c_ph += c_inc;
                det_add = (uint32_t)(((int32_t)(c_inc >> 15) * det));
                c_ph_r += c_inc + det_add;

                sub_ph += c_inc >> 1;
                voice_sub_ph[v] = sub_ph;
                sub_sig = (fm2_sine(sub_ph) * sub_mix) >> 8;

                sig_l = ((fm2_sine(c_ph + (uint32_t)pmod) * 200) >> 8) + sub_sig;
                sig_r = ((fm2_sine(c_ph_r + (uint32_t)pmod) * 200) >> 8) + sub_sig;
                if (body_mix) {
                    int32_t body = (fm2_sine((c_ph << 1) + (uint32_t)(pmod >> 1)) * body_mix) >> 8;

                    sig_l += body;
                    sig_r += body;
                }
#else
                /* Legacy: single mod index, no feedback / key-scale / zone morph */
                int32_t vt = timbre + voice_timbre_ofs[v];
                int32_t fm_depth, sub_sig;
                uint32_t sub_ph = voice_sub_ph[v];

                if (vt < 0)
                    vt = 0;
                if (vt > 127)
                    vt = 127;

                fm_depth = (vt * vt) >> 5;

                m_ph += m_inc;
                mod = fm2_sine(m_ph);
                mod = (mod * mod_env) >> 14;
                pmod = (mod * fm_depth) << 11;

                c_ph += c_inc;
                det_add = (uint32_t)(((int32_t)(c_inc >> 15) * det));
                c_ph_r += c_inc + det_add;

                sub_ph += c_inc >> 1;
                voice_sub_ph[v] = sub_ph;
                sub_sig = 0;

                sig_l = (fm2_sine(c_ph + (uint32_t)pmod) * 200) >> 8;
                sig_r = (fm2_sine(c_ph_r + (uint32_t)pmod) * 200) >> 8;
#endif
            }

            s = (sig_l * amp) >> 15;
            if (drive > 30u)
                s = fm2_warm_sat(s >> 1) << 1;
            mix_l += s;
            s = (sig_r * amp) >> 15;
            if (drive > 30u)
                s = fm2_warm_sat(s >> 1) << 1;
            mix_r += s;

            voice[v].c_ph = c_ph;
            voice[v].m_ph = m_ph;
            voice[v].amp = amp;
            voice[v].mod_env = mod_env;
            voice_amp_q8[v] = amp_q8;
            voice_mod_q8[v] = mod_q8;
            voice_stage[v] = stg;
            voice_c_ph_r[v] = c_ph_r;
        }

        mix_l = fm2_soft_clip(mix_l, drive);
        mix_r = fm2_soft_clip(mix_r, drive);
        if (drive > 30u) {
            int32_t co = 16 + (int32_t)(drive - 30u) * 3;

            if (co > 220)
                co = 220;
            fm2_drive_lp_l += ((mix_l - fm2_drive_lp_l) * co) >> 8;
            fm2_drive_lp_r += ((mix_r - fm2_drive_lp_r) * co) >> 8;
            mix_l = fm2_drive_lp_l;
            mix_r = fm2_drive_lp_r;
        } else {
            fm2_drive_lp_l = mix_l;
            fm2_drive_lp_r = mix_r;
        }

        {
            int32_t tap, wet, dry, fb, dry_atten;

            if (echo_dly_cur < echo_dly)
                echo_dly_cur++;
            else if (echo_dly_cur > echo_dly)
                echo_dly_cur--;

            tap = (int32_t)echo_buf[(echo_wr + ECHO_LEN - echo_dly_cur) & (ECHO_LEN - 1u)];
            echo_lp += ((tap - echo_lp) * 140) >> 8;
            tap = echo_lp;
            wet = (tap * (int32_t)echo_mix) >> 7;
            dry = (mix_l + mix_r) >> 1;
            fb = fm2_warm_sat(((dry * 3) >> 2) + ((wet * 3) >> 3));
            echo_buf[echo_wr] = (int16_t)fb;

            dry_atten = (int32_t)(256u - (echo_mix >> 2));
            mix_l = ((mix_l * dry_atten) >> 8) + ((wet * 3) >> 2);
            mix_r = ((mix_r * dry_atten) >> 8) + ((wet * 3) >> 2);
            echo_wr = (echo_wr + 1u) & (ECHO_LEN - 1u);
        }

        mix_l = fm2_warm_sat(mix_l);
        mix_r = fm2_warm_sat(mix_r);
        if (mix_l > 30000)
            mix_l = 30000;
        else if (mix_l < -30000)
            mix_l = -30000;
        if (mix_r > 30000)
            mix_r = 30000;
        else if (mix_r < -30000)
            mix_r = -30000;

        out[2u * i] = mix_l;
        out[2u * i + 1u] = mix_r;
    }
}

#endif /* SYNTH_FM2_H */
