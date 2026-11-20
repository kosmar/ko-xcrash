/* KO HOUSE musical rules beyond the genome must/ban masks; included by tests/plate_gen_drift.c. */
static int pgen_rules_check(uint8_t era, int32_t n)
{
    const dub_plate_t *p = &G.pl;
    int bad = 0;

    (void)n;
    bad += popc(p->sub) < 2;                     /* a bassline exists */
    bad += popc(p->chord) + popc(p->siren) > 10; /* chords/stabs punctuate, no wall */
    bad += popc(p->kick) > (era == ERA_GARAGE ? 6 : 7);
    bad += (p->rim & p->kick & 0x0101u) != 0u;   /* clap never doubles the downbeat kick */
    return bad;
}
