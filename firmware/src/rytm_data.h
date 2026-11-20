/* Auto-generated tables + Grooves/Bassment generative core. */
#ifndef RYTM_DATA_H
#define RYTM_DATA_H

#include "fm_seq.h"
#include "rytm_types.h"

/* --- String tables --- */
static const char *const RYTM_GENRE_NAMES[RYTM_NGENRES] = { "DUB", "DISCO", "HOUSE", "TECHNO", "TRIP-HOP", "HIP-HOP", "JUNGLE", "UK GARAGE", "DUBSTEP" };
static const char *const RYTM_DRUMMER_NAMES[RYTM_NDRUMMERS] = { "ELVIN", "PURDIE", "CLYDE", "JAKI", "SLY", "ALLEN", "P.TONY", "DILLA" };
static const char *const RYTM_BASSIST_NAMES[RYTM_NBASSISTS] = { "MINGUS", "JAMERSON", "BOOTSY", "JACO", "ROBBIE", "FLABBA", "FLEA", "CLAYPOOL" };
static const char *const RYTM_DVOICE_NAMES[RYTM_NDVOICES] = { "KICK", "SNARE", "C.HAT", "O.HAT", "L.TOM", "H.TOM", "CLAP" };
static const char *const RYTM_SCALE_NAMES[RYTM_NSCALES] = { "MINOR", "DORIAN", "PHRYG", "MAJOR", "MIXO", "PENT.M", "DUB", "BLUES", "HARM.M" };
static const char *const RYTM_BMODEL_NAMES[BMODEL_COUNT] = {
    "303 ACID", "SOLID FM", "SLAP WG", "DUB REESE", "SUB 808", "PWM FUNK", "FRETLESS", "GRIT PICK"
};
static const uint8_t RYTM_BASSIST_DEFAULT_MODEL[RYTM_NBASSISTS] = {
    BMODEL_PWM, BMODEL_SLAP_WG, BMODEL_ACID303, BMODEL_SOLID_FM,
    BMODEL_DUB_REESE, BMODEL_SUB808, BMODEL_SLAP_WG, BMODEL_GRIT
};

typedef struct {
    uint16_t kick, snare, hats, kick_fill, snare_fill, hats_fill;
    uint8_t kick_base, kick_accent, snare_base, snare_accent, hats_base, hats_accent;
    uint16_t kick_acc_mask, snare_acc_mask, hats_acc_mask;
    int8_t timing[16];
    int8_t push_ms[3];
    uint8_t tightness;
    uint16_t oh, ltom, htom, clap;
} rytm_drumpat_t;

typedef struct { uint16_t kick, snare, hats; } rytm_fill3_t;
typedef struct {
    uint16_t kick, snare, hats, kick_acc, snare_acc, hats_acc;
} rytm_solo_t;

typedef struct {
    uint8_t shuffle, ghost_pct, backbeat_lock, dispersal;
    int8_t clave_rot, push_pull;
    uint8_t roll_pct, limb_drift, machine;
} rytm_drummer_t;

typedef struct {
    uint16_t hits, hits_fill, accent_mask;
    uint8_t base_vel, accent_vel;
    int8_t timing[16], degree[16], oct_off[16];
    uint8_t gate_w[16];
} rytm_basspat_t;

typedef struct {
    int8_t oct_span;
    uint8_t approach_pct, ghost_pct, staccato;
    uint16_t syncop_bias;
    uint8_t accent_boost;
    uint8_t pocket;
} rytm_bassist_t;

typedef struct { uint8_t dbl, flam, trip, roll; } rytm_orn_w_t;
typedef struct { rytm_orn_w_t kick, snare, hats; } rytm_orn_weights_t;

static const rytm_drumpat_t RYTM_DRUM_PATS[RYTM_NGENRES] = {
    {
        0b0000000100000000,
        0b0000000100000000,
        0b0001000000010000,
        0b0000010000000000,
        0b0000000010000000,
        0b0010001000100010,
        55,
        100,
        50,
        95,
        40,
        70,
        0b0000000000000001,
        0b0001000000010000,
        0b0100000001000000,
        { 0, 2, 0, 3, 2, 3, 0, 4, 0, 2, 0, 3, 2, 3, 0, 4 },
        { 0, 14, 4 },
        100,
        0b0000000001000000,
        0b0100000000000000,
        0b1000000000000000,
        0b0000000000000000,
    },
    {
        0b0101000101010001,
        0b0001000000010000,
        0b0000010001000100,
        0b0000010000000100,
        0b0100000001000000,
        0b1010101010101010,
        75,
        100,
        55,
        100,
        45,
        85,
        0b0001000100010001,
        0b0001000000010000,
        0b0100010001000100,
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 2, -4 },
        70,
        0b0100000000000000,
        0b0010000000000000,
        0b1000000000000000,
        0b0001000000010000,
    },
    {
        0b0001000100010001,
        0b0000000000000000,
        0b0000010001000100,
        0b0000010000000100,
        0b0100000001000000,
        0b1111111111111111,
        80,
        100,
        55,
        100,
        45,
        80,
        0b0001000100010001,
        0b0001000000010000,
        0b0100010001000100,
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 3, -3 },
        60,
        0b0100000000000000,
        0b0000000000000000,
        0b0000000000000000,
        0b0001000000010000,
    },
    {
        0b0001010100010101,
        0b0000000000000000,
        0b1111111111111111,
        0b0100010001000100,
        0b0000000100000000,
        0b1111111111111111,
        85,
        100,
        50,
        95,
        40,
        90,
        0b0001000100010001,
        0b0001000000000000,
        0b0001000100010001,
        { 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -2 },
        { -2, 0, 0 },
        35,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000000,
        0b0001000000010000,
    },
    {
        0b0000010000000001,
        0b0000000100000000,
        0b0000000001000000,
        0b0000000000010000,
        0b0000100000000000,
        0b0100010001000100,
        50,
        95,
        45,
        90,
        35,
        65,
        0b0000000000000001,
        0b0000000000010000,
        0b0100000001000000,
        { 0, 2, 4, 6, 3, 4, 5, 7, 0, 2, 4, 6, 6, 8, 5, 7 },
        { 6, 18, 8 },
        150,
        0b0100000000000000,
        0b0000100000001000,
        0b0000000000000000,
        0b0000000000000000,
    },
    {
        0b0100000100100001,
        0b0001000000010000,
        0b0101010101010101,
        0b0100000001000000,
        0b0000010000000100,
        0b1111111111111111,
        50,
        100,
        55,
        98,
        40,
        75,
        0b0000000100000001,
        0b0001000000010000,
        0b0001000100010001,
        { 0, 3, 2, 5, 0, 3, 2, 5, 0, 3, 2, 5, 2, 5, 4, 7 },
        { 2, 12, -2 },
        130,
        0b0000010000000000,
        0b0000000000000000,
        0b0000000010000000,
        0b0001000000010000,
    },
    {
        0b0000010000000101,
        0b0100100100010000,
        0b0101010101010101,
        0b0010000001001000,
        0b0100001001000010,
        0b1111111111111111,
        55,
        100,
        50,
        100,
        40,
        88,
        0b0000000100000001,
        0b0001000000010000,
        0b1010000010100000,
        { 0, 4, -3, 2, 2, 4, 0, 5, 0, 4, -3, 2, 2, 4, 0, 6 },
        { 0, 6, -5 },
        110,
        0b0000000001000000,
        0b0000001000000000,
        0b1010000000000000,
        0b0000000000000000,
    },
    {
        0b0000010000000001,
        0b0001000000010000,
        0b0100100100100100,
        0b0010000001000000,
        0b0000001000000100,
        0b1110110111101101,
        50,
        100,
        55,
        98,
        40,
        85,
        0b0000000100000001,
        0b0001000000010000,
        0b0100010001000100,
        { 0, 5, -4, 3, 0, 5, 2, 6, 0, 5, -4, 3, 0, 5, 2, 6 },
        { 0, 8, -6 },
        90,
        0b0100000001000000,
        0b0000000000000000,
        0b1000000000000000,
        0b0001000000010000,
    },
    {
        0b0000000000000001,
        0b0000000100000000,
        0b0001000000010000,
        0b0000000001000000,
        0b0000100000000000,
        0b0101010001010100,
        60,
        100,
        55,
        100,
        40,
        75,
        0b0000000000000001,
        0b0000000100000000,
        0b0100000000000100,
        { 0, 2, 0, 3, 2, 3, 0, 4, 0, 2, 0, 3, 2, 3, 0, 4 },
        { 0, 10, 2 },
        80,
        0b0000000001000000,
        0b1100000000000000,
        0b0010000000000000,
        0b0000000100000000,
    },
};

static const rytm_fill3_t RYTM_FILLS[RYTM_NGENRES][3] = {
    {
        {
            0b0000010000000001,
            0b1001000000010000,
            0b0100000001000100,
        },
        {
            0b1000000100000001,
            0b0011000000010000,
            0b0100000001000000,
        },
        {
            0b0001000000000001,
            0b1100000000010000,
            0b1010010001000100,
        },
    },
    {
        {
            0b0001000100010001,
            0b1111000000010000,
            0b1111010101010101,
        },
        {
            0b0101000100010001,
            0b1011000000010000,
            0b1111010001000100,
        },
        {
            0b0001000100010001,
            0b1111000000010000,
            0b1000000000000000,
        },
    },
    {
        {
            0b0101000100010001,
            0b1011000000010000,
            0b1111010101010101,
        },
        {
            0b0001000100010001,
            0b1111000000010000,
            0b1111010001000100,
        },
        {
            0b1001000100010001,
            0b0110000000010000,
            0b1010010001000100,
        },
    },
    {
        {
            0b1111000100010001,
            0b0000000000000000,
            0b1111111111111111,
        },
        {
            0b0101000100010001,
            0b1010000000000000,
            0b1111010101010101,
        },
        {
            0b0001000100010001,
            0b1110000000000000,
            0b1111111111111111,
        },
    },
    {
        {
            0b0000010000000001,
            0b1011000000010000,
            0b0100000001000000,
        },
        {
            0b0000000100000001,
            0b1101000000010000,
            0b1010010001000100,
        },
        {
            0b0101010000000001,
            0b1000000000010000,
            0b0010000001000000,
        },
    },
    {
        {
            0b0001000100001001,
            0b1111000000010000,
            0b1111010101010101,
        },
        {
            0b0100010000001001,
            0b1011000000010000,
            0b1010010001000100,
        },
        {
            0b0000000100000001,
            0b1111000000010000,
            0b0101000100010001,
        },
    },
    {
        {
            0b0010100100001001,
            0b1101010000010000,
            0b1111111111111111,
        },
        {
            0b0100010001000001,
            0b1011000100010000,
            0b1111111111111111,
        },
        {
            0b0001000000000001,
            0b1111010000010000,
            0b1010010001000100,
        },
    },
    {
        {
            0b0101010001000001,
            0b1010000000010000,
            0b1111111111111111,
        },
        {
            0b0011000001000001,
            0b1100000000010000,
            0b1100110101001101,
        },
        {
            0b0110000001000001,
            0b1001000000010000,
            0b1111111111111111,
        },
    },
    {
        {
            0b0010000000000001,
            0b1000000100000000,
            0b0101000001000100,
        },
        {
            0b0001000000000001,
            0b1100000100000000,
            0b1010000001000000,
        },
        {
            0b0100000000000001,
            0b1010000100000000,
            0b0001000000000000,
        },
    },
};

