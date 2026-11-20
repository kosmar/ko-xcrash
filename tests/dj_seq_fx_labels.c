/* SPDX-License-Identifier: GPL-3.0-only */
/* Host: per-genre SEQ pg4 labels for DJ toast (compile with -DGENRE_*). */
#include "fm_dj_seq.h"
#include <stdio.h>
#include <string.h>

#ifndef EXPECT_ENS
#define EXPECT_ENS FM_SEQ_KNOB_P4[2]
#endif
#ifndef EXPECT_FLIP
#define EXPECT_FLIP "INVERT"
#endif

static int fail;

static void check(int cond, const char *msg)
{
    if (!cond) {
        fprintf(stderr, "dj_seq_fx_labels: %s\n", msg);
        fail = 1;
    }
}

int main(void)
{
    fail = 0;
    check(strcmp(fm_dj_seq_ens_toast_label(), EXPECT_ENS) == 0, "ensemble label");
    check(strcmp(fm_dj_seq_flip_toast_label(), EXPECT_FLIP) == 0, "invert label");
    return fail ? 1 : 0;
}
