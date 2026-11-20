/* KO FUNK musical rules beyond the genome must/ban masks; included by tests/plate_gen_drift.c. */
static int pgen_rules_check(uint8_t era, int32_t n)
{
    const dub_plate_t *p = &G.pl;
    uint8_t four = era == ERA_DISCO || era == ERA_BOOGIE;
    int bad = 0;

    (void)n;
    bad += (p->kick & 1u) == 0u;                 /* the one */
    bad += (p->sub & 1u) == 0u;                  /* bass anchored on the one */
    bad += popc(p->sub) < 3;                     /* funk bass is busy */
    bad += (p->rim & 0x1010u) != 0x1010u;        /* backbeat */
    bad += !four && (p->kick & 0x1010u) != 0u;   /* kick never on the backbeat snare (4/4 eras share it) */
    bad += (p->rim & p->kick & 0x0101u) != 0u;   /* snare never doubles the downbeat kick */
    bad += popc(p->kick) > (four ? 6 : 8);
    bad += popc(p->chord) + popc(p->siren) > (four ? 11 : 10); /* stabs punctuate (4/4: three are must) */
    return bad;
}