static const rytm_fill3_t RYTM_BREAKS[RYTM_NGENRES][3] = {
    {
        {
            0b0000000000000000,
            0b1000000000000000,
            0b0000000000000000,
        },
        {
            0b0000000000000000,
            0b1100000000000000,
            0b0000000000000000,
        },
        {
            0b0001000000000000,
            0b1000000000000000,
            0b0000000000000000,
        },
    },
    {
        {
            0b0001000100010001,
            0b1000000000000000,
            0b0000000000000000,
        },
        {
            0b0000000000000000,
            0b1000000000000000,
            0b0100010001000100,
        },
        {
            0b0000000000000000,
            0b1100000000000000,
            0b0000000000000000,
        },
    },
    {
        {
            0b0000000000000000,
            0b1000000000000000,
            0b0100010001000100,
        },
        {
            0b0001000100010001,
            0b1000000000000000,
            0b0000000000000000,
        },
        {
            0b0000000000000000,
            0b1000000000000000,
            0b0000000000000000,
        },
    },
    {
        {
            0b0001000100010001,
            0b1000000000000000,
            0b0000000000000000,
        },
        {
            0b0000000100000001,
            0b0000000000000000,
            0b1100000000000000,
        },
        {
            0b0000000000000000,
            0b1000000000000000,
            0b0101010101010101,
        },
    },
    {
        {
            0b0000000000000000,
            0b1100000000000000,
            0b0000000000000000,
        },
        {
            0b0000000000000000,
            0b1000000000000000,
            0b0000000000000000,
        },
        {
            0b0000000100000000,
            0b1000000000000000,
            0b0000000000000000,
        },
    },
    {
        {
            0b0000000000000000,
            0b1110000000000000,
            0b0000000000000000,
        },
        {
            0b0000000100000000,
            0b1000000000000000,
            0b0000000000000000,
        },
        {
            0b0000000000000000,
            0b1100000000000000,
            0b0000000000000000,
        },
    },
    {
        {
            0b0000000000000000,
            0b1100000000000000,
            0b0000000000000000,
        },
        {
            0b0001000000000000,
            0b1000000000000000,
            0b0000000000000000,
        },
        {
            0b0000000000000000,
            0b1001000000000000,
            0b0000000000000000,
        },
    },
    {
        {
            0b0000000001000001,
            0b1000000000000000,
            0b0000000000000000,
        },
        {
            0b0000000000000000,
            0b1100000000000000,
            0b0000000000000000,
        },
        {
            0b0000000000000000,
            0b1000000000000000,
            0b0100010000000000,
        },
    },
    {
        {
            0b0000000000000000,
            0b1000000000000000,
            0b0000000000000000,
        },
        {
            0b0000000000000000,
            0b1100000000000000,
            0b0000000000000000,
        },
        {
            0b0000000000000001,
            0b1000000000000000,
            0b0000000000000000,
        },
    },
};

static const rytm_solo_t RYTM_SOLOS[RYTM_NGENRES][3] = {
    {
        {
            0b0001000110000001,
            0b0101000000010000,
            0b0100010001000100,
            0b0000000100000001,
            0b0001000000010000,
            0b0100000001000000,
        },
        {
            0b0001000110001001,
            0b0101010000010000,
            0b0101010001010100,
            0b0000000100000001,
            0b0001000000010000,
            0b0100000001000000,
        },
        {
            0b0010100101001001,
            0b1001010000010000,
            0b0101010101010100,
            0b0000000100000001,
            0b0001000000010000,
            0b0100000001000000,
        },
    },
    {
        {
            0b0001000100010001,
            0b1001000010010000,
            0b0100010001000100,
            0b0001000100010001,
            0b0001000000010000,
            0b0100000001000000,
        },
        {
            0b0101000100010001,
            0b1011000010010000,
            0b0101010101010100,
            0b0001000100010001,
            0b0001000000010000,
            0b0100000001000000,
        },
        {
            0b0101000100010001,
            0b1001100010011000,
            0b0011010100110101,
            0b0001000100010001,
            0b0001000000010000,
            0b0001000000010000,
        },
    },
    {
        {
            0b0001000100010001,
            0b0001100000010000,
            0b0100010001000100,
            0b0001000100010001,
            0b0001000000010000,
            0b0100000001000000,
        },
        {
            0b0001000110010001,
            0b1001010000010000,
            0b0101010101010100,
            0b0001000100010001,
            0b0001000000010000,
            0b0100000001000000,
        },
        {
            0b1001000110010001,
            0b0101010000011000,
            0b1101010111010101,
            0b0001000100010001,
            0b0001000000010000,
            0b0100000001000000,
        },
    },
    {
        {
            0b0001000100010001,
            0b0001000000000000,
            0b0101010101010101,
            0b0001000100010001,
            0b0001000000000000,
            0b0100000001000000,
        },
        {
            0b0001000100001111,
            0b0001000000010000,
            0b1111010101010101,
            0b0001000100000001,
            0b0001000000010000,
            0b0100000001000000,
        },
        {
            0b0000111100001111,
            0b0111000000010000,
            0b0101010101010101,
            0b0000000100000001,
            0b0001000000010000,
            0b0100000001000000,
        },
    },
    {
        {
            0b0000010000000001,
            0b0001000000010000,
            0b0100000001000000,
            0b0000000000000001,
            0b0001000000010000,
            0b0100000001000000,
        },
        {
            0b0000010000001001,
            0b0101000000010000,
            0b0100010001000100,
            0b0000000000000001,
            0b0001000000010000,
            0b0100000001000000,
        },
        {
            0b0010010000001001,
            0b0101000010010000,
            0b0100010101000100,
            0b0000000000000001,
            0b0001000000010000,
            0b0100000001000000,
        },
    },
    {
        {
            0b0000000100001001,
            0b0001000000010000,
            0b0100010001000100,
            0b0000000100000001,
            0b0001000000010000,
            0b0100000001000000,
        },
        {
            0b0000100100001001,
            0b1001000010010000,
            0b0101010101010101,
            0b0000000100000001,
            0b0001000000010000,
            0b0100000001000000,
        },
        {
            0b0100100100001001,
            0b1011000000110000,
            0b0100010001000100,
            0b0000000100000001,
            0b0001000000010000,
            0b0100000001000000,
        },
    },
    {
        {
            0b0000010000000001,
            0b1001000010010000,
            0b0100010001000100,
            0b0000010000000001,
            0b0001000000010000,
            0b0100000001000000,
        },
        {
            0b0000010000001001,
            0b1001100010010000,
            0b0101010001010100,
            0b0000010000000001,
            0b0001000000010000,
            0b0100000001000000,
        },
        {
            0b0000010100001001,
            0b1101100010110000,
            0b0101010001010101,
            0b0000010000000001,
            0b0001000000010000,
            0b0100000001000000,
        },
    },
    {
        {
            0b0000010001000001,
            0b0001000000010000,
            0b0100010001000100,
            0b0000000000000001,
            0b0001000000010000,
            0b0100000001000000,
        },
        {
            0b0100010001000001,
            0b0001100000011000,
            0b0101010101010100,
            0b0000000000000001,
            0b0001000000010000,
            0b0100000001000000,
        },
        {
            0b0100010101000001,
            0b1001100010011000,
            0b0101010101010101,
            0b0000000000000001,
            0b0001000000010000,
            0b0100000001000000,
        },
    },
    {
        {
            0b0000000001000001,
            0b0000000100000000,
            0b0100010001000100,
            0b0000000000000001,
            0b0000000100000000,
            0b0100000001000000,
        },
        {
            0b0000010001000001,
            0b1000000100000000,
            0b0101010001000100,
            0b0000000000000001,
            0b0000000100000000,
            0b0100000001000000,
        },
        {
            0b0010010001000001,
            0b1001000100000000,
            0b0101010001010100,
            0b0000000000000001,
            0b0000000100000000,
            0b0100000001000000,
        },
    },
};

static const rytm_drummer_t RYTM_DRUMMERS[RYTM_NDRUMMERS] = {
    { 85, 88, 20, 45, 0, 75, -5, 35, 0 },
    { 100, 95, 100, 10, 0, 60, -2, 15, 0 },
    { 25, 70, 90, 15, 0, 65, 2, 20, 0 },
    { 0, 5, 70, 0, 0, 0, 0, 0, 90 },
    { 15, 20, 95, 5, -2, 20, -7, 10, 20 },
    { 30, 65, 25, 80, 1, 55, -1, 30, 0 },
    { 0, 45, 60, 70, 3, 95, 0, 0, 100 },
    { 45, 60, 15, 20, 0, 45, -6, 95, 0 },
};

