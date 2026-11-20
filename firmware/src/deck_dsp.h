/* SPDX-License-Identifier: GPL-3.0-only */
/* KO DECK 16-voice integer drum synth (Q15, freestanding). */
#ifndef DECK_DSP_H
#define DECK_DSP_H

#include "deck_kit.h"
#include "dub_types.h"
#include "fm_env_plot.h"
#include "fm_lfo.h"

#define DECK_FS 44100u
#define DECK_DLY_LEN 8192u
#define DECK_REV_COMB_LEN 512u

typedef struct {
    uint8_t level;
    uint8_t tune;
    uint8_t decay;
    uint8_t pitch_env;
    uint8_t timbre; /* SEL TIMBR — tone/FM/click; not pitch sweep (pitch_env) */
    uint8_t noise;
    uint8_t crush; /* 0 = off, 1..127 */
    uint8_t filter;
    uint8_t drive;
    uint8_t pan;       /* 0 = L, 64 = C, 127 = R */
    uint8_t click;
    uint8_t hold;
    uint8_t pitch_dec;
    uint8_t choke_grp;
    uint8_t curve;
    uint8_t vel_sens;
    uint8_t sustain;
    uint8_t release;
    /* Keep last: DKZ3 saves are migrated by copying the leading bytes. 127 = unity (full feed). */
    uint8_t dly_send;
    uint8_t rev_send;
    uint8_t duck; /* BD sidechain depth; 0 = untouched (bit-exact), ignored on BD itself */
} deck_voice_cfg_t;

typedef struct {
    /* Genre bus (dub FX bank parity) */
    uint8_t dly_lvl;
    uint8_t dly_time;
    uint8_t feedback;
    uint8_t loop_filt;
    uint8_t wow;
    uint8_t tap; /* 0..5, TAP_* in dub_types.h */
    uint8_t tank;
    uint8_t isolator; /* 64 = flat */
    uint8_t rev_mix;
    uint8_t rev_damp;
    uint8_t rev_tail;
    uint8_t dly_rev;
    /* DECK dirt (last FX pages) */
    uint8_t drive;
    uint8_t crush;
    uint8_t filter; /* 64 = off, <64 LP, >64 HP */
    uint8_t width;
    uint8_t comp;
    uint8_t dust;
    uint8_t tape;
} deck_master_t;

/* 64 = no offset. Applied at render, never written into cfg. */
typedef struct {
    uint8_t tune;
    uint8_t decay;
    uint8_t filter;
    uint8_t crush;
    uint8_t density;
    uint8_t fill;
    uint8_t mute_grp;
    uint8_t stutter;
} deck_perf_t;

typedef struct {
    uint8_t active;
    uint8_t vel;
    uint8_t accent;
    uint8_t choke; /* 2 = smooth hat choke */
    uint32_t ph;
    uint32_t mph;
    int32_t env;
    int32_t env2;
    int32_t pitch;
    int32_t mod_env;
    int32_t bp1;
    int32_t bp2;
    int32_t hp;
    int32_t flt_b; /* per-voice FILTER SVF — must not share bp1/bp2 with the voice renders */
    int32_t flt_l;
    int32_t shell1;
    int32_t shell2;
    uint32_t nph;
    uint32_t hat_ph[6];
    int32_t hat_env;
    int32_t oh_ring;
    int32_t clap_env[4];
    uint8_t clap_idx;
    int32_t click;
    uint32_t sq_ph[2];
    int32_t crush_hold;
    uint8_t crush_cnt;
    uint16_t gate_left; /* seq step gate: samples until choke */
    uint16_t env_dly_left;
    int32_t env_pend_e;
    int32_t env_pend_e2;
    uint16_t env_atk_left;
    int8_t semi; /* per-trigger transpose (seq random + P-lock), ±DECK_SEMI_MAX */
} deck_voice_t;

typedef struct {
    deck_voice_t v[DECK_NVOICES];
    deck_voice_cfg_t cfg[DECK_NVOICES];
    deck_master_t master;
    deck_perf_t perf;
    int64_t mf_ic1[2]; /* master DJ filter TPT-SVF state, [0] mid · [1] side, Q12 */
    int64_t mf_ic2[2];
    int32_t mf_idx_q8; /* smoothed cutoff index into DECK_MF_COEF, Q8 */
    int32_t mf_wet;    /* smoothed filter amount, Q15; 0 = dry (bit-exact bypass) */
    uint8_t mf_hp;     /* active mode: 0 = LP, 1 = HP */
    int32_t m_comp;
    uint32_t dust_ph;
    int32_t crush_hold;
    uint8_t crush_cnt;
    uint8_t render_bpm;
    uint32_t dly_w;
    uint32_t wow_ph;
    int32_t dly_filt;
    int32_t iso_lp;
    int32_t iso_hp;
    int32_t iso_lp_r;
    int32_t iso_hp_r;
    uint16_t rev_pos[4];
    int32_t rev_lp[4];
    uint8_t env_invert[DECK_NVOICES];
    uint8_t env_delay[DECK_NVOICES];
    uint8_t env_retrig[DECK_NVOICES];
    uint8_t era; /* decade synth character — independent of plate preset */
    uint8_t duck_atk;  /* 1 while the BD sidechain envelope rises toward duck_peak */
    int32_t duck_peak; /* Q15 */
    int32_t duck_env;  /* Q15 */
} deck_dsp_t;

#if defined(DINGHY_HOST) || defined(DINGHY_WEB)
static int16_t deck_dly_pool[DECK_DLY_LEN];
static int16_t deck_rev_comb[4][DECK_REV_COMB_LEN];
#else
static int16_t deck_dly_pool[DECK_DLY_LEN] __attribute__((section(".pool")));
static int16_t deck_rev_comb[4][DECK_REV_COMB_LEN] __attribute__((section(".pool")));
#endif

static int32_t deck_sine(uint32_t ph)
{
    int32_t x = (int32_t)(int16_t)(ph >> 16);
    int32_t ax = x < 0 ? -x : x;
    int32_t y = (x * (32768 - ax)) >> 13;
    int32_t ay = y < 0 ? -y : y;

    y += ((((y * ay) >> 15) - y) * 7373) >> 15;
    if (y > 32767)
        return 32767;
    if (y < -32767)
        return -32767;
    return y;
}

/* Gain, then monotone soft knee: linear up to KNEE, parabola (slope 1 → 0) up to the 32767 ceiling, then flat. */
enum { DECK_CLIP_KNEE = 24576, DECK_CLIP_SPAN = 2 * (32767 - DECK_CLIP_KNEE) };

static int32_t deck_softclip(int32_t x, uint32_t drive)
{
    int64_t g = 256 + ((int64_t)drive << 1);
    int64_t ax = ((int64_t)x * g) >> 8;
    int64_t u;
    int32_t y;

    if (ax < 0)
        ax = -ax;
    if (ax <= DECK_CLIP_KNEE) {
        y = (int32_t)ax;
    } else {
        u = ax - DECK_CLIP_KNEE;
        y = u >= DECK_CLIP_SPAN ? 32767 : (int32_t)(DECK_CLIP_KNEE + u - (u * u) / (2 * DECK_CLIP_SPAN));
    }
    return x < 0 ? -y : y;
}

static int32_t deck_apply_level(int32_t out, uint8_t level)
{
    return (out * (int32_t)level * (int32_t)level) >> 14;
}

static int32_t deck_fold(int32_t x, uint32_t drive)
{
    int32_t lim;
    int32_t i;

    if (drive <= 8u)
        return deck_softclip(x, drive);
    lim = 26000 - (int32_t)drive * 155;
    if (lim < 5500)
        lim = 5500;
    for (i = 0; i < 3; i++) {
        if (x > lim)
            x = lim - (x - lim);
        else if (x < -lim)
            x = -lim - (x + lim);
    }
    return deck_softclip(x, drive >> 1);
}

static uint32_t deck_note_inc(uint8_t note)
{
    static const uint32_t inc_c9[12] = {
        815363807u, 863847862u, 915214929u, 969636441u, 1027294024u, 1088380105u,
        1153098554u, 1221665363u, 1294309365u, 1371273005u, 1452813141u, 1539201906u};

    if (note > 127u)
        note = 127u;
    return inc_c9[note % 12u] >> (10u - note / 12u);
}

static int32_t deck_noise(uint32_t *st)
{
    uint32_t x = *st;

    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *st = x;
    return (int32_t)(x >> 16) - 32768;
}

#define DECK_AMP_ENV_FLOOR 8

static int32_t deck_decay(int32_t v, uint32_t sh)
{
    int32_t d;

    if (v <= 0)
        return 0;
    if (sh > 20u)
        sh = 20u;
    d = v >> sh;
    if (d < 1)
        d = 1;
    v -= d;
    return v > 0 ? v : 0;
}

static int32_t deck_env_decay(deck_voice_t *d, int32_t v, int32_t peak, const deck_voice_cfg_t *c, uint32_t body_sh)
{
    uint32_t sh = body_sh;
    int32_t floor;
    int32_t span;
    int32_t prog;
    int32_t mul;
    int32_t dstep;
    uint8_t rel = 0u;

    if (d && d->env_atk_left)
        return v;
    if (v <= 0 || !c)
        return deck_decay(v, body_sh);
    if (peak < 1)
        peak = v;
    floor = ((int32_t)c->sustain * peak) >> 7;
    if (c->sustain < 4u)
        floor = 0;
    if (v <= floor) {
        rel = 1u;
        sh = 4u + (uint32_t)(c->release >> 2);
        if (sh > 10u)
            sh = 10u;
        span = floor - DECK_AMP_ENV_FLOOR;
        if (span < 1)
            span = 1;
        prog = ((floor - v) * 128) / span;
    } else {
        span = peak - floor;
        if (span < 1)
            span = 1;
        prog = 128 - ((v - floor) * 128) / span;
    }
    if (prog < 0)
        prog = 0;
    if (prog > 128)
        prog = 128;
    sh = fm_env_decay_sh(sh, c->curve, v, peak);
    if (rel && sh > 12u)
        sh = 12u;
    mul = fm_env_decay_step_mul((uint8_t)prog, c->curve, rel);
    dstep = v >> sh;
    dstep = (dstep * mul) >> 7;
    if (dstep < 1)
        dstep = 1;
    if (!rel && v - dstep < floor) {
        v = floor;
    } else {
        v -= dstep;
        if (!rel && v < floor)
            v = floor;
    }
    if (rel && v <= DECK_AMP_ENV_FLOOR)
        return 0;
    return v > 0 ? v : 0;
}

static inline int32_t deck_env_peak_q(const deck_voice_t *d)
{
    int32_t pk;

    if (!d)
        return 1;
    pk = (int32_t)d->vel << 9;
    return pk > 0 ? pk : 1;
}

/* BASS bass_dahdsr_mod_q15 parity: invert amplitude envelope (peak − env). */
static inline int32_t deck_amp_env(const deck_voice_t *d, int32_t env, uint8_t invert)
{
    int32_t a;

    if (env <= DECK_AMP_ENV_FLOOR)
        return 0;
    if (!invert)
        return env;
    a = deck_env_peak_q(d) - env;
    return a > DECK_AMP_ENV_FLOOR ? a : 0;
}

/* pitch_dec 64 = neutral; higher = faster pitch glide down. */
static inline uint32_t deck_pitch_dec_shift(const deck_voice_cfg_t *c, uint32_t base_sh)
{
    int32_t sh = (int32_t)base_sh + (((int32_t)c->pitch_dec - 64) >> 3);

    if (sh < 3)
        sh = 3;
    if (sh > 18)
        sh = 18;
    return (uint32_t)sh;
}

/* pitch_env 64 = no sweep; below/above = opposite pitch bend from hit. */
static inline int32_t deck_pitch_sweep_amt(const deck_voice_cfg_t *c)
{
    return (int32_t)c->pitch_env - 64;
}

static inline uint32_t deck_pitch_sweep_mag(const deck_voice_cfg_t *c)
{
    int32_t a = deck_pitch_sweep_amt(c);

    return (uint32_t)(a < 0 ? -a : a);
}

static inline int32_t deck_pitch_sweep_ph_sc(const deck_voice_cfg_t *c, int32_t max_q)
{
    int32_t amt = deck_pitch_sweep_amt(c);

    if (max_q == 0)
        return 0;
    return (int32_t)(((int64_t)amt * (int64_t)max_q) / 63);
}

/* SEL TIMBR 0..127 — unipolar tone/FM/noise depth (64 ≈ neutral on most voices). */
static inline int32_t deck_timb_depth(uint8_t t)
{
    return 16 + (int32_t)t;
}

