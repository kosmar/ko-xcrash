/* SPDX-License-Identifier: GPL-3.0-only */
/* Write a host settings file in the v0 layout (no TRP1 magic) for migration roundtrip tests. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "midi_ctrl.h"

int main(int argc, char **argv)
{
    midi_cfg_t c;
    midi_cfg_v0_t o;
    uint32_t n;
    uint8_t hdr[4] = {1, 0, 0, 0};
    const char *path;
    FILE *f;

    if (argc < 2) {
        fprintf(stderr, "usage: %s OUT.bin\n", argv[0]);
        return 1;
    }
    path = argv[1];
    midi_cfg_defaults(&c);
    c.magic = 0u;
    c.mode = 2u;
    c.midi_ch = 5u;
    c.base_val[P_TEMPO] = 99u;
    c.val[P_TEMPO] = 99u;
    c.base_val[P_TIMBRE] = 77u;
    c.val[P_TIMBRE] = 77u;
    memset(c.phrases[0], 0, DITDAH_MAX_LEN);
    memcpy(c.phrases[0], "V0 ROUNDTRIP", 12u);

    memcpy(&o, &c.val, sizeof o);

    n = (uint32_t)sizeof o;
    f = fopen(path, "wb");
    if (!f) {
        perror(path);
        return 1;
    }
    if (fwrite(&n, sizeof n, 1, f) != 1 || fwrite(hdr, 1, 4, f) != 4 ||
        fwrite(&o, 1, sizeof o, f) != sizeof o) {
        fclose(f);
        fprintf(stderr, "%s: write failed\n", path);
        return 1;
    }
    fclose(f);
    return 0;
}
