/* KO HIPHOP musical rules beyond the genome must/ban masks; included by tests/plate_gen_drift.c. */
static int pgen_rules_check(uint8_t era, int32_t n)
{
    const dub_plate_t *p = &G.pl;
    const fm_pgen_genome_t *ge = &D->genome[era];
    int bad = 0;

    (void)n;
    bad += G.swing < (int8_t)ge->swing_lo || G.swing > (int8_t)ge->swing_hi; /* swing is the era's identity */
    bad += popc(p->kick & ~ge->must[FM_PGEN_L_KICK]) > 6;                    /* the kick leaves room */
    bad += popc(p->sub) < 2 || popc(p->sub) > 12;                            /* a bassline, no wall */
    bad += popc(p->chord) > (era == ERA_CLOUD ? 14 : 10);                    /* chords punctuate */
    bad += popc(p->hat) < 8;                                                 /* the hats keep time */
    return bad;
}