static inline int32_t deck_timb_noise(uint8_t t)
{
    return 8 + ((int32_t)t >> 1);
}

static inline void deck_pitch_sweep_tick(deck_voice_t *d, const deck_voice_cfg_t *c, uint32_t base_sh)
{
    if (d->pitch == 0)
        return;
    d->pitch -= d->pitch >> deck_pitch_dec_shift(c, base_sh);
    if (d->pitch > -4 && d->pitch < 4)
        d->pitch = 0;
}

#define DECK_SEMI_MAX 24

/* 2^(st/12) in Q16 for st = -24..+24. */
static const uint32_t DECK_SEMI_Q16[2 * DECK_SEMI_MAX + 1] = {
    16384u, 17358u, 18390u, 19484u, 20643u, 21870u, 23170u,
    24548u, 26008u, 27554u, 29193u, 30929u, 32768u, 34716u,
    36781u, 38968u, 41285u, 43740u, 46341u, 49097u, 52016u,
    55109u, 58386u, 61858u, 65536u, 69433u, 73562u, 77936u,
    82570u, 87480u, 92682u, 98193u, 104032u, 110218u, 116772u,
    123715u, 131072u, 138866u, 147123u, 155872u, 165140u, 174960u,
    185364u, 196386u, 208064u, 220436u, 233544u, 247431u, 262144u,
};

/* Phase increment × 2^(semi/12); semi 0 returns inc unchanged. Capped below Nyquist. */
static inline uint32_t deck_semi_inc(uint32_t inc, int8_t semi)
{
    int32_t ix = (int32_t)semi;
    uint64_t v;

    if (ix == 0)
        return inc;
    if (ix < -DECK_SEMI_MAX)
        ix = -DECK_SEMI_MAX;
    if (ix > DECK_SEMI_MAX)
        ix = DECK_SEMI_MAX;
    v = ((uint64_t)inc * (uint64_t)DECK_SEMI_Q16[ix + DECK_SEMI_MAX]) >> 16;
    if (v > 0x7FFFFFFFu)
        v = 0x7FFFFFFFu;
    if (v < 1u)
        v = 1u;
    return (uint32_t)v;
}

static inline uint32_t deck_pitch_skew_inc(uint32_t base, int32_t pitch)
{
    int64_t v = (int64_t)base + ((int64_t)pitch >> 10);

    if (v < 1)
        v = 1;
    return (uint32_t)v;
}

static void deck_svf_lp(int32_t in, int32_t fc_q15, int32_t *l1, int32_t *l2, int32_t *out)
{
    int32_t g = fc_q15 >> 7;
    int32_t damp = 200 - (fc_q15 >> 7);
    int32_t low = *l2;
    int32_t band = *l1;
    int32_t high;

    if (g < 2)
        g = 2;
    if (g > 240)
        g = 240;
    if (damp < 96)
        damp = 96;
    high = in - low - ((band * damp) >> 8);
    band += (high * g) >> 8;
    low += (band * g) >> 8;
    if (band > 28000)
        band = 28000;
    else if (band < -28000)
        band = -28000;
    if (low > 28000)
        low = 28000;
    else if (low < -28000)
        low = -28000;
    *l1 = band;
    *l2 = low;
    *out = low;
}

static int32_t deck_crush_sample(deck_voice_t *d, int32_t x, uint8_t crush)
{
    int32_t bits;
    int32_t hold;

    if (crush < 4u)
        return x;
    bits = 14 - (int32_t)(crush >> 4);
    if (bits < 4)
        bits = 4;
    if (bits > 14)
        bits = 14;
    hold = 1 + (int32_t)(crush >> 3);
    if (hold > 32)
        hold = 32;
    if (d->crush_cnt == 0u) {
        int32_t q = 1 << (15 - bits);

        d->crush_hold = (x / q) * q;
        d->crush_cnt = (uint8_t)hold;
    } else
        d->crush_cnt--;
    return d->crush_hold;
}

static int32_t deck_filter_voice(deck_voice_t *d, int32_t x, uint8_t filt)
{
    int32_t fc;
    int32_t out;

    if (filt < 8u)
        return x;
    fc = 400 + ((int32_t)filt * (int32_t)filt * 3);
    if (fc > 30000)
        fc = 30000;
    deck_svf_lp(x, fc, &d->flt_b, &d->flt_l, &out);
    /* SVF tops out near 6.7 kHz — fade to dry over the last knob steps so hats can open up. */
    if (filt >= 120u)
        out += ((x - out) * (int32_t)(filt - 119u)) >> 3;
    return out;
}

/* Master DJ filter: TPT state-variable filter (stable up to Nyquist), Q 0.9 fixed. DECK_MF_COEF[i] = {a1, a2, a3}
 * in Q30 for fc = 20 Hz * 900^(i/255) at 44.1 kHz (20 Hz .. 18 kHz, log-spaced). */
enum { DECK_MF_N = 256, DECK_MF_LP_LO = 76, DECK_MF_HP_HI = 194 };
#define DECK_MF_K_Q30 1193046471LL
#define DECK_MF_STATE_LIM ((int64_t)1 << 30)
#define DECK_MF_IN_LIM (1 << 18)

static const int32_t DECK_MF_COEF[DECK_MF_N][3] = {
    {1072042534, 1527403, 2176}, {1071996607, 1568629, 2295},
    {1071949440, 1610966, 2421}, {1071900999, 1654444, 2554},
    {1071851249, 1699093, 2693}, {1071800155, 1744945, 2841},
    {1071747681, 1792032, 2996}, {1071693789, 1840387, 3160},
    {1071638441, 1890045, 3333}, {1071581597, 1941040, 3516},
    {1071523219, 1993407, 3708}, {1071463263, 2047185, 3911},
    {1071401688, 2102410, 4126}, {1071338449, 2159121, 4351},
    {1071273502, 2217359, 4590}, {1071206801, 2277164, 4841},
    {1071138299, 2338577, 5106}, {1071067946, 2401643, 5385},
    {1070995693, 2466406, 5680}, {1070921489, 2532910, 5991},
    {1070845280, 2601202, 6319}, {1070767014, 2671331, 6664},
    {1070686634, 2743345, 7029}, {1070604083, 2817295, 7414},
    {1070519303, 2893232, 7819}, {1070432234, 2971209, 8247},
    {1070342813, 3051281, 8698}, {1070250978, 3133504, 9174},
    {1070156664, 3217936, 9676}, {1070059803, 3304634, 10206},
    {1069960327, 3393660, 10764}, {1069858165, 3485076, 11353},
    {1069753245, 3578945, 11974}, {1069645493, 3675332, 12629},
    {1069534832, 3774305, 13319}, {1069421184, 3875933, 14048},
    {1069304469, 3980285, 14816}, {1069184603, 4087435, 15626},
    {1069061502, 4197457, 16480}, {1068935079, 4310427, 17382},
    {1068805244, 4426423, 18332}, {1068671905, 4545526, 19334},
    {1068534969, 4667818, 20391}, {1068394337, 4793383, 21506},
    {1068249911, 4922309, 22681}, {1068101588, 5054684, 23921},
    {1067949263, 5190599, 25228}, {1067792830, 5330149, 26607},
    {1067632176, 5473428, 28061}, {1067467189, 5620537, 29594},
    {1067297752, 5771575, 31211}, {1067123745, 5926647, 32916},
    {1066945046, 6085858, 34714}, {1066761528, 6249318, 36610},
    {1066573061, 6417139, 38609}, {1066379512, 6589435, 40718},
    {1066180745, 6766324, 42941}, {1065976620, 6947926, 45286},
    {1065766992, 7134366, 47758}, {1065551714, 7325770, 50365},
    {1065330634, 7522268, 53115}, {1065103596, 7723993, 56013},
    {1064870439, 7931083, 59070}, {1064631001, 8143677, 62293},
    {1064385111, 8361919, 65692}, {1064132597, 8585956, 69276},
    {1063873281, 8815939, 73055}, {1063606981, 9052024, 77039},
    {1063333509, 9294368, 81240}, {1063052673, 9543133, 85670},
    {1062764275, 9798488, 90340}, {1062468113, 10060602, 95265},
    {1062163978, 10329650, 100457}, {1061851658, 10605811, 105931},
    {1061530932, 10889270, 111703}, {1061201576, 11180213, 117788},
    {1060863359, 11478835, 124204}, {1060516043, 11785331, 130968},
    {1060159386, 12099905, 138100}, {1059793136, 12422763, 145618},
    {1059417039, 12754117, 153544}, {1059030830, 13094184, 161901},
    {1058634239, 13443188, 170710}, {1058226989, 13801354, 179997},
    {1057808796, 14168917, 189787}, {1057379367, 14546115, 200107},
    {1056938403, 14933191, 210987}, {1056485595, 15330396, 222456},
    {1056020629, 15737985, 234545}, {1055543180, 16156220, 247288},
    {1055052916, 16585368, 260721}, {1054549496, 17025704, 274880},
    {1054032568, 17477506, 289804}, {1053501775, 17941062, 305535},
    {1052956748, 18416665, 322115}, {1052397108, 18904613, 339591},
    {1051822467, 19405213, 358009}, {1051232428, 19918777, 377421},
    {1050626581, 20445626, 397880}, {1050004508, 20986087, 419442},
    {1049365778, 21540493, 442165}, {1048709951, 22109185, 466112},
    {1048036574, 22692512, 491347}, {1047345183, 23290830, 517941},
    {1046635303, 23904501, 545964}, {1045906444, 24533898, 575493},
    {1045158107, 25179398, 606609}, {1044389777, 25841387, 639395},
    {1043600928, 26520261, 673940}, {1042791020, 27216420, 710337},
    {1041959498, 27930276, 748686}, {1041105796, 28662246, 789088},
    {1040229330, 29412756, 831653}, {1039329504, 30182242, 876496},
    {1038405706, 30971144, 923735}, {1037457308, 31779916, 973498},
    {1036483667, 32609015, 1025919}, {1035484123, 33458909, 1081135},
    {1034458001, 34330074, 1139296}, {1033404608, 35222994, 1200555},
    {1032323235, 36138162, 1265075}, {1031213154, 37076078, 1333028},
    {1030073620, 38037251, 1404591}, {1028903869, 39022199, 1479956},
    {1027703120, 40031447, 1559319}, {1026470570, 41065528, 1642889},
    {1025205399, 42124984, 1730887}, {1023906766, 43210365, 1823541},
    {1022573811, 44322227, 1921093}, {1021205652, 45461136, 2023799},
    {1019801385, 46627664, 2131924}, {1018360087, 47822389, 2245749},
    {1016880812, 49045900, 2365568}, {1015362592, 50298788, 2491689},
    {1013804435, 51581655, 2624438}, {1012205328, 52895107, 2764155},
    {1010564233, 54239755, 2911196}, {1008880088, 55616218, 3065938},
    {1007151808, 57025119, 3228773}, {1005378282, 58467086, 3400113},
    {1003558376, 59942750, 3580393}, {1001690927, 61452749, 3770065},
    {999774749, 62997721, 3969607}, {997808630, 64578309, 4179517},
    {995791330, 66195158, 4400318}, {993721582, 67848914, 4632560},
    {991598093, 69540222, 4876817}, {989419542, 71269731, 5133691},
    {987184580, 73038087, 5403814}, {984891830, 74845933, 5687847},
    {982539885, 76693912, 5986481}, {980127313, 78582663, 6300442},
    {977652649, 80512818, 6630488}, {975114402, 82485008, 6977414},
    {972511049, 84499852, 7342050}, {969841041, 86557965, 7725267},
    {967102795, 88659949, 8127974}, {964294702, 90806399, 8551123},
    {961415122, 92997896, 8995707}, {958462384, 95235006, 9462767},
    {955434789, 97518280, 9953390}, {952330608, 99848254, 10468711},
    {949148082, 102225443, 11009917}, {945885421, 104650341, 11578246},
    {942540809, 107123421, 12174992}, {939112398, 109645127, 12801507},
    {935598312, 112215881, 13459199}, {931996649, 114836072, 14149540},
    {928305474, 117506058, 14874063}, {924522831, 120226163, 15634368},
    {920646731, 122996672, 16432124}, {916675162, 125817833, 17269070},
    {912606086, 128689850, 18147016}, {908437440, 131612879, 19067851},
    {904167136, 134587032, 20033541}, {899793065, 137612365, 21046131},
    {895313093, 140688880, 22107753}, {890725068, 143816520, 23220623},
    {886026816, 146995164, 24387048}, {881216147, 150224627, 25609425},
    {876290851, 153504653, 26890248}, {871248704, 156834910, 28232110},
    {866087467, 160214989, 29637703}, {860804890, 163644399, 31109825},
    {855398711, 167122558, 32651381}, {849866660, 170648797, 34265389},
    {844206460, 174222346, 35954980}, {838415828, 177842336, 37723401},
    {832492480, 181507788, 39574024}, {826434131, 185217615, 41510344},
    {820238497, 188970608, 43535985}, {813903301, 192765438, 45654704},
    {807426269, 196600645, 47870394}, {800805140, 200474635, 50187089},
    {794037664, 204385672, 52608969}, {787121605, 208331872, 55140361},
    {780054746, 212311199, 57785746}, {772834891, 216321452, 60549764},
    {765459866, 220360264, 63437220}, {757927525, 224425093, 66453085},
    {750235753, 228513210, 69602504}, {742382466, 232621699, 72890804},
    {734365617, 236747441, 76323495}, {726183198, 240887110, 79906282},
    {717833245, 245037161, 83645067}, {709313838, 249193823, 87545961},
    {700623108, 253353087, 91615286}, {691759238, 257510695, 95859592},
    {682720466, 261662131, 100285657}, {673505091, 265802605, 104900506},
    {664111473, 269927043, 109711414}, {654538040, 274030074, 114725924},
    {644783289, 278106010, 119951857}, {634845791, 282148835, 125397327},
    {624724194, 286152185, 131070757}, {614417228, 290109329, 136980897},
    {603923706, 294013149, 143136841}, {593242536, 297856114, 149548050},
    {582372716, 301630260, 156224375}, {571313349, 305327156, 163176079},
    {560063640, 308937880, 170413873}, {548622911, 312452979, 177948937},
    {536990604, 315862431, 185792963}, {525166291, 319155609, 193958190},
    {513149684, 322321227, 202457443}, {500940651, 325347287, 211304187},
    {488539225, 328221025, 220512572}, {475945625, 330928836, 230097492},
    {463160276, 333456207, 240074651}, {450183835, 335787622, 250460631},
    {437017216, 337906475, 261272970}, {423661632, 339794951, 272530247},
    {410118632, 341433911, 284252180}, {396390159, 342802741, 296459730},
    {382478603, 343879202, 309175218}, {368386885, 344639237, 322422454},
    {354118540, 345056765, 336226878}, {339677831, 345103445, 350615721},
    {325069875, 344748394, 365618177}, {310300809, 343957879, 381265594},
    {295377976, 342694947, 397591685}, {280310161, 340919015, 414632757},
    {265107876, 338585381, 432427969}, {249783704, 335644671, 451019596},
    {234352716, 332042193, 470453338}, {218832989, 327717186, 490778627},
    {203246229, 322601958, 512048975}, {187618541, 316620868, 534322319},
    {171981373, 309689153, 557661392}, {156372678, 301711559, 582134080},
    {140838351, 292580738, 607813765}, {125434002, 282175384, 634779618},
    {110227161, 270358066, 663116812}, {95300017, 256972690, 692916595},
    {80752832, 241841577, 724276128}, {66708202, 224762068, 757297990},
};

