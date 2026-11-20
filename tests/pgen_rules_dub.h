/* KO DUB musical rules beyond the genome must/ban masks; included by tests/plate_gen_drift.c. */
static int pgen_rules_check(uint8_t era, int32_t n)
{
    const dub_plate_t *p = &G.pl;
    int bad = 0;

    (void)n;
    bad += popc(p->sub) < 2;                       /* a bassline exists */
    bad += popc(p->kick) > 6;                      /* dub kick leaves space */
    bad += popc(p->siren) > 3;                     /* siren stays rare */
    bad += popc(p->chord) > 8;                     /* skank punctuates, never half the bar */
    bad += era == ERA_CROYDON && (p->kick & p->rim & 0x0100u) != 0u; /* half-time snare never doubled by kick */
    return bad;
}
