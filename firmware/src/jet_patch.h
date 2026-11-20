/* SPDX-License-Identifier: GPL-3.0-only */
/* KO JET patch contract — shared by jet_dsp / jet_plates / jet.c. Header-only. */
#ifndef JET_PATCH_H
#define JET_PATCH_H

#include <stddef.h>
#include <stdint.h>

enum {
    JET_MODE_POLY = 0,
    JET_MODE_MONO,
    JET_MODE_LEGATO,
    JET_MODE_UNISON,
    JET_MODE_N
};

typedef struct {
    /* STEAM (generator) */
    uint8_t blow;     /* generator amount 0..127 */
    uint8_t dc_noise; /* 0 = pure DC pressure .. 127 = pure noise */
    uint8_t noise_lpf; /* MIDI pitch of noise one-pole LP */
    uint8_t s_dly, s_atk, s_hold, s_dec, s_sus, s_rel; /* bass_env.h time laws */
    uint8_t s_curve;  /* 64 = lin (BASS_DAHDSR_CURVE_LIN) */
    uint8_t vel_sens; /* 0 = env always full */
    /* PIPE */
    int8_t push;      /* -64..63, <0 pull, >0 push */
    uint8_t fb_dec, fb_rel; /* loop T60 gate-on / gate-off, log 10 ms..20 s */
    uint8_t sat, sat_shape, sat_sym; /* drive; 0 soft..127 hard; 0 sym..127 asym */
    uint8_t harm;     /* dispersion (stiffness) 0..127 */
    uint8_t split;    /* 0 off, 1..127 split point */
    uint8_t polarity; /* 0/1, 1 = odd harmonics */
    uint8_t lpf, hpf; /* MIDI pitch, loop one-poles */
    uint8_t tcomp;    /* filter tuning compensation, 127 = full */
    int8_t tune;      /* -24..24 semitones */
    int8_t fine;      /* -50..50 cents */
    /* VOICE */
    uint8_t mode, glide, detune, level;
    /* EXPR depths, 64 = 0 */
    uint8_t x_breath_blow, x_mw_lpf, x_mw_vib, x_at_push;
    /* FX */
    uint8_t dly_lvl, dly_time, dly_fb, dly_rev;
    uint8_t rev_mix, rev_size, rev_lo, rev_hi, rev_spin, width;
    /* FX — KO BASS echo/space; 0 = neutral so positional era seeds stay valid */
    int8_t dly_tone; /* loop tilt -64 darker .. +63 thinner */
    uint8_t wow, tap, tank;
    int8_t iso;      /* -64 LP .. +63 HP */
    /* HOME macros — offsets applied to the effective patch (not the stored knob base) */
    uint8_t mac_breath, mac_grit, mac_space;
} jet_patch_t;

enum {
    JET_FMT_RAW = 0,
    JET_FMT_PCT,
    JET_FMT_BIP,
    JET_FMT_PITCH,
    JET_FMT_TIME,
    JET_FMT_SEMI,
    JET_FMT_CENT,
    JET_FMT_ENUM,
    JET_FMT_TOG,
    JET_FMT_SYNC /* fm_time cc; app formats note division */
};

typedef struct {
    const char *lbl; /* <= 6 chars, uppercase */
    uint8_t off;     /* offsetof(jet_patch_t, field) */
    int8_t lo, hi, def;
    uint8_t fmt;
    uint8_t is_signed;
    const char *const *names; /* JET_FMT_ENUM */
} jet_param_t;

