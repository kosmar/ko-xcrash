/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef FUNK_CONF_H
#define FUNK_CONF_H

#define GENRE_ERA_N      6u
#define GENRE_PRESETS_PER_ERA 10u
#define GENRE_PRESET_N   60u
#define GENRE_PRESET_N_LEGACY 36u
#define GENRE_CFG_MAGIC  0x46554E4Cu /* bump: user_song + song_name in cfg */
#define GENRE_APP_NAME   "KO FUNK"
#define GENRE_FELUCCA_ID "FM-1_56408"

#define GENRE_PRESET_LABEL "VAMP"
#define GENRE_PRESET_HDR   "VMP"

#define GENRE_C_ACC RGB(220, 180, 70)

#define GENRE_BANK_COL \
    { RGB(180, 140, 50), RGB(200, 90, 120), RGB(120, 200, 80), RGB(160, 100, 220), \
      RGB(90, 180, 220), RGB(220, 160, 90) }

#define GENRE_ERA_COL \
    { RGB(220, 180, 70), RGB(200, 120, 180), RGB(180, 220, 90), RGB(255, 220, 80), \
      RGB(140, 200, 255), RGB(160, 90, 110) }

#endif