static const rytm_basspat_t RYTM_BASS_PATS[RYTM_NGENRES] = {
    {
        0b0000000100000001,
        0b1010101010101010,
        0b0000000000000001,
        70, 105,
        { 0, 1, 0, 2, 1, 2, 0, 2, 0, 1, 0, 2, 1, 2, 0, 2 },
        { 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 4, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 90, 50, 50, 50, 50, 50, 70, 50, 85, 50, 50, 50, 75, 50, 50, 50 },
    },
    {
        0b0001000100010001,
        0b1100010001000100,
        0b0001000000010000,
        75, 110,
        { 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2 },
        { 0, 0, 0, 4, 0, 0, 0, 5, 0, 0, 0, 4, 0, 0, 3, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 },
        { 70, 40, 70, 45, 70, 40, 70, 50, 70, 40, 70, 45, 65, 40, 60, 40 },
    },
    {
        0b0001000100010001,
        0b0100010011000100,
        0b0001000100010001,
        80, 112,
        { 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2 },
        { 0, 0, 0, 0, 6, 0, 6, 0, 5, 0, 5, 0, 6, 0, 6, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 75, 40, 70, 40, 75, 40, 70, 40, 75, 40, 70, 40, 75, 40, 70, 40 },
    },
    {
        0b0001000100010001,
        0b0100010001000100,
        0b0001000000010000,
        85, 108,
        { 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 95, 40, 90, 40, 95, 40, 90, 40, 95, 40, 90, 55, 95, 40, 90, 40 },
    },
    {
        0b0000000100000001,
        0b1001100010011000,
        0b0000000000000001,
        60, 95,
        { 0, 2, 1, 3, 2, 3, 1, 3, 0, 2, 1, 3, 3, 4, 2, 3 },
        { 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 5, 0, 4, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 },
        { 100, 50, 50, 50, 50, 50, 80, 50, 90, 50, 50, 50, 85, 50, 70, 50 },
    },
    {
        0b0100000100100001,
        0b1000011001000110,
        0b0000000100000001,
        70, 110,
        { 0, 2, 1, 3, 0, 2, 1, 3, 0, 2, 1, 3, 1, 3, 2, 4 },
        { 0, 0, 2, 0, 0, 0, 5, 0, 0, 0, 3, 0, 0, 0, 6, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 80, 40, 55, 40, 75, 40, 60, 40, 80, 40, 55, 40, 70, 40, 65, 40 },
    },
    {
        0b0100100100100101,
        0b1010010010010010,
        0b0000000100000001,
        65, 108,
        { 0, 3, -1, 2, 1, 3, 0, 4, 0, 3, -1, 2, 1, 3, 0, 4 },
        { 0, 0, 6, 0, 0, 5, 0, 2, 0, 0, 6, 0, 5, 0, 2, 0 },
        { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 },
        { 55, 35, 50, 35, 60, 40, 50, 45, 55, 35, 50, 35, 60, 40, 50, 40 },
    },
    {
        0b1000100100100001,
        0b0110010010010010,
        0b0000000100000001,
        70, 110,
        { 0, 3, -1, 2, 0, 3, 1, 4, 0, 3, -1, 2, 0, 3, 1, 4 },
        { 0, 0, 2, 0, 0, 0, 5, 0, 0, 0, 2, 0, 0, 5, 6, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 },
        { 50, 35, 60, 35, 55, 35, 65, 40, 50, 35, 60, 35, 55, 45, 50, 35 },
    },
    {
        0b0000000000000001,
        0b1000100011001000,
        0b0000000000000001,
        80, 115,
        { 0, 1, 0, 2, 1, 2, 0, 2, 0, 1, 0, 2, 1, 2, 0, 2 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 6, 0, 0 },
        { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 100, 40, 40, 40, 40, 40, 40, 40, 90, 40, 70, 40, 40, 75, 40, 40 },
    },
};

static const rytm_bassist_t RYTM_BASSISTS[RYTM_NBASSISTS] = {
    { 1, 95, 55, 68, 480, 32, 0 },
    { 0, 0, 2, 96, 0, 2, 1 },
    { 1, 40, 96, 42, 1200, 36, 0 },
    { 2, 88, 12, 100, 60, 8, 0 },
    { 0, 0, 4, 100, 0, 3, 1 },
    { 1, 22, 28, 92, 220, 16, 0 },
    { 2, 28, 72, 16, 1600, 48, 0 },
    { 2, 78, 68, 24, 1100, 44, 0 },
};

static const uint16_t RYTM_FILL_RHYTHM[RYTM_NGENRES][3] = {
    { 0b1100000000000000, 0b1010000000000000, 0b1000100000000000 },
    { 0b1110101000000000, 0b1101010000010000, 0b1111000000010000 },
    { 0b1110101010100000, 0b1101010100010000, 0b1111010001000000 },
    { 0b1111010101010000, 0b1110111000000000, 0b1111111100000000 },
    { 0b1100000000000000, 0b1010000010000000, 0b1101000000000000 },
    { 0b1110010010000000, 0b1101001001000000, 0b1111000010010000 },
    { 0b1111101001001000, 0b1101011010010000, 0b1110110100100000 },
    { 0b1110010100100000, 0b1101100001001000, 0b1111001010000000 },
    { 0b1000000000000000, 0b1100000010000000, 0b1010000000000000 },
};

static const uint16_t RYTM_SOLO_RHYTHM[RYTM_NGENRES][3] = {
    { 0b1001000010010000, 0b1010010010000100, 0b1000100100001000 },
    { 0b1010101010101010, 0b1101010111010101, 0b1010111010101110 },
    { 0b1010101010101001, 0b1101010101010100, 0b1010110110101100 },
    { 0b1010101010101010, 0b1110111011101110, 0b1101110111011101 },
    { 0b1000010010000100, 0b1010000010010000, 0b1000100001001000 },
    { 0b1010010110100101, 0b1101001011010010, 0b1010100101001010 },
    { 0b1011010110101001, 0b1101101001011010, 0b1010110110110100 },
    { 0b1010010110100101, 0b1101001011010010, 0b1001011010010110 },
    { 0b1000010010001000, 0b1010000010010000, 0b1000100001001000 },
};

static const uint8_t RYTM_GENRE_PROG_8[RYTM_NGENRES][8] = {
    { 0, 3, 0, 4, 0, 3, 4, 0 },
    { 0, 5, 3, 4, 0, 3, 4, 0 },
    { 0, 6, 5, 6, 0, 5, 6, 0 },
    { 0, 0, 0, 4, 0, 0, 4, 0 },
    { 0, 6, 5, 4, 0, 5, 4, 0 },
    { 0, 5, 2, 6, 0, 2, 5, 6 },
    { 0, 6, 5, 2, 0, 5, 2, 6 },
    { 0, 2, 5, 6, 0, 5, 2, 6 },
    { 0, 0, 5, 6, 0, 5, 6, 0 },
};

static const uint16_t RYTM_GENRE_CUTOFF[RYTM_NGENRES] = { 700, 1900, 1600, 1400, 900, 1100, 2100, 2300, 800 };

static const int8_t RYTM_SWING_BIAS[RYTM_NGENRES] = { 20, 35, 30, 8, 45, 40, 48, 50, 25 };

static const rytm_orn_weights_t RYTM_ORN_WEIGHTS[RYTM_NGENRES] = {
    { {1, 6, 1, 0}, {2, 10, 2, 1}, {1, 4, 2, 1} }, /* dub */
    { {3, 5, 2, 1}, {4, 8, 3, 2}, {2, 3, 6, 4} }, /* disco */
    { {2, 4, 1, 2}, {6, 6, 2, 3}, {1, 2, 4, 8} }, /* house */
    { {1, 2, 1, 6}, {2, 3, 2, 8}, {0, 1, 2, 12} }, /* techno */
    { {2, 7, 4, 1}, {3, 6, 6, 2}, {2, 4, 8, 3} }, /* trip */
    { {4, 8, 2, 1}, {6, 9, 3, 2}, {3, 5, 4, 3} }, /* hip */
    { {2, 4, 3, 6}, {3, 5, 4, 10}, {1, 2, 6, 12} }, /* jungle */
    { {2, 6, 3, 2}, {4, 8, 4, 3}, {2, 5, 6, 4} }, /* ukg */
    { {1, 3, 2, 4}, {2, 4, 3, 8}, {1, 2, 3, 10} }, /* dubstep */
};

static const uint8_t RYTM_SCALE_LEN[RYTM_NSCALES] = { 7, 7, 7, 7, 7, 5, 7, 6, 7 };
static const uint8_t RYTM_SCALE_SEMIS[RYTM_NSCALES][8] = {
    { 0, 2, 3, 5, 7, 8, 10, 0 },  /* minor / aeolian */
    { 0, 2, 3, 5, 7, 9, 10, 0 },  /* dorian */
    { 0, 1, 3, 5, 7, 8, 10, 0 },  /* phrygian */
    { 0, 2, 4, 5, 7, 9, 11, 0 },  /* major / ionian */
    { 0, 2, 4, 5, 7, 9, 10, 0 },  /* mixolydian */
    { 0, 3, 5, 7, 10, 0, 0, 0 },  /* pent min */
    { 0, 2, 3, 5, 7, 9, 10, 0 },  /* dub / dorian reggae */
    { 0, 3, 5, 6, 7, 10, 0, 0 },  /* blues */
    { 0, 2, 3, 5, 7, 8, 11, 0 },  /* harmonic minor */
};

/* --- PRNG / helpers (no libm) --- */
static uint32_t rytm_rng_u32(uint32_t *seed)
{
    *seed = (*seed * 1664525u) + 1013904223u;
    return *seed;
}

static uint8_t rytm_rng_pct(uint32_t *seed)
{
    return (uint8_t)(rytm_rng_u32(seed) % 100u);
}

static uint16_t rytm_u8_to4095(uint8_t v)
{
    return (uint16_t)(((uint32_t)v * 4095u) / 127u);
}

static uint8_t rytm_lerp_u8(uint8_t a, uint8_t b, uint8_t frac)
{
    return (uint8_t)((int32_t)a + (((int32_t)b - (int32_t)a) * (int32_t)frac) / 255);
}

static int32_t rytm_lerp_i32(int32_t a, int32_t b, uint8_t frac)
{
    return a + (((b - a) * (int32_t)frac) / 255);
}

static uint16_t rytm_lerp_u16(uint16_t a, uint16_t b, uint8_t frac)
{
    return (uint16_t)((int32_t)a + (((int32_t)b - (int32_t)a) * (int32_t)frac) / 255);
}

static uint16_t rytm_feel_curve(uint16_t f)
{
    uint32_t x = f;
    return (uint16_t)((x * x) / 4095u);
}

static uint16_t rytm_humanize_curve(uint16_t f)
{
    uint32_t x = f;
    return (uint16_t)((x * (x + 4095u)) / (2u * 4095u));
}

static void rytm_genre_pair(uint16_t pos4095, uint8_t *lo, uint8_t *hi, uint8_t *frac_u8)
{
    uint32_t f = pos4095;
    if (f > 4095u) {
        f = 4095u;
    }
    if (RYTM_NGENRES <= 1u) {
        *lo = *hi = 0;
        *frac_u8 = 0;
        return;
    }
    {
        uint32_t spans = (uint32_t)(RYTM_NGENRES - 1u);
        uint32_t scaled = f * spans;
        uint32_t lo_i = scaled / 4095u;
        if (lo_i >= RYTM_NGENRES - 1u) {
            *lo = (uint8_t)(RYTM_NGENRES - 1u);
            *hi = (uint8_t)(RYTM_NGENRES - 1u);
            *frac_u8 = 0;
            return;
        }
        *lo = (uint8_t)lo_i;
        *hi = (uint8_t)(lo_i + 1u);
        *frac_u8 = (uint8_t)(((scaled % 4095u) * 255u) / 4095u);
    }
}

static uint16_t rytm_feel_lerp_u16(uint16_t flat, uint16_t character, uint16_t feel)
{
    uint32_t t = rytm_feel_curve(feel);
    if (character >= flat) {
        return (uint16_t)(flat + ((character - flat) * t) / 4095u);
    }
    return (uint16_t)(flat - ((flat - character) * t) / 4095u);
}

static int32_t rytm_feel_lerp_i32(int32_t flat, int32_t character, uint16_t feel)
{
    int32_t t = (int32_t)rytm_feel_curve(feel);
    return flat + ((character - flat) * t) / 4095;
}

static int rytm_bit_set(uint16_t mask, uint8_t step)
{
    return (mask & (uint16_t)(1u << (step & 15u))) != 0;
}

/* Steps that differ between two neighbours cross one at a time, so the fader
 * walks through distinct grids instead of snapping at the midpoint. */