enum {
    JET_P_BLOW = 0,
    JET_P_DCN,
    JET_P_NZLPF,
    JET_P_SDLY,
    JET_P_SATK,
    JET_P_SHOLD,
    JET_P_SDEC,
    JET_P_SSUS,
    JET_P_SREL,
    JET_P_SCURVE,
    JET_P_VEL,
    JET_P_PUSH,
    JET_P_FBDEC,
    JET_P_FBREL,
    JET_P_SAT,
    JET_P_SHAPE,
    JET_P_SYM,
    JET_P_HARM,
    JET_P_SPLIT,
    JET_P_POLAR,
    JET_P_LPF,
    JET_P_HPF,
    JET_P_TCOMP,
    JET_P_TUNE,
    JET_P_FINE,
    JET_P_MODE,
    JET_P_GLIDE,
    JET_P_DETUNE,
    JET_P_LEVEL,
    JET_P_XBRBLOW,
    JET_P_XMWLPF,
    JET_P_XMWVIB,
    JET_P_XATPUSH,
    JET_P_DLYLVL,
    JET_P_DLYTIME,
    JET_P_DLYFB,
    JET_P_DLYREV,
    JET_P_REVMIX,
    JET_P_REVSIZE,
    JET_P_REVLO,
    JET_P_REVHI,
    JET_P_REVSPIN,
    JET_P_WIDTH,
    JET_P_DLYTONE,
    JET_P_WOW,
    JET_P_TAP,
    JET_P_TANK,
    JET_P_ISO,
    JET_P_MACBRTH,
    JET_P_MACGRIT,
    JET_P_MACSPC,
    JET_P_N
};

static const char *const JET_MODE_NAME[JET_MODE_N] = {
    "POLY", "MONO", "LEGATO", "UNISON"
};

static const char *const JET_POLAR_NAME[2] = { "ALL", "ODD" };
static const char *const JET_TAP_NAME[6] = { "SINGLE", "DUAL", "GALLOP", "TRIP", "SPREAD", "PING" };
static const char *const JET_TANK_NAME[2] = { "SPRING", "TANK" };

#define JET_POFF(f) ((uint8_t)offsetof(jet_patch_t, f))