static inline int64_t deck_mf_clamp(int64_t v)
{
    return v > DECK_MF_STATE_LIM ? DECK_MF_STATE_LIM : v < -DECK_MF_STATE_LIM ? -DECK_MF_STATE_LIM : v;
}

/* FILTER knob → mode (0 LP / 1 HP, -1 = centre deadzone 63..65) and cutoff index (Q8). LP: 0 = 150 Hz .. 62 = 18 kHz;
 * HP: 66 = 20 Hz .. 127 = 3.5 kHz. */
static int deck_mf_target(uint8_t f, int32_t *idx_q8)
{
    if (f > 127u)
        f = 127u;
    if (f < 63u) {
        *idx_q8 = (DECK_MF_LP_LO << 8) + (int32_t)(((uint32_t)f * (uint32_t)(DECK_MF_N - 1 - DECK_MF_LP_LO) << 8) / 62u);
        return 0;
    }
    if (f > 65u) {
        /* Split d*k/61 exactly: folded d*k = f*k - 66*k lands in the ROM range and trips build.py's check. */
        uint32_t d = (uint32_t)(f - 66u), k = (uint32_t)DECK_MF_HP_HI << 8;

        *idx_q8 = (int32_t)(d * (k / 61u) + d * (k % 61u) / 61u);
        return 1;
    }
    return -1;
}

static int32_t deck_mf_svf(int64_t *ic1, int64_t *ic2, int32_t x, const int32_t *c, uint8_t hp)
{
    int64_t v0, v1, v2, v3, y;

    if (x > DECK_MF_IN_LIM)
        x = DECK_MF_IN_LIM;
    else if (x < -DECK_MF_IN_LIM)
        x = -DECK_MF_IN_LIM;
    v0 = (int64_t)x * 4096;
    v3 = v0 - *ic2;
    v1 = ((int64_t)c[0] * *ic1 + (int64_t)c[1] * v3) >> 30;
    v2 = *ic2 + (((int64_t)c[1] * *ic1 + (int64_t)c[2] * v3) >> 30);
    *ic1 = deck_mf_clamp(2 * v1 - *ic1);
    *ic2 = deck_mf_clamp(2 * v2 - *ic2);
    y = hp ? v0 - ((DECK_MF_K_Q30 * deck_mf_clamp(v1)) >> 30) - v2 : v2;
    y >>= 12;
    if (y > DECK_MF_IN_LIM)
        y = DECK_MF_IN_LIM;
    else if (y < -DECK_MF_IN_LIM)
        y = -DECK_MF_IN_LIM;
    return (int32_t)y;
}

/* One sample of the stereo (mid/side) DJ filter. wet == 0 returns the inputs untouched (bit-exact bypass). */
static void deck_mf_tick(deck_dsp_t *s, int mode_t, int32_t idx_t, int32_t *mid, int32_t *side)
{
    int32_t wet_t = mode_t < 0 ? 0 : 32768;
    int32_t d;
    const int32_t *c;
    int32_t ym, ys;

    if (mode_t >= 0 && (uint8_t)mode_t != s->mf_hp)
        wet_t = 0;
    if (s->mf_wet == 0) {
        if (wet_t == 0) {
            if (mode_t >= 0)
                s->mf_hp = (uint8_t)mode_t;
            return;
        }
        s->mf_idx_q8 = idx_t;
        s->mf_ic1[0] = s->mf_ic1[1] = 0;
        s->mf_ic2[0] = deck_mf_clamp((int64_t)*mid * 4096);
        s->mf_ic2[1] = deck_mf_clamp((int64_t)*side * 4096);
    } else if (mode_t >= 0 && (uint8_t)mode_t == s->mf_hp) {
        s->mf_idx_q8 += (idx_t - s->mf_idx_q8) >> 7;
    }
    d = wet_t - s->mf_wet;
    s->mf_wet = (d >> 7) ? s->mf_wet + (d >> 7) : wet_t;
    c = DECK_MF_COEF[(s->mf_idx_q8 >> 8) & (DECK_MF_N - 1)];
    ym = deck_mf_svf(&s->mf_ic1[0], &s->mf_ic2[0], *mid, c, s->mf_hp);
    ys = deck_mf_svf(&s->mf_ic1[1], &s->mf_ic2[1], *side, c, s->mf_hp);
    *mid += (int32_t)(((int64_t)(ym - *mid) * s->mf_wet) >> 15);
    *side += (int32_t)(((int64_t)(ys - *side) * s->mf_wet) >> 15);
}

/* 80s drum-machine-ish defaults (0..127). */
static const uint8_t DECK_CFG_DEF[DECK_NVOICES][8] = {
    {120, 64, 72, 64, 40, 0, 48, 44},  /* BD */
    {108, 64, 78, 64, 90, 0, 72, 36},  /* SN */
    {96, 64, 36, 64, 70, 0, 88, 28},   /* RM */
    {88, 64, 48, 64, 36, 0, 100, 22},   /* CH — metal partials, low hiss knob */
    {100, 64, 88, 64, 42, 0, 98, 26},  /* OH */
    {96, 64, 76, 64, 95, 0, 80, 32},   /* CP */
    {88, 64, 42, 64, 110, 0, 100, 20}, /* CB */
    {90, 64, 38, 64, 100, 0, 94, 22},  /* TB */
    {90, 64, 34, 64, 30, 0, 86, 38},   /* CW */
    {96, 64, 68, 64, 35, 0, 70, 34},   /* TH */
    {94, 64, 70, 64, 38, 0, 68, 32},   /* TM */
    {92, 64, 72, 64, 40, 0, 66, 30},   /* TL */
    {90, 64, 58, 64, 45, 0, 64, 36},   /* CNH */
    {88, 64, 56, 64, 48, 0, 62, 34},   /* CNL */
    {94, 64, 112, 64, 54, 0, 96, 20},  /* RD — long wash, metal partials */
    {98, 64, 72, 64, 88, 0, 104, 36},  /* CR — bright burst, shorter cfg decay */
};

static void deck_voice_cfg_default(deck_voice_cfg_t *c, uint8_t voice)
{
    const uint8_t *d;

    if (voice >= DECK_NVOICES)
        voice = 0u;
    d = DECK_CFG_DEF[voice];
    c->level = d[0];
    c->tune = d[1];
    c->decay = d[2];
    c->pitch_env = d[3];
    c->timbre = 64u;
    c->noise = d[4];
    c->crush = d[5];
    c->filter = d[6];
    c->drive = d[7];
    c->pan = 64u;
    c->click = 64u;
    c->hold = 0u;
    c->pitch_dec = 64u;
    c->choke_grp = (voice == DECK_CH || voice == DECK_OH) ? 1u : 0u;
    c->curve = 64u;
    c->vel_sens = 80u;
    c->sustain = 0u;
    c->release = 64u;
    c->dly_send = 127u;
    c->rev_send = 127u;
    c->duck = 0u;
}

static void deck_master_default(deck_master_t *m)
{
    m->dly_lvl = 0u;
    m->dly_time = 64u;
    m->feedback = 48u;
    m->loop_filt = 64u;
    m->wow = 0u;
    m->tap = 0u;
    m->tank = 64u;
    m->isolator = 64u;
    m->rev_mix = 0u;
    m->rev_damp = 64u;
    m->rev_tail = 64u;
    m->dly_rev = 0u;
    m->drive = 24u;
    m->crush = 0u;
    m->filter = 64u;
    m->width = 40u;
    m->comp = 0u;
    m->dust = 0u;
    m->tape = 0u;
}

static void deck_perf_default(deck_perf_t *p)
{
    p->tune = 64u;
    p->decay = 64u;
    p->filter = 64u;
    p->crush = 64u;
    p->density = 64u;
    p->fill = 0u;
    p->mute_grp = 0u;
    p->stutter = 0u;
}

static uint8_t deck_u8_off(uint8_t base, uint8_t center)
{
    int32_t v = (int32_t)base + (int32_t)center - 64;

    if (v < 0)
        v = 0;
    if (v > 127)
        v = 127;
    return (uint8_t)v;
}

static void deck_dsp_init(deck_dsp_t *s)
{
    uint32_t v;

    if (!s)
        return;
    for (v = 0; v < DECK_NVOICES; v++) {
        s->v[v].active = 0u;
        s->v[v].env = 0;
        s->v[v].env2 = 0;
        s->v[v].hat_env = 0;
        s->v[v].oh_ring = 0;
        s->env_invert[v] = 0u;
        s->env_delay[v] = 0u;
        s->env_retrig[v] = 1u;
        deck_voice_cfg_default(&s->cfg[v], (uint8_t)v);
    }
    deck_master_default(&s->master);
    deck_perf_default(&s->perf);
    s->mf_ic1[0] = s->mf_ic1[1] = 0;
    s->mf_ic2[0] = s->mf_ic2[1] = 0;
    s->mf_idx_q8 = 255 << 8;
    s->mf_wet = 0;
    s->mf_hp = 0u;
    s->m_comp = 0;
    s->dust_ph = 1u;
    s->crush_hold = 0;
    s->crush_cnt = 0u;
    s->era = 0u;
    s->duck_atk = 0u;
    s->duck_peak = 0;
    s->duck_env = 0;
    s->render_bpm = 120u;
    s->dly_w = 0u;
    s->wow_ph = 0u;
    s->dly_filt = 0;
    s->iso_lp = 0;
    s->iso_hp = 0;
    s->iso_lp_r = 0;
    s->iso_hp_r = 0;
    for (v = 0u; v < 4u; v++) {
        s->rev_pos[v] = 0u;
        s->rev_lp[v] = 0;
    }
}

static inline uint16_t deck_env_delay_samples(uint8_t delay)
{
    return fm_env_time_samples(delay, DECK_FS);
}