static int rytm_morph_bit(uint16_t lo, uint16_t hi, uint8_t step, uint8_t frac, uint8_t salt)
{
    int a = rytm_bit_set(lo, step);
    int b = rytm_bit_set(hi, step);

    return fm_seq_morph_bit(a, b, step, frac, salt);
}

static uint16_t rytm_morph_mask(uint16_t lo, uint16_t hi, uint8_t frac, uint8_t salt)
{
    return fm_seq_morph_mask(lo, hi, frac, salt);
}

static uint16_t rytm_rot16(uint16_t mask, uint32_t n)
{
    n &= 15u;
    if (n == 0u) {
        return mask;
    }
    return (uint16_t)((mask << n) | (mask >> (16 - n)));
}

static uint8_t rytm_popcnt16(uint16_t x)
{
    uint8_t c = 0;
    while (x) {
        c = (uint8_t)(c + (x & 1u));
        x = (uint16_t)(x >> 1);
    }
    return c;
}

static uint8_t rytm_step_chance(uint8_t step, uint8_t voice, uint32_t salt)
{
    uint32_t x = (uint32_t)step;
    x = x * 37u + (uint32_t)voice;
    x = x * 17u + salt;
    return (uint8_t)(x % 100u);
}

static uint8_t rytm_chance_roll(uint32_t *seed, uint8_t step, uint8_t voice, uint32_t salt, uint16_t groove_t)
{
    uint32_t hashed = rytm_step_chance(step, voice, salt);
    uint32_t live = (uint32_t)rytm_rng_pct(seed);
    uint32_t w = (groove_t * 4u) / 5u;
    return (uint8_t)((hashed * (4095u - w) + live * w) / 4095u);
}

static int8_t rytm_fold_pocket(int8_t degree)
{
    int32_t d = degree % 7;
    if (d < 0) {
        d += 7;
    }
    if (d == 0 || d == 3 || d == 4) {
        return (int8_t)d;
    }
    if (d == 1 || d == 2) {
        return 0;
    }
    return 4;
}

static uint8_t rytm_scale_note(uint8_t root, uint8_t scale_id, int32_t degree, int32_t oct_off)
{
    uint8_t sid = scale_id;
    if (sid >= RYTM_NSCALES) {
        sid = 0;
    }
    uint8_t n = RYTM_SCALE_LEN[sid];
    if (n == 0u) {
        n = 7;
    }
    int32_t deg = degree;
    while (deg < 0) {
        deg += (int32_t)n;
    }
    uint8_t deg_i = (uint8_t)(deg % (int32_t)n);
    int32_t oct_add = deg / (int32_t)n;
    int32_t semi = (int32_t)RYTM_SCALE_SEMIS[sid][deg_i];
    if (degree >= (int32_t)n) {
        semi += 12;
    }
    semi += oct_add * 12;
    semi += oct_off * 12;
    {
        int32_t note = (int32_t)root + 36 + semi;
        if (note < 24) {
            note = 24;
        }
        if (note > 84) {
            note = 84;
        }
        return (uint8_t)note;
    }
}

static void rytm_euclid_mask(uint8_t hits, uint8_t steps, uint8_t rot, uint16_t *out_mask)
{
    fm_seq_euclid_mask(hits, steps, rot, out_mask);
}

static int rytm_fill_reveal(uint16_t fill, uint16_t density, uint8_t step, uint32_t rot, uint8_t *frac_out)
{
    uint16_t bit = (uint16_t)(1u << (step & 15u));
    uint8_t total;
    uint32_t rank = 0;
    uint32_t i;
    uint32_t revealed_scaled;
    uint32_t revealed_count;
    uint8_t frac;
    if ((fill & bit) == 0u) {
        return 0;
    }
    total = rytm_popcnt16(fill);
    if (total == 0u) {
        return 0;
    }
    for (i = 0; i < (uint32_t)(step & 15u); i++) {
        if (fill & (uint16_t)(1u << i)) {
            rank++;
        }
    }
    rank = (rank + (rot % (uint32_t)total)) % (uint32_t)total;
    revealed_scaled = ((uint32_t)density * (uint32_t)total * 256u) / 4095u;
    revealed_count = revealed_scaled / 256u;
    frac = (uint8_t)(revealed_scaled % 256u);
    if (rank < revealed_count) {
        *frac_out = 255;
        return 1;
    }
    if (rank == revealed_count && frac > 0u) {
        *frac_out = frac;
        return 1;
    }
    return 0;
}

static uint16_t rytm_ghost_vel_pct(uint8_t frac, uint16_t quiet, uint16_t full)
{
    return (uint16_t)(quiet + ((full - quiet) * (uint32_t)frac) / 255u);
}

static uint32_t rytm_persona_w(uint16_t feel)
{
    return rytm_feel_curve(feel);
}

static uint32_t rytm_persona_pct(uint8_t amount, uint16_t feel)
{
    return ((uint32_t)amount * rytm_persona_w(feel)) / 4095u;
}

static uint8_t rytm_shuffled_bias(uint8_t bias, uint8_t shuffle)
{
    uint16_t head = (uint16_t)(100u - (uint16_t)bias);
    uint16_t v = (uint16_t)bias + (head * (uint16_t)shuffle) / 100u;
    if (v > 100u) {
        v = 100u;
    }
    return (uint8_t)v;
}

static uint8_t rytm_roll_rate_scale(uint8_t roll_pct, uint16_t feel)
{
    int32_t full = (int32_t)roll_pct * 2;
    int32_t v = 100 + ((full - 100) * (int32_t)rytm_persona_w(feel)) / 4095;
    if (v < 0) {
        v = 0;
    }
    if (v > 200) {
        v = 200;
    }
    return (uint8_t)v;
}

static int32_t rytm_limb_drift_ms(uint8_t bar, int32_t span, uint8_t voice)
{
    uint32_t h;
    if (span <= 0 || (voice != DV_CH && voice != DV_OH)) {
        return 0;
    }
    h = (uint32_t)bar * 2654435761u + 2654435761u;
    return (int32_t)((h >> 11) % (uint32_t)(2 * span + 1)) - span;
}

static int rytm_ghost_survives(uint8_t frac_in, const rytm_drummer_t *prof, uint16_t feel, uint32_t *seed, int locked)
{
    (void)frac_in;
    if (locked || prof->ghost_pct >= 50u) {
        return 1;
    }
    {
        uint32_t chance = rytm_persona_pct((uint8_t)((50u - prof->ghost_pct) * 2u), feel);
        if ((uint32_t)rytm_rng_pct(seed) < chance) {
            return 0;
        }
    }
    return 1;
}

static int rytm_ghost_extra(const rytm_drummer_t *prof, uint16_t feel, uint32_t *seed, uint8_t step, int occupied, uint8_t *frac_out)
{
    if (occupied || prof->ghost_pct <= 50u || (step & 1u) == 0u) {
        return 0;
    }
    {
        uint32_t chance = rytm_persona_pct((uint8_t)((prof->ghost_pct - 50u) * 2u), feel);
        if (((uint32_t)rytm_rng_pct(seed) ^ 90u) < chance) {
            *frac_out = 96;
            return 1;
        }
    }
    return 0;
}

static uint16_t rytm_core_vel_pct(uint8_t base, uint8_t accent, uint16_t acc_mask, uint8_t step, uint16_t feel, uint32_t *seed, uint8_t tightness)
{
    uint16_t character = rytm_bit_set(acc_mask, step) ? accent : base;
    uint16_t v = rytm_feel_lerp_u16(70, character, feel);
    uint32_t span = (rytm_humanize_curve(feel) * 8u * (uint32_t)tightness) / (4095u * 100u);
    if (span > 0u) {
        uint32_t off = rytm_rng_u32(seed) % (2u * span + 1u);
        {
            uint32_t vv = (uint32_t)v + off;
            v = (uint16_t)(vv > span ? vv - span : 0u);
        }
        if (v < 1u) {
            v = 1u;
        }
        if (v > 100u) {
            v = 100u;
        }
    }
    return v;
}

static uint16_t rytm_hat_offbeat_duck(uint16_t vel, uint8_t step, int accented, uint16_t feel)
{
    uint32_t cut;
    if (accented || (step & 1u) == 0u) {
        return vel;
    }
    cut = (12u * rytm_humanize_curve(feel)) / 4095u;
    if (vel > cut) {
        return (uint16_t)(vel - cut);
    }
    return 1;
}

static uint8_t rytm_pct_to_midi_vel(uint16_t pct)
{
    uint32_t v = (4095u * (uint32_t)pct) / 100u;
    if (v > 4095u) {
        v = 4095u;
    }
    return (uint8_t)((v * 127u) / 4095u);
}

static uint8_t rytm_fill_vel_pct(uint8_t base, uint8_t accent, uint8_t step)
{
    uint16_t b = base;
    uint16_t a = accent;
    if (a < b) {
        a = b;
    }
    return (uint8_t)(b + ((a - b) * (uint16_t)step) / 15u);
}

static int rytm_solo_ghost(uint32_t *seed, uint16_t feel, int prev_hit, uint8_t salt, uint8_t roll_scale)
{
    uint32_t open;
    uint32_t chance;
    if (!prev_hit) {
        return 0;
    }
    open = rytm_humanize_curve(feel);
    chance = ((4u + (open * 16u) / 4095u) * (uint32_t)roll_scale) / 100u;
    return ((uint32_t)(rytm_rng_u32(seed) ^ salt) % 100u) < chance;
}

static void rytm_derive_extra_voices(uint8_t hits[7], uint8_t vels[7], uint8_t acc[7], uint8_t step, uint8_t bar, uint8_t near, uint16_t density, int gesture, int snare_acc, int hats_acc, const rytm_drummer_t *prof, uint16_t feel, uint32_t *seed, int snare_locked)
{
    uint32_t disp = rytm_persona_pct(prof->dispersal, feel);

    /* 1. Snare dispersal to toms on non-locked snare ghosts */
    if (!gesture && hits[DV_SNARE] && !snare_locked && prof->dispersal > 0u) {
        uint8_t tom = (step & 3u) == 0u ? DV_LTOM : DV_HTOM;
        if (((uint32_t)rytm_rng_u32(seed) ^ 60u) % 100u < disp) {
            hits[tom] = 1;
            hits[DV_SNARE] = 0;
            vels[tom] = vels[DV_SNARE];
            acc[tom] = acc[DV_SNARE];
        }
    }
    /* 2. Gesture tom fills in second half of bar */
    if (gesture && hits[DV_SNARE] && step >= 8u) {
        uint8_t tom = step < 12u ? DV_HTOM : DV_LTOM;
        hits[tom] = 1;
        hits[DV_SNARE] = 0;
        vels[tom] = vels[DV_SNARE];
        acc[tom] = acc[DV_SNARE];
    }
    /* 3. Groove tom conversation (harmonic/percussive tom pickups on steps 7, 11, 14, 15) */
    if (!gesture && density >= 1600u && (step == 7u || step == 11u || step == 14u || step == 15u)) {
        uint32_t tom_ch = 14u + disp / 2u + ((density - 1600u) * 28u) / 2500u + ((bar & 1u) ? 12u : 0u);
        if ((rytm_rng_u32(seed) % 100u) < tom_ch) {
            uint8_t tom = (step >= 14u) ? DV_LTOM : DV_HTOM;
            hits[tom] = 1;
            vels[tom] = (uint8_t)(68u + (rytm_rng_u32(seed) % 28u));
            acc[tom] = (step == 15u) ? 1u : 0u;
        }
    }
    /* 4. Open hats come from the genre grid, not from a shared offbeat rule. */
    /* 5. Clap / Rimshot reinforcement on backbeat + syncopated clave taps */
    if (hits[DV_SNARE] && snare_acc) {
        hits[DV_CLAP] = 1;
        vels[DV_CLAP] = (uint8_t)((vels[DV_SNARE] * 9u) / 10u);
        acc[DV_CLAP] = acc[DV_SNARE];
    } else if (!gesture && density >= 1900u && (step == 3u || step == 6u || step == 15u)) {
        if ((rytm_rng_u32(seed) % 100u) < (uint32_t)(18u + disp / 3u)) {
            hits[DV_CLAP] = 1;
            vels[DV_CLAP] = 74u;
            acc[DV_CLAP] = 0u;
        }
    }
}