static const jet_param_t JET_PARAM[JET_P_N] = {
    /* STEAM */
    { "BLOW", JET_POFF(blow), 0, 127, 90, JET_FMT_PCT, 0, 0 },
    { "DC/NZ", JET_POFF(dc_noise), 0, 127, 40, JET_FMT_PCT, 0, 0 },
    { "NZLPF", JET_POFF(noise_lpf), 0, 127, 96, JET_FMT_PITCH, 0, 0 },
    { "DELAY", JET_POFF(s_dly), 0, 127, 0, JET_FMT_TIME, 0, 0 },
    { "ATTACK", JET_POFF(s_atk), 0, 127, 30, JET_FMT_TIME, 0, 0 },
    { "HOLD", JET_POFF(s_hold), 0, 127, 0, JET_FMT_TIME, 0, 0 },
    { "DECAY", JET_POFF(s_dec), 0, 127, 60, JET_FMT_TIME, 0, 0 },
    { "SUST", JET_POFF(s_sus), 0, 127, 100, JET_FMT_PCT, 0, 0 },
    { "REL", JET_POFF(s_rel), 0, 127, 50, JET_FMT_TIME, 0, 0 },
    { "CURVE", JET_POFF(s_curve), 0, 127, 64, JET_FMT_BIP, 0, 0 },
    { "VEL", JET_POFF(vel_sens), 0, 127, 40, JET_FMT_PCT, 0, 0 },
    /* PIPE */
    { "PUSH", JET_POFF(push), -64, 63, 28, JET_FMT_BIP, 1, 0 },
    { "FBDEC", JET_POFF(fb_dec), 0, 127, 100, JET_FMT_TIME, 0, 0 },
    { "FBREL", JET_POFF(fb_rel), 0, 127, 40, JET_FMT_TIME, 0, 0 },
    { "SAT", JET_POFF(sat), 0, 127, 50, JET_FMT_PCT, 0, 0 },
    { "SHAPE", JET_POFF(sat_shape), 0, 127, 40, JET_FMT_PCT, 0, 0 },
    { "SYM", JET_POFF(sat_sym), 0, 127, 0, JET_FMT_PCT, 0, 0 },
    { "HARM", JET_POFF(harm), 0, 127, 0, JET_FMT_PCT, 0, 0 },
    { "SPLIT", JET_POFF(split), 0, 127, 0, JET_FMT_PCT, 0, 0 },
    { "POLAR", JET_POFF(polarity), 0, 1, 0, JET_FMT_ENUM, 0, JET_POLAR_NAME },
    { "LPF", JET_POFF(lpf), 0, 127, 100, JET_FMT_PITCH, 0, 0 },
    { "HPF", JET_POFF(hpf), 0, 127, 20, JET_FMT_PITCH, 0, 0 },
    { "TCOMP", JET_POFF(tcomp), 0, 127, 127, JET_FMT_PCT, 0, 0 },
    { "TUNE", JET_POFF(tune), -24, 24, 0, JET_FMT_SEMI, 1, 0 },
    { "FINE", JET_POFF(fine), -50, 50, 0, JET_FMT_CENT, 1, 0 },
    /* VOICE */
    { "MODE", JET_POFF(mode), 0, JET_MODE_N - 1, JET_MODE_POLY, JET_FMT_ENUM, 0, JET_MODE_NAME },
    { "GLIDE", JET_POFF(glide), 0, 127, 0, JET_FMT_TIME, 0, 0 },
    { "DETUNE", JET_POFF(detune), 0, 127, 6, JET_FMT_CENT, 0, 0 },
    { "LEVEL", JET_POFF(level), 0, 127, 100, JET_FMT_PCT, 0, 0 },
    /* EXPR */
    { "BR>BLW", JET_POFF(x_breath_blow), 0, 127, 64, JET_FMT_BIP, 0, 0 },
    { "MW>LPF", JET_POFF(x_mw_lpf), 0, 127, 64, JET_FMT_BIP, 0, 0 },
    { "MW>VIB", JET_POFF(x_mw_vib), 0, 127, 64, JET_FMT_BIP, 0, 0 },
    { "AT>PSH", JET_POFF(x_at_push), 0, 127, 64, JET_FMT_BIP, 0, 0 },
    /* FX */
    { "DLY", JET_POFF(dly_lvl), 0, 127, 0, JET_FMT_PCT, 0, 0 },
    { "TIME", JET_POFF(dly_time), 0, 127, 64, JET_FMT_SYNC, 0, 0 },
    { "FB", JET_POFF(dly_fb), 0, 127, 40, JET_FMT_PCT, 0, 0 },
    { "BLEED", JET_POFF(dly_rev), 0, 127, 0, JET_FMT_PCT, 0, 0 },
    { "REVERB", JET_POFF(rev_mix), 0, 127, 30, JET_FMT_PCT, 0, 0 },
    { "TAIL", JET_POFF(rev_size), 0, 127, 70, JET_FMT_PCT, 0, 0 },
    { "LOCUT", JET_POFF(rev_lo), 0, 127, 0, JET_FMT_PCT, 0, 0 },
    { "DAMP", JET_POFF(rev_hi), 0, 127, 64, JET_FMT_PCT, 0, 0 },
    { "SPIN", JET_POFF(rev_spin), 0, 127, 20, JET_FMT_PCT, 0, 0 },
    { "WIDTH", JET_POFF(width), 0, 127, 64, JET_FMT_BIP, 0, 0 },
    { "TONE", JET_POFF(dly_tone), -64, 63, 0, JET_FMT_BIP, 1, 0 },
    { "WOW", JET_POFF(wow), 0, 127, 0, JET_FMT_PCT, 0, 0 },
    { "TAP", JET_POFF(tap), 0, 5, 0, JET_FMT_ENUM, 0, JET_TAP_NAME },
    { "TANK", JET_POFF(tank), 0, 1, 0, JET_FMT_ENUM, 0, JET_TANK_NAME },
    { "ISO", JET_POFF(iso), -64, 63, 0, JET_FMT_BIP, 1, 0 },
    /* HOME macros */
    { "BREATH", JET_POFF(mac_breath), 0, 127, 0, JET_FMT_PCT, 0, 0 },
    { "GRIT", JET_POFF(mac_grit), 0, 127, 0, JET_FMT_PCT, 0, 0 },
    { "SPACE", JET_POFF(mac_space), 0, 127, 0, JET_FMT_PCT, 0, 0 },
};

static inline void jet_patch_defaults(jet_patch_t *p)
{
    uint8_t i;

    for (i = 0u; i < JET_P_N; i++) {
        const jet_param_t *pr = &JET_PARAM[i];
        uint8_t *b = (uint8_t *)p + pr->off;

        if (pr->is_signed)
            *(int8_t *)b = pr->def;
        else
            *b = (uint8_t)pr->def;
    }
}

