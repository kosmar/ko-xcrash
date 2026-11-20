#ifndef RYTM_TYPES_H
#define RYTM_TYPES_H

#include <stdint.h>

#define RYTM_NGENRES    9u
#define RYTM_NDRUMMERS  8u
#define RYTM_NBASSISTS  8u
#define RYTM_NDVOICES   7u
#define RYTM_NSCALES    9u
#define RYTM_PAGE_STEPS 16u
#define RYTM_MAX_PAGES  8u
#define RYTM_NSTEPS     (RYTM_PAGE_STEPS * RYTM_MAX_PAGES)
#define RYTM_MASK_BYTES (RYTM_NSTEPS / 8u)
#define RYTM_NSTEPS_S16 16u
#define RYTM_PHRASE_BARS 8u

/* 7 Drum Voices */
enum {
    DV_KICK  = 0,
    DV_SNARE = 1,
    DV_CH    = 2,
    DV_OH    = 3,
    DV_LTOM  = 4,
    DV_HTOM  = 5,
    DV_CLAP  = 6
};

/* Ornament kinds (Grooves ornament engine) */
enum {
    ORN_NONE    = 0,
    ORN_FLAM    = 1,
    ORN_DRAG    = 2,
    ORN_TRIPLET = 3,
    ORN_ROLL    = 4
};

/* Band performance gestures */
enum {
    GEST_NONE      = 0,
    GEST_FILL      = 1,
    GEST_BREAK     = 2,
    GEST_SOLO_DRUM = 3,
    GEST_SOLO_BASS = 4,
    GEST_SOLO_BAND = 5,
    GEST_FLAM_BURST = 6
};

/* Bass synth models */
enum {
    BMODEL_ACID303   = 0,
    BMODEL_SOLID_FM  = 1,
    BMODEL_SLAP_WG   = 2,
    BMODEL_DUB_REESE = 3,
    BMODEL_SUB808    = 4,
    BMODEL_PWM       = 5,
    BMODEL_FRETLESS  = 6,
    BMODEL_GRIT      = 7,
    BMODEL_COUNT     = 8
};

/* Per-drum-voice synthesis parameters (0..127) */
typedef struct {
    uint8_t tune;
    uint8_t decay;
    uint8_t tone;
    uint8_t drive;
    uint8_t level;
    uint8_t send_dly;
    uint8_t send_rev;
    uint8_t pan;
    uint8_t mute;
} rytm_dvoice_cfg_t;

/* Bass synthesis parameters (0..127) */
typedef struct {
    uint8_t model;     /* 0..7 (BMODEL_*) */
    uint8_t cutoff;    /* 0..127 */
    uint8_t reso;      /* 0..127 */
    uint8_t env_mod;   /* 0..127 */
    uint8_t decay;     /* 0..127 */
    uint8_t drive;     /* 0..127 */
    uint8_t glide;     /* 0..127 */
    uint8_t sub_lvl;   /* 0..127 */
} rytm_bass_cfg_t;

/* Master & Dub FX parameters (0..127) */
typedef struct {
    uint8_t dly_time;  /* 0..127 (tempo-synced or ms) */
    uint8_t dly_fb;    /* 0..127 */
    uint8_t dly_mix;   /* 0..127 */
    uint8_t rev_mix;   /* 0..127 */
    uint8_t pump;      /* 0..127 (Kick->Bass sidechain & master glue) */
    uint8_t db_bal;    /* 0..127 (64 = even Drum/Bass mix) */
    uint8_t master_drive; /* 0..127 */
    uint8_t harmonic_toms; /* 0/1: tune LT/HT to Root & 5th */
} rytm_fx_cfg_t;

/* Step resolution output for 1 drum voice on a 16th step */
typedef struct {
    uint8_t vel;           /* 0 = off, 1..127 */
    uint8_t accent;        /* 0 or 1 */
    uint8_t orn_kind;      /* ORN_NONE .. ORN_ROLL */
    uint8_t orn_vel;       /* velocity of grace/sub hits (1..127) */
    int8_t  micro_ms;      /* micro-timing offset in ms (-20..+35) */
} rytm_drum_hit_t;

/* Step resolution output for bass voice on a 16th step */
typedef struct {
    uint8_t active;        /* 0 = rest, 1 = note */
    uint8_t midi_note;     /* 24..84 */
    uint8_t vel;           /* 1..127 */
    uint8_t accent;        /* 0 or 1 */
    uint8_t legato;        /* 0 = retrigger, 1 = slide/legato */
    uint8_t dead_ghost;    /* 0 = normal, 1 = muted percussive ghost */
    uint8_t gate_pct;      /* 10..100 */
    uint8_t filter_mod;    /* 0..127 dynamic cutoff target from genre/persona/gesture */
    int8_t  micro_ms;      /* micro-timing offset in ms */
} rytm_bass_hit_t;

/* Combined step events for a 16th step */
typedef struct {
    rytm_drum_hit_t drums[RYTM_NDVOICES];
    rytm_bass_hit_t bass;
    uint8_t swing_delay_ms; /* swing offset for odd steps */
} rytm_step_events_t;