/* click doubles as transient amount (plates/eras live in 16..88); only the top of the range swells. */
#define DECK_ATK_KNEE 96u

static inline uint16_t deck_env_attack_samples(const deck_voice_cfg_t *c)
{
    if (c->click <= DECK_ATK_KNEE)
        return 0u;
    return fm_env_time_samples((uint8_t)((c->click - DECK_ATK_KNEE) * 2u), DECK_FS);
}

static inline void deck_env_begin_attack(deck_voice_t *d, const deck_voice_cfg_t *c, int32_t peak)
{
    uint16_t atk;

    if (!d || !c || peak < DECK_AMP_ENV_FLOOR)
        return;
    atk = deck_env_attack_samples(c);
    if (!atk) {
        d->env_atk_left = 0u;
        return;
    }
    d->env_atk_left = atk;
    d->env = DECK_AMP_ENV_FLOOR;
    d->env2 = DECK_AMP_ENV_FLOOR;
    d->hat_env = DECK_AMP_ENV_FLOOR;
    d->oh_ring = DECK_AMP_ENV_FLOOR;
}

static inline void deck_env_attack_tick(deck_voice_t *d, const deck_voice_cfg_t *c)
{
    uint16_t total;
    uint32_t done;
    uint8_t prog;
    int32_t peak;
    int32_t lev;

    if (!d || !c || !d->env_atk_left)
        return;
    total = deck_env_attack_samples(c);
    if (total < 1u)
        total = 1u;
    done = (uint32_t)total - (uint32_t)d->env_atk_left;
    prog = (uint8_t)((done * 128u) / (uint32_t)total);
    if (d->env_atk_left == 1u)
        prog = 128u;
    if (prog > 128u)
        prog = 128u;
    peak = (int32_t)d->vel << 9;
    if (peak < 1)
        peak = 1;
    lev = fm_env_attack_level(DECK_AMP_ENV_FLOOR, peak, prog, c->curve);
    d->env = lev;
    d->env2 = lev;
    if (d->hat_env >= DECK_AMP_ENV_FLOOR || d->oh_ring >= DECK_AMP_ENV_FLOOR) {
        d->hat_env = lev;
        d->oh_ring = lev;
    }
    d->env_atk_left--;
}

static inline int deck_voice_env_dly_tick(deck_voice_t *d, const deck_voice_cfg_t *c)
{
    if (!d || !d->env_dly_left)
        return 1;
    d->env_dly_left--;
    if (d->env_dly_left == 0u) {
        int32_t peak = d->env_pend_e > d->env_pend_e2 ? d->env_pend_e : d->env_pend_e2;

        if (peak < DECK_AMP_ENV_FLOOR)
            peak = (int32_t)d->vel << 9;
        deck_env_begin_attack(d, c, peak);
        return 1;
    }
    return 0;
}

static void deck_voice_reset(deck_voice_t *d)
{
    d->ph = 0u;
    d->mph = 0u;
    d->bp1 = 0;
    d->bp2 = 0;
    d->hp = 0;
    d->shell1 = 0;
    d->shell2 = 0;
    d->click = 0;
    d->choke = 0u;
    d->crush_cnt = 0u;
    d->crush_hold = 0;
    d->env_dly_left = 0u;
    d->env_pend_e = 0;
    d->env_pend_e2 = 0;
    d->env_atk_left = 0u;
}

static void deck_voice_gate_close(deck_voice_t *d)
{
    if (!d)
        return;
    d->gate_left = 0u;
    d->env >>= 3;
    d->env2 >>= 3;
    d->click >>= 2;
    d->hat_env >>= 3;
    d->oh_ring >>= 3;
    if (d->env < 48 && d->env2 < 48 && d->hat_env < 48)
        d->active = 0u;
}

static inline void deck_voice_gate_tick(deck_voice_t *d)
{
    if (!d || !d->gate_left)
        return;
    d->gate_left--;
    if (!d->gate_left)
        deck_voice_gate_close(d);
}

static void deck_dsp_choke_hats(deck_dsp_t *s)
{
    if (!s)
        return;
    if (s->v[DECK_OH].active && s->v[DECK_OH].oh_ring > 0)
        s->v[DECK_OH].choke = 2u;
    if (s->v[DECK_CH].active && s->v[DECK_CH].hat_env > 0)
        s->v[DECK_CH].hat_env >>= 2;
}

/* Seq stopped: kill hat/ride/crash metallic tails (no idle FM partials). */
static void deck_dsp_halt_metallic_idle(deck_dsp_t *s, uint8_t seq_playing)
{
    static const uint8_t metallic[] = {DECK_CH, DECK_OH, DECK_RD, DECK_CR};
    uint32_t i;

    if (!s || seq_playing)
        return;
    for (i = 0u; i < sizeof(metallic); i++) {
        deck_voice_t *d = &s->v[metallic[i]];

        if (!d->active)
            continue;
        if (d->hat_env > DECK_AMP_ENV_FLOOR || d->oh_ring > DECK_AMP_ENV_FLOOR)
            continue;
        d->hat_env = 0;
        d->oh_ring = 0;
        d->active = 0u;
        d->choke = 0u;
        d->hp = 0;
        d->bp1 = 0;
        d->bp2 = 0;
    }
}

static void deck_dsp_trig_st(deck_dsp_t *s, uint8_t voice, uint8_t vel, uint8_t accent, uint8_t slide_legato,
                             int8_t semi);

/* ACCENT / SLIDE step level 0..3 (3 = full, the former binary ON). */
static inline int32_t deck_dsp_lv(uint8_t lv)
{
    return lv > 3u ? 3 : (int32_t)lv;
}

static void deck_dsp_trig(deck_dsp_t *s, uint8_t voice, uint8_t vel, uint8_t accent)
{
    deck_dsp_trig_st(s, voice, vel, accent, 0u, 0);
}

static void deck_dsp_trig_ex(deck_dsp_t *s, uint8_t voice, uint8_t vel, uint8_t accent, uint8_t slide_legato)
{
    deck_dsp_trig_st(s, voice, vel, accent, slide_legato, 0);
}

static void deck_dsp_trig_st(deck_dsp_t *s, uint8_t voice, uint8_t vel, uint8_t accent, uint8_t slide_legato,
                             int8_t semi)
{
    deck_voice_t *d;
    deck_voice_cfg_t *c;
    int32_t vlev;
    int32_t prev_pitch;
    int32_t prev_env;
    uint32_t inc;
    uint8_t note;
    uint32_t h;

    if (!s || voice >= DECK_NVOICES || !vel)
        return;
    d = &s->v[voice];
    c = &s->cfg[voice];
    prev_pitch = d->pitch;
    prev_env = d->env > d->hat_env ? d->env : d->hat_env;
    if (!slide_legato && !s->env_retrig[voice] && d->active && prev_env > 512)
        return;
    {
        int32_t vs = 64 + (((int32_t)c->vel_sens - 64) >> 1);

        vlev = ((int32_t)vel * vs) >> 6;
        if (vlev < 1)
            vlev = 1;
        if (vlev > 127)
            vlev = 127;
    }
    d->vel = (uint8_t)vlev;
    d->accent = accent;
    d->active = 1u;
    d->choke = 0u;
    if (semi < -DECK_SEMI_MAX)
        semi = -DECK_SEMI_MAX;
    if (semi > DECK_SEMI_MAX)
        semi = DECK_SEMI_MAX;
    d->semi = semi;
    if (!slide_legato)
        deck_voice_reset(d);
    else
        d->click >>= 1;

    switch (voice) {
    case DECK_BD:
        s->duck_peak = (int32_t)d->vel << 8;
        s->duck_atk = 1u;
        d->env = (int32_t)d->vel << 9;
        d->env2 = (int32_t)d->vel << 9;
        d->pitch = deck_pitch_sweep_ph_sc(c, 247 << 15);
        d->mod_env = (int32_t)c->timbre << 8;
        d->nph = 0xBD001337u ^ ((uint32_t)vel << 12);
        d->click = (((int32_t)d->vel << 7) * (3 + deck_dsp_lv(accent)) / 3) * (16 + (int32_t)c->click) >> 6;
        break;
    case DECK_SN:
        d->env = (int32_t)d->vel << 9;
        d->env2 = (int32_t)d->vel << 9;
        d->pitch = deck_pitch_sweep_ph_sc(c, 207 << 14);
        d->nph = 2463534242u ^ ((uint32_t)vel << 16);
        d->click = ((int32_t)d->vel << 6) * (1 + deck_dsp_lv(accent));
        break;
    case DECK_RM:
        d->env = (int32_t)d->vel << 9;
        d->env2 = (int32_t)d->vel << 8;
        d->nph = 11223344u ^ vel;
        d->click = (int32_t)d->vel << 9;
        d->shell1 = (int32_t)((199u + ((uint32_t)c->tune << 2)) * 97391u);
        d->pitch = deck_pitch_sweep_ph_sc(c, 252 * 97391);
        break;
    case DECK_CH:
        deck_dsp_choke_hats(s);
        d->hat_env = (int32_t)d->vel << 9;
        d->pitch = deck_pitch_sweep_ph_sc(c, 1100000 >> 6);
        for (h = 0; h < 6u; h++)
            d->hat_ph[h] = (h + 1u) * 0x2AAAAAAu;
        d->nph = 0xC4A7F00Du ^ ((uint32_t)vel << 9);
        d->mod_env = 0;
        break;
    case DECK_OH:
        d->oh_ring = (int32_t)d->vel << 9;
        d->hat_env = d->oh_ring;
        d->pitch = deck_pitch_sweep_ph_sc(c, 1100000 >> 6);
        for (h = 0; h < 6u; h++)
            d->hat_ph[h] = (h + 2u) * 0x2492492u;
        d->nph = 0x0BADC0DEu ^ ((uint32_t)vel << 9);
        d->mod_env = 0;
        if (s->v[DECK_CH].active)
            s->v[DECK_CH].hat_env >>= 2;
        break;
    case DECK_CP:
        d->clap_idx = 1u;
        d->clap_env[0] = (int32_t)d->vel << 9;
        d->clap_env[1] = 0;
        d->clap_env[2] = 0;
        d->clap_env[3] = 0;
        d->nph = 54321u ^ ((uint32_t)vel << 11);
        d->env = (int32_t)d->vel << 9;
        d->pitch = deck_pitch_sweep_ph_sc(c, 850 << 10);
        break;
    case DECK_CB:
    case DECK_TB:
        d->env = (int32_t)d->vel << 9;
        d->nph = 99887766u ^ ((uint32_t)voice << 12);
        d->pitch = deck_pitch_sweep_ph_sc(c, 1200000 >> 8);
        break;
    case DECK_CW:
        d->env = (int32_t)d->vel << 9;
        d->sq_ph[0] = 0u;
        d->sq_ph[1] = 0x40000000u;
        d->pitch = deck_pitch_sweep_ph_sc(c, 780000 >> 6);
        break;
    case DECK_TH:
        note = 50u;
        note = (uint8_t)(note + (c->tune >> 3) - 8);
        inc = deck_note_inc(note);
        d->shell1 = (int32_t)inc;
        d->pitch = (int32_t)(inc >> 1) + deck_pitch_sweep_ph_sc(c, (int32_t)(inc >> 5));
        d->env = (int32_t)d->vel << 9;
        d->click = (int32_t)d->vel << 7;
        break;
    case DECK_TM:
        note = 47u;
        note = (uint8_t)(note + (c->tune >> 3) - 8);
        inc = deck_note_inc(note);
        d->shell1 = (int32_t)inc;
        d->pitch = (int32_t)(inc >> 1) + deck_pitch_sweep_ph_sc(c, (int32_t)(inc >> 5));
        d->env = (int32_t)d->vel << 9;
        d->click = (int32_t)d->vel << 7;
        break;
    case DECK_TL:
        note = 43u;
        note = (uint8_t)(note + (c->tune >> 3) - 8);
        inc = deck_note_inc(note);
        d->shell1 = (int32_t)inc;
        d->pitch = (int32_t)(inc >> 1) + deck_pitch_sweep_ph_sc(c, (int32_t)(inc >> 5));
        d->env = (int32_t)d->vel << 9;
        d->click = (int32_t)d->vel << 7;
        break;
    case DECK_CNH:
        note = 64u;
        note = (uint8_t)(note + (c->tune >> 3) - 8);
        inc = deck_note_inc(note);
        d->shell1 = (int32_t)inc;
        d->pitch = (int32_t)(inc >> 1) + deck_pitch_sweep_ph_sc(c, (int32_t)(inc >> 5));
        d->env = (int32_t)d->vel << 9;
        d->click = (int32_t)d->vel << 6;
        break;
    case DECK_CNL:
        note = 59u;
        note = (uint8_t)(note + (c->tune >> 3) - 8);
        inc = deck_note_inc(note);
        d->shell1 = (int32_t)inc;
        d->pitch = (int32_t)(inc >> 1) + deck_pitch_sweep_ph_sc(c, (int32_t)(inc >> 5));
        d->env = (int32_t)d->vel << 9;
        d->click = (int32_t)d->vel << 6;
        break;
    case DECK_RD:
        d->hat_env = (int32_t)d->vel << 9;
        d->oh_ring = d->hat_env;
        d->pitch = deck_pitch_sweep_ph_sc(c, 580000 >> 6);
        for (h = 0; h < 6u; h++)
            d->hat_ph[h] = (h + 3u) * 0x2222222u;
        d->nph = 777888999u;
        break;
    case DECK_CR:
        d->hat_env = (int32_t)d->vel << 10;
        d->oh_ring = d->hat_env;
        d->pitch = deck_pitch_sweep_ph_sc(c, 580000 >> 6);
        for (h = 0; h < 6u; h++)
            d->hat_ph[h] = (h + 1u) * 0x1999999u;
        d->nph = 333444555u;
        d->mod_env = 0;
        break;
    default:
        break;
    }
    if (slide_legato && d->active && prev_env > 800) {
        d->env = prev_env + ((((int32_t)d->vel << 9) - prev_env) >> 4);
        d->env2 = d->env;
        if (prev_pitch != 0 && d->pitch != prev_pitch)
            d->pitch = prev_pitch + (int32_t)(((int64_t)(d->pitch - prev_pitch) * (4 - deck_dsp_lv(slide_legato))) >> 2);
    }
    if (!slide_legato) {
        uint16_t dl = deck_env_delay_samples(s->env_delay[voice]);

        if (dl) {
            int32_t peak = d->env > d->hat_env ? d->env : d->hat_env;

            d->env_dly_left = dl;
            d->env_pend_e = peak;
            d->env_pend_e2 = d->env2;
            d->env = DECK_AMP_ENV_FLOOR;
            d->env2 = DECK_AMP_ENV_FLOOR;
            d->hat_env = DECK_AMP_ENV_FLOOR;
            d->oh_ring = DECK_AMP_ENV_FLOOR;
            d->click = 0;
        } else {
            int32_t peak = d->env > d->hat_env ? d->env : d->hat_env;

            if (peak < DECK_AMP_ENV_FLOOR)
                peak = (int32_t)d->vel << 9;
            deck_env_begin_attack(d, c, peak);
        }
    }
    (void)c;
    (void)note;
    (void)inc;
}