static inline int jet_param_get(const jet_patch_t *p, uint8_t id)
{
    const jet_param_t *pr;
    const uint8_t *b;

    if (id >= JET_P_N)
        return 0;
    pr = &JET_PARAM[id];
    b = (const uint8_t *)p + pr->off;
    if (pr->is_signed)
        return (int)*(const int8_t *)b;
    return (int)*b;
}

static inline void jet_param_set(jet_patch_t *p, uint8_t id, int v)
{
    const jet_param_t *pr;
    uint8_t *b;

    if (id >= JET_P_N)
        return;
    pr = &JET_PARAM[id];
    if (v < pr->lo)
        v = pr->lo;
    if (v > pr->hi)
        v = pr->hi;
    b = (uint8_t *)p + pr->off;
    if (pr->is_signed)
        *(int8_t *)b = (int8_t)v;
    else
        *b = (uint8_t)v;
}

static inline void jet_patch_sanitize(jet_patch_t *p)
{
    uint8_t i;

    for (i = 0u; i < JET_P_N; i++)
        jet_param_set(p, i, jet_param_get(p, i));
}

static inline void jet_param_itoa(char *s, int v)
{
    char tmp[8];
    uint8_t n = 0u;
    unsigned u;
    uint8_t i;

    if (v < 0) {
        *s++ = '-';
        u = (unsigned)(-v);
    } else {
        u = (unsigned)v;
    }
    do {
        tmp[n++] = (char)('0' + (u % 10u));
        u /= 10u;
    } while (u && n < 8u);
    for (i = 0u; i < n; i++)
        s[i] = tmp[n - 1u - i];
    s[n] = 0;
}

/* Writes short readout into s (caller provides >= 8 bytes). */
static inline void jet_param_fmt(char *s, uint8_t id, int v)
{
    const jet_param_t *pr;
    int pct;

    if (!s)
        return;
    if (id >= JET_P_N) {
        s[0] = 0;
        return;
    }
    pr = &JET_PARAM[id];
    if (v < pr->lo)
        v = pr->lo;
    if (v > pr->hi)
        v = pr->hi;

    switch (pr->fmt) {
    case JET_FMT_ENUM:
        if (pr->names && v >= pr->lo && v <= pr->hi) {
            const char *nm = pr->names[v];
            uint8_t i = 0u;

            while (nm[i] && i < 7u) {
                s[i] = nm[i];
                i++;
            }
            s[i] = 0;
            return;
        }
        jet_param_itoa(s, v);
        return;
    case JET_FMT_TOG:
        s[0] = v ? 'O' : 'o';
        s[1] = v ? 'N' : 'f';
        s[2] = v ? 0 : 'f';
        s[3] = 0;
        return;
    case JET_FMT_PCT:
        pct = (v * 100 + 63) / 127;
        jet_param_itoa(s, pct);
        {
            uint8_t n = 0u;

            while (s[n])
                n++;
            s[n++] = '%';
            s[n] = 0;
        }
        return;
    case JET_FMT_BIP: {
        int c = v - 64;

        if (pr->is_signed)
            c = v;
        if (c > 0)
            *s++ = '+';
        jet_param_itoa(s, c);
        return;
    }
    case JET_FMT_SEMI:
        if (v > 0)
            *s++ = '+';
        jet_param_itoa(s, v);
        return;
    case JET_FMT_CENT:
        if (v > 0)
            *s++ = '+';
        jet_param_itoa(s, v);
        {
            uint8_t n = 0u;

            while (s[n])
                n++;
            s[n++] = 'c';
            s[n] = 0;
        }
        return;
    case JET_FMT_PITCH:
        /* MIDI note number as raw — app may overlay note name later. */
        jet_param_itoa(s, v);
        return;
    case JET_FMT_TIME:
    case JET_FMT_SYNC:
    case JET_FMT_RAW:
    default:
        jet_param_itoa(s, v);
        return;
    }
}

#undef JET_POFF

#endif /* JET_PATCH_H */