static uint8_t rytm_orn_kind_weights(const rytm_orn_w_t *w, uint8_t roll)
{
    uint16_t total = (uint16_t)w->dbl + w->flam + w->trip + w->roll;
    uint16_t pick;
    uint16_t acc = 0;
    if (total == 0u) {
        return ORN_NONE;
    }
    pick = (uint16_t)roll % total;
    acc = (uint16_t)(acc + w->dbl);
    if (pick < acc) {
        return ORN_DRAG;
    }
    acc = (uint16_t)(acc + w->flam);
    if (pick < acc) {
        return ORN_FLAM;
    }
    acc = (uint16_t)(acc + w->trip);
    if (pick < acc) {
        return ORN_TRIPLET;
    }
    return ORN_ROLL;
}

static uint8_t rytm_orn_entry_rate(uint8_t voice_fam, const rytm_orn_w_t *w, uint8_t intensity, uint8_t rate_scale)
{
    uint32_t voice_mul;
    uint8_t cap;
    uint32_t sum;
    uint32_t raw;
    if (intensity == 0u) {
        return 0;
    }
    voice_mul = voice_fam == 0u ? 1u : (voice_fam == 1u ? 2u : 3u);
    cap = voice_fam == 0u ? 14u : (voice_fam == 1u ? 22u : 36u);
    sum = (uint32_t)w->dbl + w->flam + w->trip + w->roll;
    raw = sum * voice_mul * (uint32_t)intensity * (uint32_t)(rate_scale > 0u ? rate_scale : 1u);
    raw = raw / (100u * 9u * 100u);
    if (raw > cap) {
        raw = cap;
    }
    return (uint8_t)raw;
}

static uint8_t rytm_orn_intensity(int gesture, uint16_t density, uint16_t feel, uint8_t flam_amt, uint8_t step, uint8_t solo_tier, uint8_t solo_bar)
{
    if (gesture == GEST_FLAM_BURST) {
        return 100;
    }
    if (flam_amt == 0u && gesture == GEST_NONE) {
        return 0;
    }
    if (gesture == GEST_FILL) {
        uint32_t tail = step;
        uint32_t arc = tail >= 12u ? 95u : (tail >= 8u ? 70u : 45u);
        return (uint8_t)((arc * (1024u + rytm_feel_curve(feel))) / 5119u);
    }
    if (gesture == GEST_SOLO_DRUM || gesture == GEST_SOLO_BAND) {
        uint32_t tier_boost = solo_tier == 0u ? 45u : (solo_tier == 1u ? 65u : 85u);
        uint32_t arc = (solo_bar & 3u) == 0u ? 25u : ((solo_bar & 3u) == 1u ? 45u : ((solo_bar & 3u) == 2u ? 65u : 35u));
        uint32_t res = ((tier_boost + arc) * (1024u + rytm_feel_curve(feel))) / 5119u;
        return (uint8_t)(res > 100u ? 100u : res);
    }
    {
        uint16_t floor_d = flam_amt > 48u ? (uint16_t)(2400u - ((uint32_t)(flam_amt - 48u) * 18u)) : 2400u;
        if (density < floor_d) {
            return (uint8_t)(flam_amt > 80u ? (flam_amt - 80u) / 2u : 0u);
        }
        {
            uint32_t d = ((uint32_t)(density - floor_d) * 100u) / (4095u - floor_d + 1u);
            uint32_t f = (512u + (uint32_t)rytm_feel_curve(feel)) * 100u / 4607u;
            uint32_t res = (d * f / 100u) + ((uint32_t)flam_amt * 45u / 127u);
            return (uint8_t)(res > 100u ? 100u : res);
        }
    }
}

static void rytm_apply_ornament(uint8_t main_vel, uint8_t flam_amt, uint8_t orn_kind, rytm_drum_hit_t *hit, uint16_t feel, uint16_t sixteenth_ms)
{
    uint32_t boost;
    if (orn_kind == ORN_NONE || main_vel == 0u || flam_amt == 0u) {
        hit->orn_kind = ORN_NONE;
        hit->orn_vel = 0;
        return;
    }
    (void)feel;
    (void)sixteenth_ms;
    boost = 75u + ((uint32_t)flam_amt * 50u) / 127u;
    hit->orn_kind = orn_kind;
    switch (orn_kind) {
    case ORN_FLAM:
        hit->orn_vel = (uint8_t)(((uint32_t)main_vel * 42u * boost) / 10000u);
        break;
    case ORN_DRAG:
        hit->orn_vel = (uint8_t)(((uint32_t)main_vel * 55u * boost) / 10000u);
        break;
    case ORN_TRIPLET:
        hit->orn_vel = (uint8_t)(((uint32_t)main_vel * 68u * boost) / 10000u);
        break;
    case ORN_ROLL:
        hit->orn_vel = (uint8_t)(((uint32_t)main_vel * 64u * boost) / 10000u);
        break;
    default:
        hit->orn_kind = ORN_NONE;
        hit->orn_vel = 0;
        break;
    }
    if (hit->orn_kind != ORN_NONE && hit->orn_vel < 18u) {
        hit->orn_vel = 18u;
    }
    if (hit->orn_vel > 127u) {
        hit->orn_vel = 127u;
    }
}

static void rytm_voice_genre_bias(const rytm_bassist_t *voice, uint8_t genre, uint8_t *ghost, uint8_t *approach)
{
    *ghost = voice->ghost_pct;
    *approach = voice->approach_pct;
    if (genre >= RYTM_NGENRES) {
        return;
    }
    if ((genre == 6u || genre == 7u) && voice->pocket) {
        *ghost = (uint8_t)(*ghost / 3u);
        *approach = (uint8_t)(*approach / 2u);
    } else if ((genre == 0u || genre == 3u) && !voice->pocket) {
        *ghost = (uint8_t)(((uint16_t)*ghost * 2u) / 3u);
    } else if ((genre == 5u || genre == 6u) && voice->approach_pct > 50u) {
        *approach = (uint8_t)(*approach + 12u > 100u ? 100u : *approach + 12u);
    }
}

