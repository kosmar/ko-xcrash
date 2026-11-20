/* SPDX-License-Identifier: GPL-3.0-only */
/* KO BASS: HOME macros / PERFORM knobs are not stored in the cfg blob. After a boot from a saved
 * blob every one of them must sit at 64 (neutral), like after a factory boot.
 * Build: cc -DDINGHY_HOST -DGENRE_BASS -Ifirmware/src -Ifirmware/gen tests/bass_macro_boot.c */
#define main bass_macro_boot_host_main
#include "fm_app.c"
#undef main

static int check_macros(const char *when)
{
    uint8_t i;
    int bad = 0;

    for (i = 0u; i < 8u; i++) {
        uint8_t *mp = dub_macro_ptr(i);

        if (!mp || *mp != 64u) {
            fprintf(stderr, "bass_macro_boot: FAIL %s macro %u = %u (want 64)\n", when, (unsigned)i,
                    mp ? (unsigned)*mp : 0u);
            bad = 1;
        }
    }
    return bad;
}

int main(void)
{
    int fail = 0;

    app_init();
    fail |= check_macros("factory boot");
    if (settings_save(&g.cfg, sizeof g.cfg) != 0) {
        fprintf(stderr, "bass_macro_boot: FAIL save\n");
        return 1;
    }
    memset(&g, 0, sizeof g);
    app_init();
    if (g.cfg.magic != DUB_CFG_MAGIC) {
        fprintf(stderr, "bass_macro_boot: FAIL saved blob not loaded\n");
        return 1;
    }
    fail |= check_macros("saved boot");
    if (!fail)
        printf("bass_macro_boot: OK\n");
    return fail;
}
