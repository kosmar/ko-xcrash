/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef TECHNO_CONF_H
#define TECHNO_CONF_H

#define GENRE_ERA_N      6u
#define GENRE_PRESETS_PER_ERA 10u
#define GENRE_PRESET_N   60u
#define GENRE_CFG_USER_PLATE_N 60u
#define GENRE_PRESET_N_LEGACY 48u
#define GENRE_CFG_MAGIC  0x54454331u /* TEC1 */
#define GENRE_APP_NAME   "KO TECHNO"
#define GENRE_FELUCCA_ID "FM-1_56409"

#define GENRE_PRESET_LABEL "LOOP"
#define GENRE_PRESET_HDR   "LOP"

#define GENRE_C_ACC RGB(80, 220, 255)

#define GENRE_BANK_COL \
    { RGB(60, 200, 230), RGB(255, 220, 60), RGB(255, 60, 60), RGB(180, 80, 255), \
      RGB(120, 160, 200), RGB(80, 255, 100) }

#define GENRE_ERA_COL \
    { RGB(0, 220, 255), RGB(255, 230, 40), RGB(255, 40, 50), RGB(160, 60, 255), \
      RGB(100, 150, 210), RGB(60, 255, 80) }

#endif