static void rytm_resolve_bass(const rytm_query_t *q, const rytm_basspat_t *pat_lo, const rytm_basspat_t *pat_hi, uint8_t g_frac, uint16_t density, uint16_t feel, uint8_t near, const rytm_bassist_t *voice, uint32_t *seed, rytm_bass_hit_t *out)
{
    uint8_t phrase_bar = q->bar % RYTM_PHRASE_BARS;
    uint32_t rot = (phrase_bar == 4u || phrase_bar == 5u) ? 4u : 0u;
    uint16_t hits = rytm_morph_mask(rytm_rot16(pat_lo->hits, rot), rytm_rot16(pat_hi->hits, rot), g_frac, 110u);
    uint16_t hits_fill = rytm_morph_mask(rytm_rot16(pat_lo->hits_fill, rot), rytm_rot16(pat_hi->hits_fill, rot), g_frac, 140u);
    uint16_t accent_mask = rytm_morph_mask(rytm_rot16(pat_lo->accent_mask, rot), rytm_rot16(pat_hi->accent_mask, rot), g_frac, 90u);
    uint8_t si;
    uint8_t si_step = q->step;
    uint16_t gfeel = feel;
    uint16_t groove_t = rytm_feel_curve(gfeel);
    int core;
    uint8_t fill_frac = 0;
    int has_fill = 0;
    int is_accent;
    int is_ghost = 0;
    int8_t degree;
    int8_t oct;
    uint8_t ghost_base;
    uint8_t approach_base;
    uint16_t ghost_pct;
    uint16_t approach_pct;
    uint8_t base_vel;
    uint8_t accent_vel;
    uint16_t vel_pct;
    uint8_t gate_w;
    uint8_t stacc;
    int32_t chord;

    if (phrase_bar == 4u || phrase_bar == 5u) {
        si = (uint8_t)((si_step + 4u) & 15u);
    } else if (phrase_bar == 6u) {
        si = (uint8_t)((si_step + 2u) & 15u);
    } else {
        si = si_step;
    }

    core = rytm_bit_set(hits, si_step);
    has_fill = rytm_fill_reveal(hits_fill, density, si_step, 0, &fill_frac);

    if (!core && !has_fill) {
        return;
    }
    is_accent = rytm_bit_set(accent_mask, si_step);
    if (phrase_bar == 7u && core && !is_accent && rytm_chance_roll(seed, q->step, 0, 13, groove_t) < 65u) {
        return;
    }
    if (phrase_bar == 7u && !core) {
        return;
    }

    rytm_voice_genre_bias(voice, near, &ghost_base, &approach_base);
    {
        uint32_t feel_scale = 50u + (50u * groove_t) / 4095u;
        uint16_t ghost_phrase = phrase_bar == 7u ? 0u : (phrase_bar == 4u || phrase_bar == 5u ? (uint16_t)ghost_base * 3u / 4u : ghost_base);
        uint16_t approach_phrase = phrase_bar == 4u || phrase_bar == 5u ? 28u : (phrase_bar == 6u ? 18u : (phrase_bar == 7u ? 0u : 8u));
        ghost_pct = phrase_bar == 7u ? 0u : (uint16_t)(((ghost_phrase * feel_scale) / 100u) + voice->syncop_bias / 40u);
        if (ghost_pct > 100u) {
            ghost_pct = 100u;
        }
        approach_pct = phrase_bar == 7u ? 0u : (uint16_t)(((approach_base * feel_scale) / 100u) + (approach_base > 0u ? approach_phrase : 0u));
        if (approach_pct > 100u) {
            approach_pct = 100u;
        }
    }

    is_ghost = (!core && has_fill);
    if (is_ghost) {
        uint8_t drop_bar = groove_t > 2800u ? 90u : 128u;
        if (rytm_chance_roll(seed, q->step, 0, 3, groove_t) >= ghost_pct && fill_frac < drop_bar) {
            return;
        }
    }

    degree = (int8_t)rytm_lerp_i32(pat_lo->degree[si], pat_hi->degree[si], g_frac);
    if (q->prog_depth < 32u) {
        degree = (int8_t)(((int32_t)degree * (int32_t)q->prog_depth) / 32);
    } else if (voice->pocket && q->prog_depth < 64u) {
        degree = rytm_fold_pocket(degree);
    }
    oct = (int8_t)rytm_lerp_i32(pat_lo->oct_off[si], pat_hi->oct_off[si], g_frac);
    if (oct > voice->oct_span) {
        oct = voice->oct_span;
    }
    if (oct < -voice->oct_span) {
        oct = (int8_t)(-voice->oct_span);
    }

    chord = (int32_t)RYTM_GENRE_PROG_8[near][phrase_bar];
    degree = (int8_t)(((int32_t)degree + chord * (int32_t)q->prog_depth / 96) % 7);
    if (q->prog_depth >= 64u && ((phrase_bar & 1u) || (q->step & 2u)))
        degree = (int8_t)((degree + ((q->step * 3u + phrase_bar) % 5u)) % 7);
    if (q->prog_depth >= 96u && (phrase_bar >= 4u || (q->step == 14u)))
        oct = (int8_t)(oct + 1);

    out->midi_note = rytm_scale_note(q->root_note, q->scale_id, degree, oct);
    out->active = 1;

    base_vel = rytm_lerp_u8(pat_lo->base_vel, pat_hi->base_vel, g_frac);
    accent_vel = rytm_lerp_u8(pat_lo->accent_vel, pat_hi->accent_vel, g_frac);
    if (accent_vel + voice->accent_boost > 127u) {
        accent_vel = 127;
    } else {
        accent_vel = (uint8_t)(accent_vel + voice->accent_boost);
    }

    if (is_accent) {
        vel_pct = accent_vel;
    } else {
        vel_pct = base_vel;
    }
    if (is_ghost) {
        vel_pct = (uint16_t)rytm_ghost_vel_pct(fill_frac, 12, 45);
        vel_pct = rytm_feel_lerp_u16(70, vel_pct, gfeel);
    } else {
        vel_pct = rytm_feel_lerp_u16(70, vel_pct, gfeel);
    }
    out->vel = rytm_pct_to_midi_vel(vel_pct > 100u ? 100u : vel_pct);
    out->accent = is_accent ? 1u : 0u;
    out->dead_ghost = (is_ghost && groove_t > 1800u && rytm_chance_roll(seed, q->step, 0, 37, groove_t) < (uint8_t)(18u + groove_t / 200u)) ? 1u : 0u;

    gate_w = rytm_lerp_u8(pat_lo->gate_w[si], pat_hi->gate_w[si], g_frac);
    if (out->dead_ghost) {
        if (gate_w > 28u) {
            gate_w = 28;
        }
    }
    stacc = (uint8_t)rytm_feel_lerp_u16(100, voice->staccato, gfeel);
    out->gate_pct = (uint8_t)(((uint16_t)gate_w * stacc) / 100u);
    if (out->gate_pct < 10u) {
        out->gate_pct = 10;
    }

    out->legato = (q->last_bass_note != 0u && out->midi_note == q->last_bass_note) ? 1u : 0u;
    {
        uint16_t cutoff = RYTM_GENRE_CUTOFF[near];
        out->filter_mod = (uint8_t)(cutoff * 127u / 2300u);
        if (is_accent) {
            out->filter_mod = (uint8_t)(out->filter_mod + 20u > 127u ? 127u : out->filter_mod + 20u);
        }
    }

    if (is_ghost && rytm_chance_roll(seed, q->step, 0, 11, groove_t) < approach_pct) {
        if (out->midi_note > 24u) {
            out->midi_note--;
        }
    }
}

static int rytm_step_cond_pass(uint8_t cond, const rytm_query_t *q, uint8_t voice)
{
    uint8_t b = q->bar % 8u;
    int is_fill = (q->gesture == GEST_FILL || q->gesture == GEST_SOLO_DRUM ||
                   q->gesture == GEST_SOLO_BASS || q->gesture == GEST_SOLO_BAND || b == 3u || b == 7u);

    return fm_seq_cond_pass(cond, q->ovr_step, q->bar, voice, is_fill);
}