/* Pre-128-step layout, only read when migrating old settings */
typedef struct {
    uint16_t drum_force_on[RYTM_NDVOICES];
    uint16_t drum_force_off[RYTM_NDVOICES];
    uint16_t drum_accent[RYTM_NDVOICES];
    uint16_t drum_flam[RYTM_NDVOICES];
    uint16_t bass_force_on;
    uint16_t bass_force_off;
    uint8_t  bass_note[RYTM_NSTEPS_S16];
    uint8_t  bass_acc[RYTM_NSTEPS_S16];
    uint8_t  vary;
    uint8_t  vary_lane[8];
    uint8_t  freeze;
    uint8_t  step_vel[8][RYTM_NSTEPS_S16];
    uint8_t  step_prob[8][RYTM_NSTEPS_S16];
    int8_t   step_micro[8][RYTM_NSTEPS_S16];
    int8_t   step_tune[8][RYTM_NSTEPS_S16];
    uint8_t  bass_gate[RYTM_NSTEPS_S16];
    int8_t   lane_vel[8];
    int8_t   lane_trig[8];
    int8_t   lane_gate[8];
    int8_t   lane_tune[8];
} rytm_seq_ovr_s16_t;

/* User step overrides & overdubs for SEQ / REC modes */
typedef struct {
    uint8_t  drum_force_on[RYTM_NDVOICES][RYTM_MASK_BYTES];  /* bit s = force hit on step s */
    uint8_t  drum_force_off[RYTM_NDVOICES][RYTM_MASK_BYTES]; /* bit s = mute step s */
    uint8_t  drum_accent[RYTM_NDVOICES][RYTM_MASK_BYTES];    /* bit s = force accent */
    uint8_t  drum_flam[RYTM_NDVOICES][RYTM_MASK_BYTES];      /* bit s = force flam/ornament */
    uint8_t  bass_force_on[RYTM_MASK_BYTES];                 /* bit s = user bass note on step s */
    uint8_t  bass_force_off[RYTM_MASK_BYTES];                /* bit s = mute bass on step s */
    uint8_t  pages;                         /* pattern length in 16-step pages, 1..8 */
    uint8_t  bass_note[RYTM_NSTEPS];        /* user recorded midi note per step */
    uint8_t  bass_acc[RYTM_NSTEPS];         /* bit0=acc, bit1=slide, bits 2..6 = gate 0..31 */
    uint8_t  vary;                          /* legacy global vary */
    uint8_t  vary_lane[8];                  /* per-lane 0 LOCK .. 4 MAX */
    uint8_t  freeze;                        /* 1 = hold the bar seed */
    uint8_t  step_vel[8][RYTM_NSTEPS];      /* 0 = generator, else 1..127 */
    uint8_t  step_prob[8][RYTM_NSTEPS];     /* 0 = always, else 1..127 */
    int8_t   step_micro[8][RYTM_NSTEPS];    /* -25..+25 ms, 0 = none */
    int8_t   step_tune[8][RYTM_NSTEPS];     /* drums: semitone offset; bass unused */
    uint8_t  bass_gate[RYTM_NSTEPS];        /* 0 = default, else 10..100 */
    int8_t   lane_vel[8];                   /* OXI-style bipolar velocity offset -64..+63 */
    int8_t   lane_trig[8];                  /* OXI-style bipolar trig probability/density offset -64..+63 */
    int8_t   lane_gate[8];                  /* OXI-style bipolar gate/decay offset -64..+63 */
    int8_t   lane_tune[8];                  /* OXI-style bipolar pitch/tune offset -24..+24 */
} rytm_seq_ovr_t;

/* Query inputs for rytm_resolve_step */
typedef struct {
    uint8_t  step;        /* 0..15 */
    uint8_t  bar;         /* 0..7 (8-bar phrase) */
    uint8_t  ovr_step;    /* 0..127, absolute pattern step for seq overrides */
    uint8_t  genre;       /* 0..127 continuous genre fader (mapped to 0..4095) */
    uint8_t  density;     /* 0..127 (mapped to 0..4095) */
    uint8_t  feel;        /* 0..127 syncopation / feel (mapped to 0..4095) */
    uint8_t  flam_amt;    /* 0..127 ornament / flam intensity */
    uint8_t  drummer;     /* 0..7 (Elvin..Dilla) */
    uint8_t  bassist;     /* 0..7 (Mingus..Claypool) */
    uint8_t  root_note;   /* 0..11 (C..B, added to base octave 36 = C2) */
    uint8_t  scale_id;    /* 0..8 */
    uint8_t  prog_depth;  /* 0..127 (how much GENRE_PROG_8 harmonic walk applies) */
    uint8_t  swing_max;   /* 0..127 (mapped to 0..100%) */
    uint8_t  bpm;         /* 60..200 */
    uint8_t  gesture;     /* GEST_NONE .. GEST_FLAM_BURST */
    uint8_t  solo_bar;    /* 0..7 counter during solo */
    uint8_t  human;       /* 0..127 drummer/bassist humanize & drift scale */
    uint8_t  bass_density;/* 0..127 separate bass density */
    uint8_t  bass_feel;   /* 0..127 separate bass feel */
    uint8_t  bass_flam;   /* 0..127 separate bass slide/ghost density */
    uint8_t  last_bass_note; /* previous sounding bass note for legato detection */
    uint32_t rng_seed;    /* deterministic or advancing seed */
} rytm_query_t;

#endif /* RYTM_TYPES_H */
