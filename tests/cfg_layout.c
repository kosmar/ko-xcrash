/* SPDX-License-Identifier: GPL-3.0-only */
/* Prints sizes of saved structs; included app chosen by -DLAYOUT_APP_* */
#define main cfg_layout_host_main
#if defined(LAYOUT_APP_RYTM)
#include "rytm.c"
#elif defined(LAYOUT_APP_CTRL)
#include "ctrl.c"
#elif defined(LAYOUT_APP_DECK)
#include "deck.c"
#else
#include "fm_app.c"
#endif
#undef main
int main(void)
{
#if defined(LAYOUT_APP_RYTM)
    printf("rytm_cfg_t %u\n", (unsigned)sizeof(rytm_cfg_t));
#elif defined(LAYOUT_APP_CTRL)
    printf("ctrl_cfg_t %u\n", (unsigned)sizeof(ctrl_cfg_t));
#elif defined(LAYOUT_APP_DECK)
    printf("deck_meta_t %u\n", (unsigned)sizeof(deck_meta_t));
    printf("deck_bank_t %u\n", (unsigned)sizeof(deck_bank_t));
    printf("deck_lane_t %u\n", (unsigned)sizeof(deck_lane_t));
#else
    printf("dub_cfg_t %u\n", (unsigned)sizeof(dub_cfg_t));
    printf("dub_params_t %u\n", (unsigned)sizeof(dub_params_t));
    printf("fm_lfo_slot_t %u\n", (unsigned)sizeof(fm_lfo_slot_t));
#endif
    return 0;
}
