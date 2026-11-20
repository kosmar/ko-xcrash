/* SPDX-License-Identifier: GPL-3.0-only */
/* KO RYTM: app_audio must never write g.cfg (LFO mutes, auto pair). */
#include <stdio.h>
#include <string.h>
#define main rytm_host_main
#include "rytm.c"
#undef main
int main(void)
{
    static int32_t buf[256 * 2];
    static rytm_cfg_t before;
    uint32_t i;
    app_init();
    g.cfg.lfo.depth[LFO_TGT_ENSEMBL] = 100u;
    g.cfg.lfo.depth[LFO_TGT_SEQ_FLIP] = 100u;
    g.cfg.drummer_auto = 1u;
    g.cfg.bassist_auto = 1u;
    g.cfg.lfo.depth[LFO_TGT_GENRE] = 127u;
    g.playing = 1u;
    before = g.cfg;
    for (i = 0; i < 4000u; i++)
        app_audio(buf, 256u);
    if (memcmp(&before, &g.cfg, sizeof before) != 0) {
        printf("rytm_cfg_purity: FAIL g.cfg changed in audio path\n");
        return 1;
    }
    printf("rytm_cfg_purity: OK\n");
    return 0;
}