/* --- Era synthesis (render mix + cfg offsets; plate-independent) --- */
typedef struct {
    int8_t timbre;
    int8_t noise;
    int8_t filter;
    int8_t click;
    int8_t pitch_env;
    int8_t pitch_dec;
} deck_era_voice_ofs_t;

typedef struct {
    uint8_t bd_fm;
    uint8_t bd_sweep;
    uint8_t bd_click;
    uint8_t sn_shell;
    uint8_t sn_wire;
    uint8_t sn_wire_dec; /* higher → faster noise-wire decay */
    uint8_t hat_fm;
    uint8_t hat_sq;
    uint8_t hat_close; /* higher → shorter closed-hat tail */
    uint8_t tom_fm;
    uint8_t tom_beater;
    uint8_t cp_burst;
} deck_era_mix_t;

static inline uint8_t deck_sat_cfg(int32_t v)
{
    if (v < 0)
        return 0u;
    if (v > 127)
        return 127u;
    return (uint8_t)v;
}

/* BD SN RM CH OH CP CB TB CW TH TM TL CNH CNL RD CR — timbre/noise/filter/click/pitch_env/pitch_dec */
static const deck_era_voice_ofs_t DECK_ERA_VOICE[DECK_ERA_COUNT][DECK_NVOICES] = {
    { /* STAX — warm, not blanket-LP */
        {-14, -6, 0, -10, -6, -8}, {0, 16, -6, -12, -8, 4}, {0, 0, 4, 0, 0, 0},
        {0, -14, -14, 0, 0, 0}, {0, -10, -12, 0, 0, 0}, {0, 4, 6, 0, 0, 0},
        {0, 0, 4, 0, 0, 0}, {0, 0, 6, 0, 0, 0}, {0, 0, 6, 0, 0, 0},
        {6, 0, 0, -4, 0, -4}, {6, 0, 0, -4, 0, -4}, {4, 0, 0, -4, 0, -4},
        {8, 0, 2, 0, 0, 0}, {6, 0, 2, 0, 0, 0}, {-6, -8, 4, 0, 0, 0}, {-8, -6, 2, 0, 0, 0},
    },
    { /* DEAD */
        {0, 0, 10, 4, 0, 0}, {0, 14, 10, 8, 2, 0}, {0, 0, 10, 0, 0, 0},
        {0, -8, 6, 0, 0, 0}, {0, -4, 6, 0, 0, 0}, {0, 2, 10, 0, 0, 0},
        {0, 0, 10, 0, 0, 0}, {0, 0, 10, 0, 0, 0}, {6, 0, 10, 0, 0, 0},
        {0, 0, 4, 0, 0, -2}, {0, 0, 4, 0, 0, -2}, {0, 0, 4, 0, 0, -2},
        {0, 0, 6, 0, 0, 0}, {0, 0, 6, 0, 0, 0}, {-6, -4, 2, 0, 0, 0}, {-4, -2, 4, 0, 0, 0},
    },
    { /* GATE */
        {0, 0, 14, 12, 6, 0}, {0, 22, 18, 18, 8, 0}, {0, 0, 14, 8, 4, 0},
        {0, -2, 16, 6, 4, 0}, {0, 0, 14, 4, 2, 0}, {0, 6, 12, 8, 4, 0},
        {0, 0, 10, 0, 0, 0}, {0, 0, 10, 0, 0, 0}, {0, 0, 10, 0, 0, 0},
        {12, 0, 14, 6, 8, 0}, {12, 0, 14, 6, 8, 0}, {10, 0, 14, 6, 6, 0},
        {0, 0, 10, 0, 0, 0}, {0, 0, 10, 0, 0, 0}, {0, 0, 14, 0, 0, 0}, {0, 0, 14, 0, 0, 0},
    },
    { /* CRATE — still darkest era, less mud */
        {-8, 0, -4, 0, -8, -6}, {0, 18, -4, 4, -6, 0}, {0, 0, 0, 0, 0, 0},
        {0, -4, -12, 0, 0, 0}, {0, -2, -10, 0, 0, 0}, {0, 4, 4, 0, 0, 0},
        {0, 0, 0, 0, 0, 0}, {0, 0, 2, 0, 0, 0}, {0, 0, 0, 0, 0, 0},
        {0, 0, -2, 0, 0, 0}, {0, 0, -2, 0, 0, 0}, {0, 0, -2, 0, 0, 0},
        {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}, {4, 2, 0, 0, 0, 0}, {2, 0, -2, 0, 0, 0},
    },
    { /* GRID */
        {0, 0, 10, 14, 0, 4}, {0, -8, 12, 6, 0, 0}, {0, 0, 14, 8, 0, 0},
        {0, -4, 18, 12, 0, 0}, {0, -2, 16, 10, 0, 0}, {0, -4, 12, 8, 0, 0},
        {0, 0, 10, 0, 0, 0}, {0, 0, 10, 0, 0, 0}, {0, 0, 10, 0, 0, 0},
        {0, 0, 12, 8, 0, 2}, {0, 0, 12, 8, 0, 2}, {0, 0, 12, 8, 0, 2},
        {0, 0, 10, 0, 0, 0}, {0, 0, 10, 0, 0, 0}, {-6, -4, 14, 0, 0, 0}, {-4, -2, 12, 0, 0, 0},
    },
    { /* TRAP */
        {-18, 0, 10, 0, 16, 12}, {0, 8, 10, 4, 4, 0}, {0, 0, 10, 0, 0, 0},
        {0, -6, 12, 8, 0, 0}, {0, -4, 10, 6, 0, 0}, {0, 8, 10, 0, 0, 0},
        {0, 0, 10, 0, 0, 0}, {0, 0, 10, 0, 0, 0}, {0, 0, 10, 0, 0, 0},
        {6, 0, 6, 0, 10, 6}, {6, 0, 6, 0, 10, 6}, {8, 0, 6, 0, 12, 8},
        {0, 0, 10, 0, 0, 0}, {0, 0, 10, 0, 0, 0}, {-10, -8, 0, 0, 0, 0}, {-8, -6, -2, 0, 0, 0},
    },
};

static const deck_era_mix_t DECK_ERA_MIX[DECK_ERA_COUNT] = {
    /* STAX */ {14, 50, 54, 94, 58, 52, 40, 56, 96, 80, 54, 64},
    /* DEAD */ {22, 56, 58, 82, 72, 58, 62, 58, 100, 72, 58, 68},
    /* GATE */ {44, 62, 94, 26, 124, 112, 100, 94, 118, 112, 78, 74},
    /* CRATE */ {20, 42, 62, 84, 88, 70, 48, 50, 92, 68, 62, 74},
    /* GRID */ {18, 48, 100, 64, 84, 84, 54, 96, 120, 52, 92, 68},
    /* TRAP */ {6, 112, 58, 44, 68, 96, 78, 82, 126, 104, 50, 112},
};

static inline const deck_era_mix_t *deck_era_mix(uint8_t era)
{
    return &DECK_ERA_MIX[era % DECK_ERA_COUNT];
}

static inline void deck_era_voice_apply(deck_voice_cfg_t *c, uint8_t voice, uint8_t era)
{
    const deck_era_voice_ofs_t *e;

    if (!c || voice >= DECK_NVOICES)
        return;
    era %= DECK_ERA_COUNT;
    e = &DECK_ERA_VOICE[era][voice];
    c->timbre = deck_sat_cfg((int32_t)c->timbre + (int32_t)e->timbre);
    c->noise = deck_sat_cfg((int32_t)c->noise + (int32_t)e->noise);
    c->filter = deck_sat_cfg((int32_t)c->filter + (int32_t)e->filter);
    c->click = deck_sat_cfg((int32_t)c->click + (int32_t)e->click);
    c->pitch_env = deck_sat_cfg((int32_t)c->pitch_env + (int32_t)e->pitch_env);
    c->pitch_dec = deck_sat_cfg((int32_t)c->pitch_dec + (int32_t)e->pitch_dec);
}

/* BD TIMBR: ≤64 ≈ sine body + pitch sweep; >64 adds FM, click harmonics, noise. */
static inline int32_t deck_kick_timb_extra(uint8_t t)
{
    int32_t x = (int32_t)t - 64;

    return x > 0 ? x : 0;
}

static int32_t deck_render_kick(deck_voice_t *d, const deck_voice_cfg_t *c, uint8_t inv, uint8_t era)
{
    const deck_era_mix_t *m = deck_era_mix(era);
    int32_t p_shift = 6 + (c->decay >> 5) + ((int32_t)c->pitch_dec >> 6);
    int32_t body, fm, click, out;
    int32_t amp, amp2;
    int32_t timb_x = (deck_kick_timb_extra(c->timbre) * (int32_t)m->bd_fm) >> 6;
    uint32_t base_inc = deck_note_inc((uint8_t)(28u + ((uint32_t)c->tune * 24u) / 127u));
    uint32_t sweep_inc =
        (uint32_t)(d->pitch >> 4) * (380u + ((deck_pitch_sweep_mag(c) * (int32_t)m->bd_sweep) >> 6) * 2u);
    uint32_t inc = deck_semi_inc(base_inc + sweep_inc, d->semi);
    uint32_t dsh = 9u + (uint32_t)(c->decay >> 5);

    if (era == 5u && c->timbre <= 64u)
        timb_x = 0;
    if (c->hold && d->env > 16000)
        dsh += 2u + ((uint32_t)c->hold >> 6);
    d->pitch -= d->pitch >> p_shift;
    if (d->pitch < 0)
        d->pitch = 0;
    d->env = deck_env_decay(d, d->env, (int32_t)d->vel << 9, c, dsh);
    d->env2 = deck_env_decay(d, d->env2, (int32_t)d->vel << 9, c, 10u + (uint32_t)(c->decay >> 4));
    d->mod_env = deck_decay(d->mod_env, 8u);
    d->mod_env += (((int32_t)c->timbre << 8) - d->mod_env) >> 5;
    amp = deck_amp_env(d, d->env, inv);
    amp2 = deck_amp_env(d, d->env2, inv);
    d->ph += inc;
    d->mph += inc + (inc >> 1);
    fm = deck_sine(d->mph);
    body = deck_sine(d->ph);
    if (timb_x > 0)
        body = deck_sine(d->ph + (uint32_t)(((int64_t)fm * d->mod_env * timb_x) >> 6));
    click = (int32_t)(((int64_t)d->click * deck_sine(d->mph << 2)) >> 15);
    if (timb_x > 0)
        click += (int32_t)(((int64_t)d->click * timb_x * deck_sine(d->mph << 3)) >> 17);
    click = (int32_t)(((int64_t)click * amp * (int32_t)m->bd_click) >> 21);
    d->click -= (d->click >> 3) + (d->click > 0);
    out = (body * amp2) >> 15;
    if (timb_x > 0)
        out += (((fm * amp) >> 15) * timb_x) >> 6;
    out += click;
    if (timb_x > 0) {
        int32_t kn = deck_noise(&d->nph) >> (10 - (c->timbre >> 5));

        out += (int32_t)(((int64_t)kn * amp2 * timb_x * deck_timb_noise(c->timbre)) >> 19);
    }
    out = deck_fold(out, c->drive);
    if (d->env < 8 && d->env2 < 8 && d->click < 4) {
        d->env = 0;
        d->env2 = 0;
        d->active = 0u;
    }
    return deck_apply_level(out, c->level);
}

