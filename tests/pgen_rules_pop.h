/* KO POP musical rules beyond the genome must/ban masks; included by tests/plate_gen_drift.c. */
static int pgen_rules_check(uint8_t era, int32_t n)
{
    const dub_plate_t *p = &G.pl;
    const uint32_t m[6] = { p->kick, p->rim, p->hat, p->sub, p->chord, p->siren };
    int bad = 0, l;

    (void)n;
    for (l = 0; l < 6; l++)
        bad += (m[l] >> 16) != (m[l] & 0xFFFFu);   /* one bar, stored twice like the ROM */
    /* harmony: the shadowed slot's progression/melody/bass tables are written for its key and mode */
    bad += pop_era_of_plate(G.slot) != era;
    bad += G.root != POP_PLATE_ROOT[G.slot] || G.scale != POP_PLATE_SCALE[G.slot];
    bad += pop_plate_root(G.slot) != (uint8_t)POP_PLATE_ROOT[G.slot];
    bad += pop_plate_scale(G.slot) != (uint8_t)POP_PLATE_SCALE[G.slot];
    bad += pop_plate(G.slot) != p;
    for (l = 0; l < 6; l++)
        bad += popc(m[l] & 0xFFFFu) > POP_GEN_CAP[era][l];
    bad += popc(p->kick & 0xFFFFu) > (era == ERA_BELL || era == ERA_STAB ? 7 : era == ERA_HORN ? 6 : 4);
    bad += popc(p->hat & 0xFFFFu) < 4;
    bad += popc(p->sub & 0xFFFFu) < 2;              /* a bassline exists */
    bad += popc(p->chord & 0xFFFFu) < 4;            /* chord on every beat */
    bad += popc(p->siren & 0xFFFFu) < 1;            /* the lead has a trigger */
    return bad;
}
