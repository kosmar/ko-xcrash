/* KO JAZZ musical rules beyond the genome must/ban masks; included by tests/plate_gen_drift.c. */
static int pgen_rules_check(uint8_t era, int32_t n)
{
    const dub_plate_t *p = &G.pl;
    const fm_pgen_genome_t *ge = &D->genome[era];
    int bad = 0, s;

    (void)n;
    bad += popc(p->sub) < 2;                                  /* the bass walks */
    bad += popc(p->kick) > popc(ge->must[FM_PGEN_L_KICK]) + 4; /* feathered, never a four-on-the-floor wall */
    bad += popc(p->chord) > popc(ge->must[FM_PGEN_L_CHORD]) + 8; /* comping leaves air */
    bad += popc(G.lead_gate) < 1 || popc(G.lead_gate) > 16;   /* a phrase, not a sequencer run */
    for (s = 0; s < 32; s++)                                  /* 32-step lead stays in the plate's scale */
        bad += !jazz_gen_pitch_legal(FM_PGEN_LEAD_STATE.pitch[s], (uint8_t)G.scale);
    return bad;
}
