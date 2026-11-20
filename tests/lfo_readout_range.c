/* Host unit: knob readout = base ± depth, clamped to 0..127 for any depth > 0; depth 0 = live. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

typedef struct { uint8_t unused; } felucca_font_t;
static const felucca_font_t FONT_S = {0};
enum { C_DIM = 0, C_ACC = 0, C_WHITE = 0, C_LINE = 0, C_BLACK = 0, C_HI = 0, C_GRAY = 0, C_AMB = 0 };
static inline uint16_t RGB(uint8_t r, uint8_t g, uint8_t b) { (void)r; (void)g; (void)b; return 0; }

#include "../firmware/src/fm_lfo.h"

static int fails;

static void expect(int cond, const char *msg)
{
    if (!cond) {
        printf("FAIL %s\n", msg);
        fails++;
    }
}

static void range_test(uint8_t base, uint8_t depth, uint8_t want_lo, uint8_t want_hi, const char *msg)
{
    uint8_t lo = 0xFFu, hi = 0xFFu;

    fm_lfo_readout_range(base, depth, &lo, &hi);
    expect(lo == want_lo && hi == want_hi, msg);
}

int main(void)
{
    uint8_t lo = 0, hi = 0;
    uint8_t r;
    uint8_t fast_rate = 80u;
    uint8_t slow_rate = 10u;
    uint32_t bpm = 120u, t;

    memset(fm_lfo_readout_slots, 0, sizeof(fm_lfo_readout_slots));

    expect(fm_lfo_slot_hz100(fast_rate, bpm) >= FM_LFO_READOUT_FAST_HZ100, "fast_rate above threshold");
    expect(fm_lfo_slot_hz100(slow_rate, bpm) < FM_LFO_READOUT_FAST_HZ100, "slow_rate below threshold");

    /* Range: base ± depth, clamped at both ends (unipolar and bipolar share raw 0..127, centre 64). */
    range_test(10u, 40u, 0u, 50u, "unipolar low end clamps to 0");
    range_test(100u, 60u, 40u, 127u, "unipolar high end clamps to 127");
    range_test(0u, 127u, 0u, 127u, "base 0, full depth -> 0..127");
    range_test(127u, 0u, 127u, 127u, "depth 0 at max -> 127~127");
    range_test(64u, 40u, 24u, 104u, "bipolar centre 64 ± 40");
    range_test(64u, 127u, 0u, 127u, "bipolar centre, full depth clamps both ends");
    range_test(20u, 30u, 0u, 50u, "bipolar negative side clamps to 0");

    /* Gate: depth 0 -> live value. */
    r = fm_lfo_readout_update(0u, 64u, 64u, 0u, fast_rate, bpm, &lo, &hi);
    expect(r == 0u, "depth 0 -> live");

    /* Slow LFO, depth > 0: base ± depth on the first frame too (no fast gate). */
    memset(fm_lfo_readout_slots, 0, sizeof(fm_lfo_readout_slots));
    r = fm_lfo_readout_update(0u, 64u, 64u, 40u, slow_rate, bpm, &lo, &hi);
    expect(r == 1u && lo == 24u && hi == 104u, "slow first frame -> base±depth");

    /* Fast by rate: base ± depth on every frame, whatever the effective value does. */
    memset(fm_lfo_readout_slots, 0, sizeof(fm_lfo_readout_slots));
    for (t = 0u; t < 1000u; t++) {
        uint8_t eff = (uint8_t)((t * 37u) % 128u);

        r = fm_lfo_readout_update(1u, eff, 64u, 40u, fast_rate, bpm, &lo, &hi);
        expect(r == 1u && lo == 24u && hi == 104u, "fast rate: base±depth every frame");
    }

    /* Slow LFO, any movement pattern: base ± depth (clamped), fast gate no longer decides. */
    memset(fm_lfo_readout_slots, 0, sizeof(fm_lfo_readout_slots));
    r = fm_lfo_readout_update(2u, 50u, 80u, 60u, slow_rate, bpm, &lo, &hi);
    expect(r == 1u && lo == 20u && hi == 127u, "slow first frame (2): base 80 ± 60 -> 20..127");
    r = fm_lfo_readout_update(2u, 60u, 80u, 60u, slow_rate, bpm, &lo, &hi);
    expect(r == 1u && lo == 20u && hi == 127u, "slow moving: base 80 ± 60 -> 20..127");
    r = fm_lfo_readout_update(2u, 61u, 80u, 60u, slow_rate, bpm, &lo, &hi);
    expect(r == 1u && lo == 20u && hi == 127u, "slow, 1 step per frame: base ± depth");

    /* Depth 0 at any rate -> live value. */
    r = fm_lfo_readout_update(3u, 64u, 64u, 0u, slow_rate, bpm, &lo, &hi);
    expect(r == 0u, "slow depth 0 -> live");

    if (fails) {
        printf("lfo_readout_range: %d FAIL\n", fails);
        return 1;
    }
    printf("lfo_readout_range: OK\n");
    return 0;
}
