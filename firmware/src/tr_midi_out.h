/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef TR_MIDI_OUT_H
#define TR_MIDI_OUT_H

static uint32_t trm_held[16][4]; /* bit per note, per channel */

static void trm_on(uint32_t st, uint32_t n, uint32_t v)
{
    uint32_t ch = st & 0x0Fu;

    if (v == 0u) {
        midi_send(st, n, 0);
        trm_held[ch][n >> 5] &= ~(1u << (n & 31u));
        return;
    }
    midi_send(st, n, v);
    trm_held[ch][n >> 5] |= 1u << (n & 31u);
}

static void trm_off(uint32_t st, uint32_t n)
{
    uint32_t ch = st & 0x0Fu;

    midi_send(st, n, 0);
    trm_held[ch][n >> 5] &= ~(1u << (n & 31u));
}

static int trm_is_on(uint32_t ch, uint32_t n)
{
    if (ch >= 16u || n >= 128u)
        return 0;
    return (int)((trm_held[ch][n >> 5] >> (n & 31u)) & 1u);
}

static void trm_panic(void)
{
    uint32_t ch;
    uint32_t w;
    uint32_t b;

    for (ch = 0; ch < 16u; ch++) {
        for (w = 0; w < 4u; w++) {
            uint32_t mask = trm_held[ch][w];

            trm_held[ch][w] = 0;
            for (b = 0; b < 32u; b++) {
                if (mask & (1u << b))
                    midi_send(0x80u | ch, w * 32u + b, 0);
            }
        }
    }
}

#endif
