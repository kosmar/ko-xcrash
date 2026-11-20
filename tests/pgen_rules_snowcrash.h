/* KO XCRASH musical rules beyond the genome must/ban masks; included by tests/plate_gen_drift.c.
 * Interference groove: every lane is a p-step pattern tiled over 32 steps (p = SNOWCRASH_LANE_PERIOD), and the
 * pattern popcount stays inside the era band (SNOWCRASH_GEN_HITS). */
static int pgen_rules_check(uint8_t era, int32_t n)
{
    uint32_t m[FM_PGEN_L_N];
    int bad = 0, l;

    (void)n;
    snap_masks(m);
    for (l = 0; l < FM_PGEN_L_N; l++) {
        uint8_t p = SNOWCRASH_LANE_PERIOD[era][l];
        uint32_t pat = m[l] & ((1u << p) - 1u);
        int c = popc(pat);

        bad += snowcrash_gen_tile(pat, p) != m[l];
        bad += c < SNOWCRASH_GEN_HITS[era][l][0] || c > SNOWCRASH_GEN_HITS[era][l][1];
    }
    return bad;
}
