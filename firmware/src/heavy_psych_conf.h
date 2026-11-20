/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef HEAVY_PSYCH_CONF_H
#define HEAVY_PSYCH_CONF_H

#define GENRE_ERA_N      6u
#define GENRE_PRESETS_PER_ERA 10u
#define GENRE_PRESET_N   60u
#define GENRE_PRESET_N_LEGACY 30u
#define GENRE_CFG_MAGIC  0x48565941u /* bump: user_song + song_name in cfg */
#define GENRE_APP_NAME   "KO HVYPSY"
#define GENRE_FELUCCA_ID "FM-1_56403"

#define GENRE_PRESET_LABEL "RIFF"
#define GENRE_PRESET_HDR   "RFF"

#define GENRE_C_ACC RGB(255, 170, 72)

#define GENRE_BANK_COL \
    { RGB(255, 170, 72), RGB(180, 95, 58), RGB(120, 140, 70), RGB(140, 100, 210), \
      RGB(110, 150, 190), RGB(70, 55, 48) }

#define GENRE_ERA_COL \
    { RGB(255, 50, 45), RGB(255, 130, 25), RGB(255, 220, 35), RGB(45, 215, 65), \
      RGB(35, 200, 255), RGB(210, 55, 255) }

#endif