static int32_t deck_render_snare(deck_voice_t *d, const deck_voice_cfg_t *c, uint8_t inv, uint8_t era)
{
    const deck_era_mix_t *m = deck_era_mix(era);
    int32_t tone = (int32_t)c->noise;
    int32_t timb = (int32_t)c->timbre;
    int32_t shell, wire, n, out;
    int32_t amp, amp2;
    int32_t shell_w, wire_w;
    int32_t inc1s =
        (int32_t)((145u + ((uint32_t)c->tune * 3u >> 1)) * 97391u) + ((d->pitch >> 3) * 320);
    uint32_t inc1 = deck_semi_inc(inc1s > 1 ? (uint32_t)inc1s : 1u, d->semi);
    uint32_t inc2 = inc1 + (inc1 >> 1) + (inc1 >> 4);
    uint32_t wire_sh = 7u + (uint32_t)((c->decay * 5u) >> 7) + ((128u - (uint32_t)m->sn_wire_dec) >> 5);

    deck_pitch_sweep_tick(d, c, 7u);
    d->env = deck_env_decay(d, d->env, (int32_t)d->vel << 9, c, 7u + (uint32_t)(c->decay >> 5));
    d->env2 = deck_env_decay(d, d->env2, (int32_t)d->vel << 9, c, wire_sh);
    amp = deck_amp_env(d, d->env, inv);
    amp2 = deck_amp_env(d, d->env2, inv);
    d->ph += inc1;
    d->mph += inc2;
    shell = (deck_sine(d->ph) + (deck_sine(d->mph) >> 1)) >> 1;
    n = deck_noise(&d->nph);
    d->bp1 += (n - d->bp1) >> 2;
    d->bp2 += (d->bp1 - d->bp2) >> 1;
    wire = (d->bp1 - d->bp2) + ((n * shell) >> 16);
    shell_w = (168 - (tone >> 1) - ((timb * 5) >> 2)) * (int32_t)m->sn_shell >> 6;
    wire_w = (8 + (tone * 3 >> 2) + ((timb * 3) >> 1)) * (int32_t)m->sn_wire >> 6;
    out = (int32_t)((((int64_t)shell * shell_w >> 7) * amp) >> 15);
    out += (int32_t)((((int64_t)wire * wire_w >> 7) * amp2) >> 15) +
           ((d->click >> 2) * (int32_t)m->sn_wire >> 6);
    d->click -= (d->click >> 3) + (d->click > 0);
    out = deck_fold(out, c->drive);
    if (d->env < 8 && d->env2 < 8 && d->click < 8) {
        d->env = 0;
        d->env2 = 0;
        d->active = 0u;
    }
    return deck_apply_level(out, c->level);
}

static int32_t deck_render_rim(deck_voice_t *d, const deck_voice_cfg_t *c, uint8_t inv)
{
    int32_t stick, n, out;
    int32_t amp, amp2;

    d->env = deck_env_decay(d, d->env, (int32_t)d->vel << 9, c, 8u + (uint32_t)(c->decay >> 4));
    d->env2 = deck_env_decay(d, d->env2, (int32_t)d->vel << 9, c, 9u + (uint32_t)(c->decay >> 3));
    deck_pitch_sweep_tick(d, c, 9u);
    amp = deck_amp_env(d, d->env, inv);
    amp2 = deck_amp_env(d, d->env2, inv);
    {
        int32_t rinc0 = d->shell1 + d->pitch;
        uint32_t rinc;

        if (rinc0 < 97391)
            rinc0 = 97391;
        rinc = deck_semi_inc((uint32_t)rinc0, d->semi);
        d->ph += rinc;
        d->mph += rinc * 3u + (rinc >> 1) + (rinc >> 3);
    }
    stick = (deck_sine(d->ph) + deck_sine(d->mph + ((uint32_t)c->timbre << 24))) >> 1;
    n = deck_noise(&d->nph) >> (3 - (c->timbre >> 6));
    out = ((((stick * amp) >> 15) * (32 + ((int32_t)c->timbre >> 1))) >> 7) +
          ((((n * amp2) >> 15) * deck_timb_noise(c->timbre)) >> 7) + (d->click >> 3);
    d->click -= d->click >> 2;
    if (d->env < 8 && d->click < 4) {
        d->env = 0;
        d->active = 0u;
    }
    if (c->drive > 16u)
        out = deck_fold(out, c->drive);
    return deck_apply_level(out, c->level);
}

static int32_t deck_render_hat(deck_voice_t *d, const deck_voice_cfg_t *c, int32_t open, int32_t long_decay,
                               uint8_t inv, uint8_t era)
{
    const deck_era_mix_t *m = deck_era_mix(era);
    int32_t tone = (int32_t)c->noise;
    int32_t timb = (int32_t)c->timbre;
    int32_t sum = 0, hpf;
    int32_t ring_amp;
    int32_t sq_gain = ((6000 - tone * 24) * (int32_t)m->hat_sq) >> 6;
    int32_t fm_gain = (((timb * timb) >> 6) * (int32_t)m->hat_fm) >> 7;
    int32_t n_gain = 24 + tone;
    uint32_t h;
    static const uint32_t ratio[6] = {205u, 298u, 369u, 518u, 545u, 800u};
    uint32_t base = 760000u + ((uint32_t)c->tune * 6000u);
    uint32_t dec_sh;

    if (sq_gain < 2400)
        sq_gain = 2400;

    deck_pitch_sweep_tick(d, c, long_decay ? 10u : 9u);

    dec_sh = long_decay ? (12u + (uint32_t)(c->decay >> 3)) : (7u + (uint32_t)(c->decay >> 5));
    if (!open)
        dec_sh += ((128u - (uint32_t)m->hat_close) >> 4);
    if ((open ? d->oh_ring : d->hat_env) <= DECK_AMP_ENV_FLOOR && !d->env_atk_left) {
        if (open)
            d->oh_ring = 0;
        else
            d->hat_env = 0;
        d->active = 0u;
        return 0;
    }
    ring_amp = deck_amp_env(d, open ? d->oh_ring : d->hat_env, inv);

    for (h = 0; h < 6u; h++) {
        uint32_t inc = deck_semi_inc(deck_pitch_skew_inc(ratio[h] * base, d->pitch), d->semi);
        int32_t sq, fm;

        d->hat_ph[h] += inc;
        sq = (d->hat_ph[h] & 0x80000000u) ? sq_gain : -sq_gain;
        fm = deck_sine(d->hat_ph[h] + ((uint32_t)deck_sine(d->hat_ph[(h + 1u) % 6u]) << (9 + (timb >> 4))));
        sum += (sq >> 2) + ((fm * fm_gain) >> 9);
    }
    sum += (deck_noise(&d->nph) * n_gain) >> 8;
    sum = (int32_t)(((int64_t)sum * ring_amp) >> 15);
    d->hp += (sum - d->hp) >> 1;
    hpf = sum - d->hp;
    d->bp1 += (hpf - d->bp1) >> 1;
    hpf = hpf - d->bp1;
    d->mod_env += (hpf - d->mod_env) >> 1;
    hpf = hpf - d->mod_env;
    if (open) {
        if (d->choke == 2u) {
            d->oh_ring -= (d->oh_ring >> 5) + 16;
            d->choke = 0u;
        } else {
            uint32_t osh = long_decay ? (11u + (uint32_t)(c->decay >> 4)) : (9u + (uint32_t)(c->decay >> 4));

            d->oh_ring = deck_env_decay(d, d->oh_ring, (int32_t)d->vel << 9, c, osh);
        }
        if (d->oh_ring <= DECK_AMP_ENV_FLOOR && !d->env_atk_left) {
            d->oh_ring = 0;
            d->active = 0u;
        }
    } else {
        d->hat_env = deck_env_decay(d, d->hat_env, (int32_t)d->vel << 9, c, dec_sh);
        if (d->hat_env <= DECK_AMP_ENV_FLOOR && !d->env_atk_left) {
            d->hat_env = 0;
            d->active = 0u;
        }
    }
    return deck_apply_level(deck_fold(hpf * 8, c->drive >> 1), c->level);
}

static int32_t deck_render_tom(deck_voice_t *d, const deck_voice_cfg_t *c, uint8_t inv, uint8_t era)
{
    const deck_era_mix_t *m = deck_era_mix(era);
    int32_t out, mod;
    int32_t amp;
    uint32_t inc = deck_semi_inc((uint32_t)d->shell1 + (uint32_t)d->pitch, d->semi);

    d->env = deck_env_decay(d, d->env, (int32_t)d->vel << 9, c, 9u + (uint32_t)(c->decay >> 4));
    deck_pitch_sweep_tick(d, c, 8u + (c->decay >> 5));
    amp = deck_amp_env(d, d->env, inv);
    d->ph += inc;
    d->mph += inc + (inc >> 1);
    mod = (deck_sine(d->mph) * deck_timb_depth(c->timbre) * (int32_t)m->tom_fm) >> 11;
    out = deck_sine(d->ph + ((uint32_t)mod << 12));
    out += (deck_sine(d->ph + (uint32_t)((int32_t)c->timbre << 18)) * amp) >> 18;
    out = (int32_t)(((int64_t)out * amp) >> 15) +
          (int32_t)(((int64_t)(d->click >> 3) * amp * (int32_t)m->tom_beater) >> 21);
    d->click -= (d->click >> 3) + (d->click > 0);
    out = deck_fold(out, c->drive);
    if (d->env < 8 && d->click < 4) {
        d->env = 0;
        d->active = 0u;
    }
    return deck_apply_level(out, c->level);
}

static int32_t deck_render_clap(deck_voice_t *d, const deck_voice_cfg_t *c, uint8_t inv, uint8_t era)
{
    const deck_era_mix_t *m = deck_era_mix(era);
    int32_t tone = (int32_t)c->noise;
    int32_t timb = (int32_t)c->timbre;
    int32_t burst = 0, n, bp_n, body, out;
    int32_t amp;
    uint32_t i;
    int32_t burst_w = (96 + (timb >> 1)) * (int32_t)m->cp_burst >> 6;

    for (i = 0; i < 4u; i++) {
        if (d->clap_env[i] > 0)
            burst += deck_amp_env(d, d->clap_env[i], inv) >> 2;
        d->clap_env[i] = deck_env_decay(d, d->clap_env[i], (int32_t)d->vel << 9, c, 6u + (uint32_t)(i >> 1));
    }
    if (d->clap_idx < 4u) {
        d->mph++;
        if (d->mph > (11u * 44u)) {
            d->clap_env[d->clap_idx] = (int32_t)d->vel << 9;
            d->clap_idx++;
            d->mph = 0u;
        }
    }
    d->env = deck_env_decay(d, d->env, (int32_t)d->vel << 9, c, 9u + (uint32_t)(c->decay >> 4));
    amp = deck_amp_env(d, d->env, inv);
    n = deck_noise(&d->nph);
    d->bp1 += (n - d->bp1) >> 2;
    bp_n = n - d->bp1;
    deck_pitch_sweep_tick(d, c, 8u);
    d->ph += deck_semi_inc(deck_pitch_skew_inc((850u + (uint32_t)tone * 6u) * 97391u, d->pitch), d->semi);
    body = deck_sine(d->ph + ((uint32_t)bp_n << (9 + (timb >> 4))));
    out = (int32_t)(((int64_t)bp_n * (burst + (amp >> 1)) * burst_w) >> 22) +
          (int32_t)((((int64_t)body * (64 + (timb >> 1) - (tone >> 2)) >> 7) * burst) >> 14);
    out = deck_fold(out, c->drive);
    if (d->env < 8 && burst < 8 && d->clap_idx >= 4u) {
        d->env = 0;
        d->active = 0u;
    }
    return deck_apply_level(out, c->level);
}

