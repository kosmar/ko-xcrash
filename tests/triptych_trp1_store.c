/* SPDX-License-Identifier: GPL-3.0-only */
/* Write a TRP1 host settings store for P2 migration tests. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "midi_ctrl.h"

int main(int argc, char **argv)
{
    midi_cfg_t tmp;
    midi_cfg_v1_t c;
    uint32_t n;
    uint8_t hdr[4] = {1, 0, 0, 0};
    const char *path;
    FILE *f;

    if (argc < 2) {
        fprintf(stderr, "usage: %s OUT.bin\n", argv[0]);
        return 1;
    }
    path = argv[1];
    midi_cfg_defaults(&tmp);
    memset(&c, 0, sizeof c);
    memcpy(&c.magic, &tmp.magic, sizeof c);
    c.magic = TRYP_CFG_MAGIC_V1;
    c.mode = 0u;
    c.base_val[P_TIMBRE] = 55u;
    c.val[P_TIMBRE] = 55u;

    n = (uint32_t)sizeof c;
    f = fopen(path, "wb");
    if (!f) {
        perror(path);
        return 1;
    }
    if (fwrite(&n, sizeof n, 1, f) != 1 || fwrite(hdr, 1, 4, f) != 4 ||
        fwrite(&c, 1, sizeof c, f) != sizeof c) {
        fclose(f);
        fprintf(stderr, "%s: write failed\n", path);
        return 1;
    }
    fclose(f);
    return 0;
}
