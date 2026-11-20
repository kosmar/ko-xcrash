/* IDM musical rules beyond the genome must/ban masks; included by tests/plate_gen_drift.c. */
static int pgen_rules_check(uint8_t era, int32_t n)
{
    const dub_plate_t *p = &G.pl;
    int lead = popc(G.lead_gate);
    int bad = 0;

    (void)n;
    bad += lead < 1;                                         /* a lead phrase exists */
    bad += (era == ERA_WARM || era == ERA_MELO) && lead > 8; /* warm/melodic lead stays sparse */
    if (era == ERA_DRILL) {
        bad += popc(p->hat) < 12;                            /* drill hats are busy */
        bad += popc(p->hat) + popc(p->rim) < 22;
    }
    bad += popc(p->sub) < 2;
    return bad;
}