static int32_t deck_render_noise_burst(deck_voice_t *d, const deck_voice_cfg_t *c, uint32_t dec_base, uint8_t inv)
{
    int32_t n, out;
    int32_t amp;

    d->env = deck_env_decay(d, d->env, (int32_t)d->vel << 9, c, dec_base + (uint32_t)(c->decay >> 4));
    amp = deck_amp_env(d, d->env, inv);
    n = deck_noise(&d->nph);
    d->bp1 += (n - d->bp1) >> 2;
    out = ((d->bp1 * amp) >> 15) * ((int32_t)c->noise + deck_timb_noise(c->timbre)) >> 7;
    if (d->env < 8) {
        d->env = 0;
        d->active = 0u;
    }
    return deck_apply_level(out, c->level);
}

static int32_t deck_render_cabasa(deck_voice_t *d, const deck_voice_cfg_t *c, uint8_t inv)
{
    int32_t n, out;
    int32_t amp;

    d->env = deck_env_decay(d, d->env, (int32_t)d->vel << 9, c, 6u + (uint32_t)(c->decay >> 5));
    amp = deck_amp_env(d, d->env, inv);
    n = deck_noise(&d->nph);
    deck_pitch_sweep_tick(d, c, 7u);
    d->ph += deck_semi_inc(deck_pitch_skew_inc(1200u * 97391u, d->pitch), d->semi);
    out = ((n * deck_sine(d->ph + ((uint32_t)c->timbre << 16) + ((uint32_t)c->timbre << 20))) >> 15) * amp >> 15;
    out = (out * (int32_t)c->noise) >> 7;
    if (d->env < 8) {
        d->env = 0;
        d->active = 0u;
    }
    return deck_apply_level(out, c->level);
}

static int32_t deck_render_tamb(deck_voice_t *d, const deck_voice_cfg_t *c, uint8_t inv)
{
    int32_t n, ping, out;
    int32_t amp;

    d->env = deck_env_decay(d, d->env, (int32_t)d->vel << 9, c, 7u + (uint32_t)(c->decay >> 5));
    amp = deck_amp_env(d, d->env, inv);
    deck_pitch_sweep_tick(d, c, 7u);
    d->ph += deck_semi_inc(deck_pitch_skew_inc((2400u + ((uint32_t)c->tune << 2)) * 97391u, d->pitch), d->semi);
    ping = deck_sine(d->ph + ((uint32_t)c->timbre << 17));
    n = deck_noise(&d->nph) >> (4 - (c->timbre >> 6));
    out = ((((ping * amp) >> 15) * (48 + ((int32_t)c->timbre >> 1))) >> 7) +
          ((((n * amp) >> 15) * deck_timb_noise(c->timbre)) >> 4);
    if (d->env < 8) {
        d->env = 0;
        d->active = 0u;
    }
    return deck_apply_level(deck_fold(out, c->drive >> 1), c->level);
}

static int32_t deck_render_cowbell(deck_voice_t *d, const deck_voice_cfg_t *c, uint8_t inv)
{
    int32_t sq1, sq2, out;
    int32_t amp;
    uint32_t inc1 = deck_pitch_skew_inc((540u + ((uint32_t)c->tune << 2)) * 97391u, d->pitch);
    uint32_t inc2 = inc1 + (inc1 >> 2) + (inc1 >> 4) + ((uint32_t)c->timbre << 11) + (inc1 >> (14 - (c->timbre >> 5)));

    inc1 = deck_semi_inc(inc1, d->semi);
    inc2 = deck_semi_inc(inc2, d->semi);
    deck_pitch_sweep_tick(d, c, 8u);
    d->env = deck_env_decay(d, d->env, (int32_t)d->vel << 9, c, 8u + (uint32_t)(c->decay >> 4));
    amp = deck_amp_env(d, d->env, inv);
    d->sq_ph[0] += inc1;
    d->sq_ph[1] += inc2;
    sq1 = (d->sq_ph[0] & 0x80000000u) ? (12000 - (int32_t)c->timbre * 48) : -(12000 - (int32_t)c->timbre * 48);
    sq2 = (d->sq_ph[1] & 0x80000000u) ? (8000 + (int32_t)c->timbre * 32) : -(8000 + (int32_t)c->timbre * 32);
    out = (((sq1 * (148 - (int32_t)c->timbre) + sq2 * (20 + (int32_t)c->timbre)) >> 7) * amp) >> 16;
    out += (((deck_sine(d->sq_ph[0]) * amp) >> 16) * deck_timb_depth(c->timbre)) >> 10;
    out = deck_fold(out, c->drive);
    if (d->env < 8) {
        d->env = 0;
        d->active = 0u;
    }
    return deck_apply_level(out, c->level);
}

static int32_t deck_render_ride(deck_voice_t *d, const deck_voice_cfg_t *c, uint8_t inv)
{
    int32_t tone = (int32_t)c->noise;
    int32_t timb = (int32_t)c->timbre;
    int32_t sum = 0, body, wash, out;
    int32_t ring_amp;
    uint32_t h;
    static const uint32_t ratio[6] = {152u, 218u, 281u, 356u, 431u, 512u};
    uint32_t base = 580000u + ((uint32_t)c->tune * 4500u);

    deck_pitch_sweep_tick(d, c, 10u);

    if (d->oh_ring <= DECK_AMP_ENV_FLOOR && !d->env_atk_left) {
        d->oh_ring = 0;
        d->hat_env = 0;
        d->active = 0u;
        return 0;
    }
    ring_amp = deck_amp_env(d, d->oh_ring, inv);

    /* Ride: inharmonic FM cluster (no CH square), slower ring than crash. */
    for (h = 0; h < 6u; h++) {
        uint32_t inc = deck_semi_inc(deck_pitch_skew_inc(ratio[h] * base, d->pitch), d->semi);
        int32_t fm;

        d->hat_ph[h] += inc;
        fm = deck_sine(d->hat_ph[h] +
                       ((uint32_t)deck_sine(d->hat_ph[(h + 1u) % 6u]) << (12 + (tone >> 6) + (timb >> 5))));
        sum += fm >> (1 + (h >> 1));
    }
    sum = (int32_t)(((int64_t)sum * ring_amp) >> 15);
    d->hp += (sum - d->hp) >> 2;
    body = sum - (d->hp >> 1);
    d->bp1 += (body - d->bp1) >> 2;
    body -= d->bp1 >> 1;

    out = body;
    wash = deck_noise(&d->nph);
    d->bp2 += (wash - d->bp2) >> 2;
    out += (int32_t)(((int64_t)d->bp2 * ring_amp * deck_timb_noise(c->timbre)) >> 16);
    d->oh_ring = deck_env_decay(d, d->oh_ring, (int32_t)d->vel << 9, c, 11u + (uint32_t)(c->decay >> 2));
    if (d->oh_ring <= DECK_AMP_ENV_FLOOR && !d->env_atk_left) {
        d->oh_ring = 0;
        d->active = 0u;
    }
    return deck_apply_level(deck_fold(out, c->drive >> 2), c->level);
}

static int32_t deck_render_crash(deck_voice_t *d, const deck_voice_cfg_t *c, uint8_t inv, uint8_t era)
{
    int32_t hat = deck_render_hat(d, c, 1, 1, inv, era);
    int32_t n = deck_noise(&d->nph);

    d->bp2 += (n - d->bp2) >> 1;
    return hat + (int32_t)(((int64_t)d->bp2 * deck_amp_env(d, d->oh_ring, inv)) >> 20);
}

static int32_t deck_render_voice(deck_dsp_t *s, uint8_t voice)
{
    deck_voice_t *d = &s->v[voice];
    const deck_voice_cfg_t *c = &s->cfg[voice];
    uint8_t inv = s->env_invert[voice];
    uint8_t era = s->era;
    int32_t sig;

    if (!d->active)
        return 0;
    if (!deck_voice_env_dly_tick(d, c))
        return 0;
    deck_env_attack_tick(d, c);
    switch (voice) {
    case DECK_BD:
        sig = deck_render_kick(d, c, inv, era);
        break;
    case DECK_SN:
        sig = deck_render_snare(d, c, inv, era);
        break;
    case DECK_RM:
        sig = deck_render_rim(d, c, inv);
        break;
    case DECK_CH:
        sig = deck_render_hat(d, c, 0, 0, inv, era);
        break;
    case DECK_OH:
        sig = deck_render_hat(d, c, 1, 0, inv, era);
        break;
    case DECK_CP:
        sig = deck_render_clap(d, c, inv, era);
        break;
    case DECK_CB:
        sig = deck_render_cabasa(d, c, inv);
        break;
    case DECK_TB:
        sig = deck_render_tamb(d, c, inv);
        break;
    case DECK_CW:
        sig = deck_render_cowbell(d, c, inv);
        break;
    case DECK_TH:
    case DECK_TM:
    case DECK_TL:
    case DECK_CNH:
    case DECK_CNL:
        sig = deck_render_tom(d, c, inv, era);
        break;
    case DECK_RD:
        sig = deck_render_ride(d, c, inv);
        break;
    case DECK_CR:
        sig = deck_render_crash(d, c, inv, era);
        break;
    default:
        sig = 0;
        break;
    }
    sig = deck_filter_voice(d, sig, c->filter);
    sig = deck_crush_sample(d, sig, c->crush);
    return sig;
}

static uint16_t deck_perf_mute_mask(const deck_perf_t *p)
{
    switch (p->mute_grp) {
    case 1u:
        return (uint16_t)((1u << DECK_CH) | (1u << DECK_OH));
    case 2u:
        return (uint16_t)((1u << DECK_TH) | (1u << DECK_TM) | (1u << DECK_TL));
    case 3u:
        return (uint16_t)((1u << DECK_CB) | (1u << DECK_TB) | (1u << DECK_CW) | (1u << DECK_RM));
    case 4u:
        return (uint16_t)((1u << DECK_RD) | (1u << DECK_CR));
    default:
        return 0u;
    }
}

static uint32_t deck_dly_samples(uint8_t dly_time, uint8_t bpm)
{
    uint32_t b = bpm < 40u ? 72u : (bpm > 180u ? 180u : (uint32_t)bpm);
    uint32_t d = fm_time_samples(dly_time, b, DECK_FS);
    uint8_t rdiv = 1u;

    if (d == 0u)
        d = (60u * DECK_FS) / b;
    while (d > (DECK_DLY_LEN - 128u) && rdiv < 16u) {
        d >>= 1;
        rdiv <<= 1;
    }
    if (d < 8u)
        d = 8u;
    if (d > (DECK_DLY_LEN - 64u))
        d = DECK_DLY_LEN - 64u;
    (void)rdiv;
    return d;
}

static int32_t deck_dly_read(uint32_t wpos, uint32_t delay, int32_t wow_smp)
{
    int64_t rp = (int64_t)wpos + (int64_t)DECK_DLY_LEN - (int64_t)delay - (int64_t)wow_smp;
    int32_t ip;
    int32_t frac;

    while (rp < 0)
        rp += DECK_DLY_LEN;
    while (rp >= (int64_t)DECK_DLY_LEN)
        rp -= DECK_DLY_LEN;
    ip = (int32_t)rp;
    frac = (int32_t)((rp - ip) * 256);
    {
        int32_t i0 = ip;
        int32_t i1 = (i0 + 1) % (int32_t)DECK_DLY_LEN;
        int32_t s0 = deck_dly_pool[i0];
        int32_t s1 = deck_dly_pool[i1];

        return s0 + (((s1 - s0) * frac) >> 8);
    }
}

static int32_t deck_rev_comb_step(deck_dsp_t *s, uint8_t ix, int32_t in, int32_t damp, int32_t tail_g)
{
    uint32_t len = DECK_REV_COMB_LEN - (uint32_t)ix * 37u;
    uint32_t p;
    int32_t d;
    int32_t out;

    if (len < 64u)
        len = 64u;
    p = (uint32_t)s->rev_pos[ix] % len;
    d = deck_rev_comb[ix][p];
    s->rev_lp[ix] += ((d - s->rev_lp[ix]) * damp) >> 8;
    out = s->rev_lp[ix];
    {
        int32_t nw = in + ((out * tail_g) >> 9);

        if (nw > 32767)
            nw = 32767;
        else if (nw < -32767)
            nw = -32767;
        deck_rev_comb[ix][p] = (int16_t)nw;
    }
    s->rev_pos[ix] = (uint16_t)((p + 1u) % len);
    return out;
}

