/* KO HEAVY_PSYCH musical rules beyond the genome must/ban masks; included by tests/plate_gen_drift.c. */
static int pgen_rules_check(uint8_t era, int32_t n)
{
    const dub_plate_t *p = &G.pl;
    int bad = 0;

    (void)era;
    (void)n;
    bad += popc(p->kick) > 6;                  /* heavy = space */
    bad += popc(G.lead_gate) < 4;              /* a riff exists */
    bad += popc(p->sub) < 1;                   /* a bassline exists */
    bad += (p->rim & p->kick) != 0u;           /* snare never doubles the kick */
    bad += (p->hat & 0x11111111u) != 0x11111111u;
    return bad;
}