static void rytm_resolve_step(const rytm_query_t *q, const rytm_seq_ovr_t *ovr, rytm_step_events_t *out)
{
    uint8_t lo;
    uint8_t hi;
    uint8_t g_frac;
    uint16_t pos4095;
    uint16_t density;
    uint16_t feel;
    uint8_t near;
    const rytm_drumpat_t *pat;
    const rytm_drumpat_t *pat_lo;
    const rytm_drumpat_t *pat_hi;
    const rytm_drummer_t *prof;
    const rytm_bassist_t *bass_prof;
    const rytm_basspat_t *bpat_lo;
    const rytm_basspat_t *bpat_hi;
    uint32_t seed;
    uint8_t v;
    uint8_t hits[7];
    uint8_t vels[7];
    uint8_t acc[7];
    uint8_t ghosts[7];
    uint16_t kick_rot;
    uint16_t kick_mask;
    uint16_t kick_fill_mask;
    uint16_t kick_acc_mask;
    int gesture;
    int is_break;
    const rytm_fill3_t *fill_fig;
    const rytm_solo_t *solo_fig;
    uint8_t fill_var;
    uint8_t solo_tier;
    uint16_t sixteenth_ms;
    uint8_t roll_scale;
    int snare_locked;

    for (v = 0; v < RYTM_NDVOICES; v++) {
        out->drums[v].vel = 0;
        out->drums[v].accent = 0;
        out->drums[v].orn_kind = ORN_NONE;
        out->drums[v].orn_vel = 0;
        out->drums[v].micro_ms = 0;
    }
    out->bass.active = 0;
    out->bass.accent = 0;
    out->bass.legato = 0;
    out->bass.dead_ghost = 0;
    out->bass.micro_ms = 0;
    out->swing_delay_ms = 0;

    pos4095 = rytm_u8_to4095(q->genre);
    density = rytm_u8_to4095(q->density);
    feel = rytm_u8_to4095(q->feel);
    rytm_genre_pair(pos4095, &lo, &hi, &g_frac);
    near = g_frac < 128u ? lo : hi;
    pat = &RYTM_DRUM_PATS[near];
    pat_lo = &RYTM_DRUM_PATS[lo];
    pat_hi = &RYTM_DRUM_PATS[hi];
    prof = &RYTM_DRUMMERS[q->drummer % RYTM_NDRUMMERS];
    bass_prof = &RYTM_BASSISTS[q->bassist % RYTM_NBASSISTS];
    bpat_lo = &RYTM_BASS_PATS[lo];
    bpat_hi = &RYTM_BASS_PATS[hi];
    seed = q->rng_seed ^ ((uint32_t)q->step * 2654435761u) ^ ((uint32_t)q->bar * 2246822519u);
    if (seed == 0u) {
        seed = 1u;
    }
    sixteenth_ms = (uint16_t)(60000u / (uint32_t)(q->bpm > 0u ? q->bpm : 120u) / 4u);

    kick_rot = (uint16_t)(((int32_t)prof->clave_rot * (int32_t)rytm_persona_w(feel)) / 4095);
    kick_rot = (uint16_t)(kick_rot & 15u);
    kick_mask = rytm_morph_mask(rytm_rot16(pat_lo->kick, kick_rot), rytm_rot16(pat_hi->kick, kick_rot), g_frac, 0u);
    kick_fill_mask = rytm_morph_mask(rytm_rot16(pat_lo->kick_fill, kick_rot), rytm_rot16(pat_hi->kick_fill, kick_rot), g_frac, 20u);
    kick_acc_mask = rytm_morph_mask(rytm_rot16(pat_lo->kick_acc_mask, kick_rot), rytm_rot16(pat_hi->kick_acc_mask, kick_rot), g_frac, 40u);
    roll_scale = rytm_roll_rate_scale(prof->roll_pct, feel);

    gesture = (q->gesture != GEST_NONE && q->gesture != GEST_SOLO_BASS) ? 1 : 0;
    is_break = q->gesture == GEST_BREAK ? 1 : 0;
    fill_fig = 0;
    solo_fig = 0;
    fill_var = (uint8_t)((near + q->bar + (seed & 3u)) % 3u);
    solo_tier = (uint8_t)(q->solo_bar % 3u);

    if (q->gesture == GEST_FILL || q->gesture == GEST_BREAK) {
        fill_fig = &RYTM_FILLS[near][fill_var];
        if (is_break) {
            fill_fig = &RYTM_BREAKS[near][fill_var];
        }
    } else if (q->gesture == GEST_SOLO_DRUM || q->gesture == GEST_SOLO_BAND) {
        solo_fig = &RYTM_SOLOS[near][solo_tier];
    }

    {
        int kick_core = 0;
        int snare_core = 0;
        int hats_core = 0;
        uint8_t k_frac = 0;
        uint8_t s_frac = 0;
        uint8_t h_frac = 0;
        uint32_t ghost_rot = rytm_humanize_curve(feel) >= 1024u ? (q->bar % 4u) : 0u;
        uint8_t tightness = rytm_lerp_u8(pat_lo->tightness, pat_hi->tightness, g_frac);
        uint32_t machine = rytm_persona_pct(prof->machine, feel);
        if (machine > 100u) {
            machine = 100u;
        }
        int32_t humanize_left = (int32_t)(((100u - machine) * (uint32_t)(q->human > 0u ? q->human : 100u)) / 127u);

        if (solo_fig) {
            kick_core = rytm_bit_set(solo_fig->kick, q->step);
            snare_core = rytm_bit_set(solo_fig->snare, q->step);
            hats_core = rytm_bit_set(solo_fig->hats, q->step);
        } else if (fill_fig) {
            kick_core = rytm_bit_set(fill_fig->kick, q->step);
            snare_core = rytm_bit_set(fill_fig->snare, q->step);
            hats_core = rytm_bit_set(fill_fig->hats, q->step);
        } else {
            kick_core = rytm_bit_set(kick_mask, q->step);
            snare_core = rytm_morph_bit(pat_lo->snare, pat_hi->snare, q->step, g_frac, 50u);
            hats_core = rytm_morph_bit(pat_lo->hats, pat_hi->hats, q->step, g_frac, 100u);
        }

        snare_locked = (q->step == 4u || q->step == 12u) && (uint32_t)rytm_rng_pct(&seed) < prof->backbeat_lock;

        if (!gesture) {
            int kf = rytm_fill_reveal(kick_fill_mask, density, q->step, ghost_rot, &k_frac);
            int sf = rytm_fill_reveal(rytm_morph_mask(pat_lo->snare_fill, pat_hi->snare_fill, g_frac, 60u), density, q->step, ghost_rot, &s_frac);
            int hf = rytm_fill_reveal(rytm_morph_mask(pat_lo->hats_fill, pat_hi->hats_fill, g_frac, 80u), density, q->step, ghost_rot, &h_frac);
            if (kf && !rytm_ghost_survives(k_frac, prof, feel, &seed, 0)) {
                kf = 0;
            }
            if (sf && !rytm_ghost_survives(s_frac, prof, feel, &seed, snare_locked)) {
                sf = 0;
            } else if (!sf && !snare_core) {
                uint8_t ex = 0;
                if (rytm_ghost_extra(prof, feel, &seed, q->step, snare_core, &ex)) {
                    sf = 1;
                    s_frac = ex;
                }
            }
            if (hf && !rytm_ghost_survives(h_frac, prof, feel, &seed, 0)) {
                hf = 0;
            }
            kick_core = kick_core || kf;
            snare_core = snare_core || sf;
            hats_core = hats_core || hf;
            (void)k_frac;
            (void)s_frac;
            (void)h_frac;
        }

        for (v = 0; v < 7u; v++) {
            hits[v] = 0;
            vels[v] = 0;
            acc[v] = 0;
            ghosts[v] = 0;
        }

        if (kick_core || snare_core || hats_core) {
            uint16_t vpct;
            if (kick_core) {
                vpct = solo_fig ? rytm_core_vel_pct(pat_lo->kick_base, pat_lo->kick_accent, kick_acc_mask, q->step, feel, &seed, tightness)
                                  : (gesture ? rytm_fill_vel_pct(rytm_lerp_u8(pat_lo->kick_base, pat_hi->kick_base, g_frac), rytm_lerp_u8(pat_lo->kick_accent, pat_hi->kick_accent, g_frac), q->step)
                                             : rytm_core_vel_pct(rytm_lerp_u8(pat_lo->kick_base, pat_hi->kick_base, g_frac), rytm_lerp_u8(pat_lo->kick_accent, pat_hi->kick_accent, g_frac), kick_acc_mask, q->step, feel, &seed, tightness));
                hits[DV_KICK] = 1;
                vels[DV_KICK] = rytm_pct_to_midi_vel(vpct > 100u ? 100u : vpct);
                acc[DV_KICK] = rytm_bit_set(kick_acc_mask, q->step) ? 1u : 0u;
            }
            if (snare_core) {
                vpct = rytm_core_vel_pct(rytm_lerp_u8(pat_lo->snare_base, pat_hi->snare_base, g_frac), rytm_lerp_u8(pat_lo->snare_accent, pat_hi->snare_accent, g_frac), pat->snare_acc_mask, q->step, feel, &seed, tightness);
                hits[DV_SNARE] = 1;
                vels[DV_SNARE] = rytm_pct_to_midi_vel(vpct > 100u ? 100u : vpct);
                acc[DV_SNARE] = rytm_bit_set(pat->snare_acc_mask, q->step) ? 1u : 0u;
            }
            if (hats_core) {
                int hats_acc = rytm_bit_set(pat->hats_acc_mask, q->step);
                vpct = rytm_core_vel_pct(rytm_lerp_u8(pat_lo->hats_base, pat_hi->hats_base, g_frac), rytm_lerp_u8(pat_lo->hats_accent, pat_hi->hats_accent, g_frac), pat->hats_acc_mask, q->step, feel, &seed, tightness);
                if (!gesture) {
                    vpct = rytm_hat_offbeat_duck(vpct, q->step, hats_acc, feel);
                }
                hits[DV_CH] = 1;
                vels[DV_CH] = rytm_pct_to_midi_vel(vpct > 100u ? 100u : vpct);
                acc[DV_CH] = hats_acc ? 1u : 0u;
            }
        }

        rytm_derive_extra_voices(hits, vels, acc, q->step, q->bar, near, density, gesture, acc[DV_SNARE], acc[DV_CH], prof, feel, &seed, snare_locked);
        if (!solo_fig && !fill_fig) {
            if (rytm_morph_bit(pat_lo->oh, pat_hi->oh, q->step, g_frac, 150u)) {
                hits[DV_OH] = 1;
                hits[DV_CH] = 0;
                vels[DV_OH] = 104;
                acc[DV_OH] = 1;
            }
            if (rytm_morph_bit(pat_lo->ltom, pat_hi->ltom, q->step, g_frac, 180u)) {
                hits[DV_LTOM] = 1;
                vels[DV_LTOM] = 108;
                acc[DV_LTOM] = 1;
            }
            if (rytm_morph_bit(pat_lo->htom, pat_hi->htom, q->step, g_frac, 30u)) {
                hits[DV_HTOM] = 1;
                vels[DV_HTOM] = 100;
                acc[DV_HTOM] = 1;
            }
            if (rytm_morph_bit(pat_lo->clap, pat_hi->clap, q->step, g_frac, 70u)) {
                hits[DV_CLAP] = 1;
                vels[DV_CLAP] = 102;
                acc[DV_CLAP] = 1;
            }
        }

        for (v = 0; v < RYTM_NDVOICES; v++) {
            if (!hits[v]) {
                continue;
            }
            out->drums[v].vel = vels[v];
            out->drums[v].accent = acc[v];
            {
                uint8_t fam = v <= DV_KICK ? 0u : (v == DV_SNARE || v == DV_CLAP ? 1u : 2u);
                const rytm_orn_weights_t *gw = &RYTM_ORN_WEIGHTS[near];
                const rytm_orn_w_t *ow = fam == 0 ? &gw->kick : (fam == 1 ? &gw->snare : &gw->hats);
                uint8_t intensity = rytm_orn_intensity(q->gesture, density, feel, q->flam_amt, q->step, solo_tier, q->solo_bar);
                uint8_t entry = rytm_orn_entry_rate(fam, ow, intensity, roll_scale);
                uint8_t chance = rytm_step_chance(q->step, v, 167u + (uint32_t)q->bar * 29u + (seed & 63u));
                if (q->gesture == GEST_FLAM_BURST || (entry > 0u && chance < entry)) {
                    uint8_t sub = rytm_step_chance(chance, v, 167u + (uint32_t)q->bar * 13u);
                    uint8_t kind = rytm_orn_kind_weights(ow, sub);
                    rytm_apply_ornament(vels[v], q->gesture == GEST_FLAM_BURST ? 127u : q->flam_amt, kind, &out->drums[v], feel, sixteenth_ms);
                }
            }
            {
                int32_t timing_off = (rytm_feel_lerp_i32(0, rytm_lerp_i32(pat_lo->timing[q->step], pat_hi->timing[q->step], g_frac), feel) * humanize_left) / 100;
                int32_t push = (rytm_feel_lerp_i32(0, prof->push_pull, feel) * humanize_left) / 100;
                int32_t fam_push = v == DV_SNARE || v == DV_CLAP ? pat_lo->push_ms[1] : (v == DV_CH || v == DV_OH ? pat_lo->push_ms[2] : pat_lo->push_ms[0]);
                int32_t drift = rytm_limb_drift_ms(q->bar, (int32_t)((rytm_persona_pct(prof->limb_drift, feel) * 6u) / 100u), v);
                int32_t ms = timing_off + push + fam_push + drift;
                if (q->human > 8u) {
                    int32_t span = ((int32_t)q->human * 18) / 127;
                    int32_t j = (int32_t)(rytm_rng_pct(&seed) % (uint32_t)(span * 2 + 1)) - span;
                    int32_t nv = (int32_t)vels[v] + j;
                    if (nv < 1)
                        nv = 1;
                    if (nv > 127)
                        nv = 127;
                    vels[v] = (uint8_t)nv;
                    out->drums[v].vel = vels[v];
                    ms += j / 2;
                    if (v == DV_CH || v == DV_OH)
                        ms += j / 3;
                }
                if (ms > 35) {
                    ms = 35;
                }
                if (ms < -20) {
                    ms = -20;
                }
                out->drums[v].micro_ms = (int8_t)ms;
            }
        }
    }

    if (q->gesture != GEST_SOLO_DRUM && q->gesture != GEST_BREAK) {
        uint16_t b_dens = (uint16_t)(((uint32_t)q->bass_density * 4095u) / 127u);
        uint16_t b_feel = (uint16_t)(((uint32_t)q->bass_feel * 4095u) / 127u);
        rytm_resolve_bass(q, bpat_lo, bpat_hi, g_frac, b_dens, b_feel, near, bass_prof, &seed, &out->bass);
        if (out->bass.active && q->bass_flam > 12u) {
            uint8_t s_ch = rytm_step_chance(q->step, 7u, 211u + (uint32_t)q->bar * 19u);
            if (s_ch < (uint8_t)(((uint32_t)q->bass_flam * 65u) / 127u))
                out->bass.legato = 1u;
        }
    } else if (q->gesture == GEST_BREAK && q->step == 0u) {
        out->bass.active = 1;
        out->bass.midi_note = (uint8_t)(36u + q->root_note);
        out->bass.vel = 60;
        out->bass.gate_pct = 95;
        out->bass.filter_mod = 40;
    }

    if (q->gesture == GEST_SOLO_BASS) {
        uint16_t mask = RYTM_SOLO_RHYTHM[near][fill_var];
        if (rytm_bit_set(mask, q->step)) {
            out->bass.active = 1;
            out->bass.midi_note = rytm_scale_note(q->root_note, q->scale_id, (int32_t)(q->step % 7u), 1);
            out->bass.vel = 100;
            out->bass.accent = 1;
            out->bass.gate_pct = 70;
            out->bass.filter_mod = 100;
        }
    }

    /* Pocket lock: bass downbeats sit on the kick; ghosts yield the snare backbeat. */
    if (bass_prof->pocket && q->gesture != GEST_SOLO_DRUM) {
        int kick_on = out->drums[DV_KICK].vel > 0u;
        int snare_bb = (q->step == 4u || q->step == 12u) && out->drums[DV_SNARE].vel > 0u;
        if (snare_bb && out->bass.dead_ghost)
            out->bass.active = 0u;
        if (kick_on && (q->step % 4u) == 0u && !out->bass.active) {
            int32_t chord = (int32_t)RYTM_GENRE_PROG_8[near][q->bar % RYTM_PHRASE_BARS];
            int32_t deg = (chord * (int32_t)q->prog_depth) / 127;
            out->bass.active = 1u;
            out->bass.midi_note = rytm_scale_note(q->root_note, q->scale_id, deg, 0);
            out->bass.vel = 88u;
            out->bass.gate_pct = 80u;
            out->bass.accent = out->drums[DV_KICK].accent;
            out->bass.micro_ms = out->drums[DV_KICK].micro_ms;
        } else if (kick_on && out->bass.active) {
            if ((q->step % 4u) == 0u) {
                int32_t chord = (int32_t)RYTM_GENRE_PROG_8[near][q->bar % RYTM_PHRASE_BARS];
                int32_t deg = (chord * (int32_t)q->prog_depth) / 127;
                out->bass.midi_note = rytm_scale_note(q->root_note, q->scale_id, deg, 0);
            }
            out->bass.micro_ms = out->drums[DV_KICK].micro_ms;
        }
    }

    /* Call & response on bars 4 and 8: bass walks up, snare flams into the downbeat. */
    if ((q->bar == 3u || q->bar == 7u) && q->step >= 12u && q->gesture == GEST_NONE) {
        if (out->bass.active) {
            int32_t deg = (int32_t)q->step - 11;
            out->bass.midi_note = rytm_scale_note(q->root_note, q->scale_id, deg, 0);
            out->bass.vel = (uint8_t)(out->bass.vel < 110u ? out->bass.vel + 8u : 118u);
        }
        if (q->step == 15u && out->drums[DV_SNARE].vel)
            rytm_apply_ornament(out->drums[DV_SNARE].vel, 96u, ORN_FLAM, &out->drums[DV_SNARE], feel, sixteenth_ms);
    }

    /* SEQ overrides. VARY > LOCK mutates Euclid force masks per lane instead of freezing them. */
    if (ovr) {
        uint8_t bvary = fm_seq_vary_norm(ovr->vary_lane[7]);
        uint8_t btier = fm_seq_vary_tier(bvary);
        uint8_t broll = rytm_step_chance(q->step, 7u, 41u + (seed & 255u));
        uint8_t bmutate_pct = (uint8_t)(((uint32_t)bvary * 88u) / 127u);
        int bmutate = bvary > 0u && broll < bmutate_pct;

        for (v = 0; v < RYTM_NDVOICES; v++) {
            uint8_t vary = fm_seq_vary_norm(ovr->vary_lane[v]);
            uint8_t tier = fm_seq_vary_tier(vary);
            uint8_t roll = rytm_step_chance(q->step, (uint8_t)(v + (q->bar & 7u)), 41u + (seed & 255u));
            uint8_t mutate_pct = (uint8_t)(((uint32_t)vary * 88u) / 127u);
            uint8_t ghost_thr = (uint8_t)(((uint32_t)vary * 64u) / 127u);
            uint8_t drop_thr = (uint8_t)(((uint32_t)vary * 28u) / 127u);
            uint8_t ghost_vel = (uint8_t)(48u + ((uint32_t)vary * 56u) / 127u);
            int mutate = vary > 0u && roll < mutate_pct;
            if (fm_seq_bit(ovr->drum_force_off[v], q->ovr_step)) {
                if (mutate && ((roll & 1u) != 0u))
                    out->drums[v].vel = ghost_vel;
                else
                    out->drums[v].vel = 0;
            } else if (fm_seq_bit(ovr->drum_force_on[v], q->ovr_step)) {
                if (mutate && roll < drop_thr)
                    out->drums[v].vel = 0u;
                else if (mutate && (roll & 2u))
                    out->drums[v].vel = 36u;
                else if (out->drums[v].vel == 0u)
                    out->drums[v].vel = 90u;
                if (mutate && tier >= 2u && roll > 70u)
                    rytm_apply_ornament(out->drums[v].vel, 96u, tier >= 4u ? ORN_ROLL : ORN_FLAM, &out->drums[v], feel, sixteenth_ms);
            } else if (mutate) {
                if (out->drums[v].vel == 0u && roll < ghost_thr) {
                    out->drums[v].vel = ghost_vel;
                    if (tier >= 3u && (roll & 3u) == 0u)
                        rytm_apply_ornament(out->drums[v].vel, 96u, ORN_FLAM, &out->drums[v], feel, sixteenth_ms);
                } else if (out->drums[v].vel > 0u && vary >= 48u && roll > 82u && (q->step & 3u) != 0u) {
                    out->drums[v].vel = 0u;
                } else if (out->drums[v].vel > 0u && vary >= 32u && roll < drop_thr) {
                    out->drums[v].vel = 0u;
                } else if (out->drums[v].vel > 0u && tier >= 2u && roll > 64u) {
                    rytm_apply_ornament(out->drums[v].vel, 96u, tier >= 4u ? ORN_ROLL : ORN_FLAM, &out->drums[v], feel, sixteenth_ms);
                }
            }
            if (fm_seq_bit(ovr->drum_accent[v], q->ovr_step))
                out->drums[v].accent = 1;
            if (fm_seq_bit(ovr->drum_flam[v], q->ovr_step))
                rytm_apply_ornament(out->drums[v].vel, 127, ORN_FLAM, &out->drums[v], feel, sixteenth_ms);
            if (ovr->step_prob[v][q->ovr_step] && !rytm_step_cond_pass(ovr->step_prob[v][q->ovr_step], q, (uint8_t)v))
                out->drums[v].vel = 0;
            if (ovr->step_vel[v][q->ovr_step])
                out->drums[v].vel = ovr->step_vel[v][q->ovr_step];
            if (out->drums[v].vel > 0u &&
                !fm_seq_trig_pass(ovr->lane_trig[v], q->ovr_step, q->bar, (uint8_t)v))
                out->drums[v].vel = 0u;
            if (out->drums[v].vel > 0u && ovr->lane_vel[v] != 0) {
                int32_t nv = (int32_t)out->drums[v].vel +
                             (int32_t)fm_seq_rand_vel(ovr->lane_vel[v], q->ovr_step, q->bar, (uint8_t)v,
                                                      q->rng_seed);
                out->drums[v].vel = (uint8_t)(nv < 1 ? 1 : nv > 127 ? 127 : nv);
            }
            if (ovr->step_micro[v][q->ovr_step])
                out->drums[v].micro_ms = (int8_t)(out->drums[v].micro_ms + ovr->step_micro[v][q->ovr_step]);
            out->drums[v].accent = out->drums[v].vel > 105u ? 1u : out->drums[v].accent;
        }
        if (fm_seq_bit(ovr->bass_force_off, q->ovr_step)) {
            if (bmutate && ((broll & 1u) != 0u)) {
                if (!out->bass.active) {
                    out->bass.active = 1;
                    out->bass.midi_note = rytm_scale_note(q->root_note, q->scale_id, (int32_t)((q->step + broll) % 7u), 0);
                    out->bass.vel = 78u;
                    out->bass.gate_pct = 55u;
                }
            } else
                out->bass.active = 0;
        } else if (fm_seq_bit(ovr->bass_force_on, q->ovr_step)) {
            if (bmutate && bvary >= 48u && broll < (uint8_t)(((uint32_t)bvary * 28u) / 127u)) {
                out->bass.active = 0;
            } else {
                out->bass.active = 1;
                if (ovr->bass_note[q->ovr_step] != 0u)
                    out->bass.midi_note = ovr->bass_note[q->ovr_step];
                if (bmutate && btier >= 2u && broll > 65u)
                    out->bass.midi_note = rytm_scale_note(q->root_note, q->scale_id, (int32_t)((q->step + broll) % 7u), (broll & 1u) ? 1 : 0);
                out->bass.vel = ovr->step_vel[7][q->ovr_step] ? ovr->step_vel[7][q->ovr_step] : 90u;
            }
        } else if (bmutate) {
            if (!out->bass.active && broll < (uint8_t)(((uint32_t)bvary * 64u) / 127u)) {
                out->bass.active = 1;
                out->bass.midi_note = rytm_scale_note(q->root_note, q->scale_id, (int32_t)((q->step * 2u + q->bar) % 7u), 0);
                out->bass.vel = (uint8_t)(48u + ((uint32_t)bvary * 56u) / 127u);
                out->bass.gate_pct = 50u;
            } else if (out->bass.active && btier >= 2u && broll > 60u) {
                out->bass.midi_note = rytm_scale_note(q->root_note, q->scale_id, (int32_t)((q->step + broll) % 7u), (btier >= 4u && (broll & 1u)) ? 1 : 0);
            }
        }
        if (ovr->bass_acc[q->ovr_step] & 1u)
            out->bass.accent = 1;
        if (ovr->bass_acc[q->ovr_step] & 2u)
            out->bass.legato = 1;
        if (ovr->bass_gate[q->ovr_step])
            out->bass.gate_pct = ovr->bass_gate[q->ovr_step];
        if (ovr->step_prob[7][q->ovr_step] && !rytm_step_cond_pass(ovr->step_prob[7][q->ovr_step], q, 7u))
            out->bass.active = 0;
        if (out->bass.active &&
            !fm_seq_trig_pass(ovr->lane_trig[7], q->ovr_step, q->bar, 7u))
            out->bass.active = 0u;
        if (out->bass.active) {
            if (ovr->bass_note[q->ovr_step] != 0u)
                out->bass.midi_note = ovr->bass_note[q->ovr_step];
            if (ovr->step_tune[7][q->ovr_step] != 0) {
                int32_t nn = (int32_t)out->bass.midi_note + ovr->step_tune[7][q->ovr_step];
                out->bass.midi_note = (uint8_t)(nn < 24 ? 24 : nn > 84 ? 84 : nn);
            }
            if (ovr->lane_vel[7] != 0) {
                int32_t nv = (int32_t)out->bass.vel +
                             (int32_t)fm_seq_rand_vel(ovr->lane_vel[7], q->ovr_step, q->bar, 7u, q->rng_seed);
                out->bass.vel = (uint8_t)(nv < 1 ? 1 : nv > 127 ? 127 : nv);
            }
            if (ovr->lane_gate[7] != 0) {
                int32_t ng = (int32_t)out->bass.gate_pct +
                             (int32_t)fm_seq_rand_gate(ovr->lane_gate[7], q->ovr_step, q->bar, 7u, q->rng_seed);
                out->bass.gate_pct = (uint8_t)(ng < 10 ? 10 : ng > 100 ? 100 : ng);
                if (ng >= 95)
                    out->bass.legato = 1u;
            }
            if (ovr->lane_tune[7] != 0) {
                int32_t nn = (int32_t)out->bass.midi_note +
                             (int32_t)fm_seq_rand_pitch(ovr->lane_tune[7], q->ovr_step, q->bar, 7u, q->rng_seed);
                out->bass.midi_note = (uint8_t)(nn < 24 ? 24 : nn > 84 ? 84 : nn);
            }
        }
        if (ovr->step_micro[7][q->ovr_step])
            out->bass.micro_ms = (int8_t)(out->bass.micro_ms + ovr->step_micro[7][q->ovr_step]);
    }

    {
        uint8_t bias = rytm_shuffled_bias((uint8_t)rytm_lerp_i32(RYTM_SWING_BIAS[lo], RYTM_SWING_BIAS[hi], g_frac), prof->shuffle);
        uint32_t swing_pct = ((uint32_t)bias * rytm_feel_curve(feel)) / 4095u;
        if (swing_pct > (uint32_t)q->swing_max) {
            swing_pct = q->swing_max;
        }
        if (q->step & 1u) {
            out->swing_delay_ms = (uint8_t)((sixteenth_ms * swing_pct * 2u) / 300u);
        }
    }
}


#endif /* RYTM_DATA_H */