static void deck_master_iso(deck_dsp_t *s, int32_t iso, int32_t *l, int32_t *r)
{
    int32_t sum_l = *l;
    int32_t sum_r = *r;

    if (iso < -64)
        iso = -64;
    else if (iso > 63)
        iso = 63;
    if (iso < 0) {
        int32_t c = 10 + ((64 + iso) * 54) / 64;

        s->iso_lp += ((sum_l - s->iso_lp) * c) >> 6;
        s->iso_lp_r += ((sum_r - s->iso_lp_r) * c) >> 6;
        sum_l = ((s->iso_lp * 3) + sum_l) >> 2;
        sum_r = ((s->iso_lp_r * 3) + sum_r) >> 2;
    } else if (iso > 0) {
        int32_t c = 1 + (iso * 17) / 63;

        s->iso_hp += ((sum_l - s->iso_hp) * c) >> 6;
        s->iso_hp_r += ((sum_r - s->iso_hp_r) * c) >> 6;
        sum_l = sum_l - ((s->iso_hp * 3) >> 2);
        sum_r = sum_r - ((s->iso_hp_r * 3) >> 2);
    }
    *l = sum_l;
    *r = sum_r;
}

/* dly_in / rev_in: L+R sums of the per-voice sends (halved here, like the old full-mix mid). */
static void deck_bus_genre_fx(deck_dsp_t *s, int32_t *mix_l, int32_t *mix_r, int32_t dly_in, int32_t rev_in_v,
                              uint8_t fx_off)
{
    const deck_master_t *m = &s->master;
    int32_t mid;
    int32_t dly_wet_l = 0;
    int32_t dly_wet_r = 0;
    int32_t rev_wet = 0;

    if (fx_off)
        return;
    mid = dly_in >> 1;
    if (m->dly_time > 0u) {
        uint32_t delay = deck_dly_samples(m->dly_time, s->render_bpm);
        int32_t fb = ((int32_t)m->feedback * 28000) >> 7;
        int32_t wow_smp = 0;
        int32_t tap_rd;
        int32_t path;
        int32_t dlvl = (int32_t)m->dly_lvl;

        if (m->wow) {
            s->wow_ph += 0x80000u + ((uint32_t)m->wow << 12);
            wow_smp = (deck_sine(s->wow_ph) * (int32_t)m->wow) >> 12;
        }
        tap_rd = deck_dly_read(s->dly_w, delay, wow_smp);
        if (m->loop_filt < 127u) {
            int32_t cf = 2 + ((int32_t)m->loop_filt >> 2);

            s->dly_filt += (tap_rd - s->dly_filt) / cf;
            tap_rd = s->dly_filt;
        }
        path = mid + ((tap_rd * fb) >> 15);
        if (path > 65000)
            path = 65000;
        else if (path < -65000)
            path = -65000;
        path = deck_softclip(path, 32u + (uint32_t)m->feedback / 4u);
        deck_dly_pool[s->dly_w] = (int16_t)path;
        s->dly_w = (s->dly_w + 1u) % DECK_DLY_LEN;
        {
            /* RE-201 multi-head modes (parity with dub_dsp / dub_draw_space TAP_*) */
            uint32_t dly_s = delay;
            uint32_t dly_b = dly_s;
            uint8_t head_mode = (uint8_t)(m->tap % TAP_N);
            int32_t tap_a = tap_rd;
            int32_t tap_b;

            switch (head_mode) {
            case TAP_DUAL:
                dly_b = dly_s >> 1;
                break;
            case TAP_GALLOP:
                dly_b = (dly_s * 3u) >> 2;
                break;
            case TAP_TRIP:
                dly_b = (dly_s * 2u) / 3u;
                break;
            case TAP_SPREAD: {
                uint32_t spread_ofs = dly_s >> 5;

                if (spread_ofs < 8u)
                    spread_ofs = 8u;
                dly_b = dly_s + spread_ofs;
                break;
            }
            case TAP_PING:
                dly_b = dly_s >> 1;
                break;
            default:
                break;
            }
            if (dly_b < 8u)
                dly_b = 8u;
            if (dly_b > (DECK_DLY_LEN - 64u))
                dly_b = DECK_DLY_LEN - 64u;
            tap_b = deck_dly_read(s->dly_w, dly_b, -wow_smp);
            switch (head_mode) {
            case TAP_SINGLE:
                dly_wet_l = (tap_a * dlvl) >> 7;
                dly_wet_r = dly_wet_l;
                break;
            case TAP_SPREAD:
                dly_wet_l = (tap_a * dlvl) >> 7;
                dly_wet_r = (tap_b * dlvl) >> 7;
                break;
            case TAP_PING:
                dly_wet_l = (tap_b * dlvl) >> 7;
                dly_wet_r = (tap_a * dlvl) >> 7;
                break;
            default:
                dly_wet_l = ((tap_a + (tap_b >> 1)) * dlvl) >> 7;
                dly_wet_r = (((tap_a >> 1) + tap_b) * dlvl) >> 7;
                break;
            }
        }
    }
    if (m->rev_mix > 0u) {
        int32_t damp = 248 - ((int32_t)m->rev_damp * 180) / 127;
        int32_t tail_g = 110 + ((int32_t)m->rev_tail * 130) / 127;
        int32_t rev_in = (rev_in_v >> 1) + ((((dly_wet_l + dly_wet_r) >> 2) * (int32_t)m->dly_rev) >> 7);
        int32_t t0;
        int32_t t1;
        int32_t t2;
        int32_t t3;
        int32_t rev_mix = (int32_t)m->rev_mix;

        if (m->tank < 64u)
            rev_in >>= 1;
        t0 = deck_rev_comb_step(s, 0u, rev_in, damp, tail_g);
        t1 = deck_rev_comb_step(s, 1u, rev_in + ((t0 * tail_g) >> 8), damp, tail_g);
        t2 = deck_rev_comb_step(s, 2u, rev_in + ((t1 * tail_g) >> 8), damp, tail_g);
        t3 = deck_rev_comb_step(s, 3u, rev_in + ((t2 * tail_g) >> 8), damp, tail_g);
        rev_wet = (((t0 + t1 + t2 + t3) >> 2) * rev_mix) >> 7;
    }
    *mix_l = *mix_l + dly_wet_l + rev_wet;
    *mix_r = *mix_r + dly_wet_r + rev_wet;
    deck_master_iso(s, (int32_t)m->isolator - 64, mix_l, mix_r);
}

static void deck_dsp_render_block(deck_dsp_t *s, int32_t *out, uint32_t n, uint16_t mute_mask, uint16_t solo_mask,
                                  uint8_t fx_off, uint8_t seq_playing)
{
    uint32_t i, v;
    uint16_t play_mask = 0xFFFFu;
    int solo_any = 0;
    deck_voice_cfg_t hold[DECK_NVOICES];
    deck_voice_cfg_t *cfg = s->cfg;
    int32_t dly_w[DECK_NVOICES]; /* 0..128; 127 → 128 keeps the legacy full-mix feed bit-exact */
    int32_t rev_w[DECK_NVOICES];
    int32_t duck_w[DECK_NVOICES];
    int32_t duck_rel;
    int mf_mode;
    int32_t mf_idx = 0;

    if (!s || !out || n == 0u)
        return;
    deck_dsp_halt_metallic_idle(s, seq_playing);
    for (v = 0; v < DECK_NVOICES; v++) {
        if (solo_mask & (1u << v)) {
            solo_any = 1;
            break;
        }
    }
    if (solo_any)
        play_mask = solo_mask;
    play_mask &= (uint16_t)~mute_mask;

    for (v = 0; v < DECK_NVOICES; v++) {
        hold[v] = s->cfg[v];
        s->cfg[v].tune = deck_u8_off(hold[v].tune, s->perf.tune);
        s->cfg[v].decay = deck_u8_off(hold[v].decay, s->perf.decay);
        s->cfg[v].filter = deck_u8_off(hold[v].filter, s->perf.filter);
        s->cfg[v].crush = deck_u8_off(hold[v].crush, s->perf.crush);
    }

    for (v = 0; v < DECK_NVOICES; v++) {
        dly_w[v] = (int32_t)s->cfg[v].dly_send + ((int32_t)s->cfg[v].dly_send >> 6);
        rev_w[v] = (int32_t)s->cfg[v].rev_send + ((int32_t)s->cfg[v].rev_send >> 6);
        duck_w[v] = v == DECK_BD || !(play_mask & (1u << DECK_BD)) ?
                        0 :
                        (int32_t)s->cfg[v].duck + ((int32_t)s->cfg[v].duck >> 6);
    }
    /* Release τ ≈ 1/32 note: ~60 ms @ 120 BPM, ducked gain back near unity after ~150 ms. */
    duck_rel = ((int32_t)(s->render_bpm ? s->render_bpm : 120u) * 13) >> 6;
    mf_mode = deck_mf_target(s->master.filter, &mf_idx);
    for (i = 0; i < n; i++) {
        int32_t mix_l = 0, mix_r = 0;
        int32_t dly_in = 0, rev_in = 0;
        int32_t mix, l, r, ax, side;

        if (s->duck_atk) {
            s->duck_env += (s->duck_peak - s->duck_env) >> 5;
            if (s->duck_env >= s->duck_peak - 64)
                s->duck_atk = 0u;
        } else if (s->duck_env > 0) {
            s->duck_env -= (s->duck_env * duck_rel >> 16) + 1;
            if (s->duck_env < 0)
                s->duck_env = 0;
        }
        for (v = 0; v < DECK_NVOICES; v++) {
            int32_t sig, pan, pl, pr, vl, vr;

            if (!(play_mask & (1u << v)))
                continue;
            deck_voice_gate_tick(&s->v[v]);
            sig = deck_render_voice(s, (uint8_t)v);
            if (duck_w[v])
                sig = (int32_t)(((int64_t)sig * (32768 - ((duck_w[v] * s->duck_env) >> 7))) >> 15);
            pan = (int32_t)s->cfg[v].pan;
            pr = (pan * pan) >> 7;
            pl = ((127 - pan) * (127 - pan)) >> 7;
            vl = (sig * pl) >> 6;
            vr = (sig * pr) >> 6;
            mix_l += vl;
            mix_r += vr;
            dly_in += ((vl + vr) * dly_w[v]) >> 7;
            rev_in += ((vl + vr) * rev_w[v]) >> 7;
        }
        deck_bus_genre_fx(s, &mix_l, &mix_r, dly_in, rev_in, fx_off);
        mix = (mix_l + mix_r) >> 1;
        mix = deck_softclip(mix, 16u + (uint32_t)s->master.drive);
        if (s->master.crush) {
            if (s->crush_cnt == 0u) {
                s->crush_hold = mix;
                s->crush_cnt = (uint8_t)(1u + (s->master.crush >> 4));
            } else
                s->crush_cnt--;
            mix = s->crush_hold;
        }
        side = mix_l - mix_r;
        deck_mf_tick(s, mf_mode, mf_idx, &mix, &side);
        if (s->master.comp) {
            ax = mix < 0 ? -mix : mix;
            if (ax > s->m_comp)
                s->m_comp += (ax - s->m_comp) >> 2;
            else
                s->m_comp -= s->m_comp >> 6;
            if (s->m_comp > 8000)
                mix = (mix * (32767 - ((s->m_comp - 8000) * (int32_t)s->master.comp >> 7))) >> 15;
        }
        if (s->master.dust) {
            s->dust_ph = s->dust_ph * 1664525u + 1013904223u;
            mix += ((int32_t)(int16_t)(s->dust_ph >> 16) * (int32_t)s->master.dust) >> 14;
        }
        if (s->master.tape)
            mix = deck_softclip(mix, (uint32_t)s->master.tape);
        {
            int32_t w = (int32_t)s->master.width;
            int32_t mid = mix;
            int32_t sw = (side * w) >> 7;

            l = mid + sw;
            r = mid - sw;
        }
        if (l > 32767)
            l = 32767;
        if (l < -32767)
            l = -32767;
        if (r > 32767)
            r = 32767;
        if (r < -32767)
            r = -32767;
        out[2u * i] = l;
        out[2u * i + 1u] = r;
    }
    for (v = 0; v < DECK_NVOICES; v++)
        s->cfg[v] = hold[v];
    (void)cfg;
}

#endif /* DECK_DSP_H */
