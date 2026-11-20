/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef SOUL_CONF_H
#define SOUL_CONF_H

#define GENRE_ERA_N      6u
#define GENRE_PRESETS_PER_ERA 10u
#define GENRE_PRESET_N   60u
#define GENRE_PRESET_N_LEGACY 36u /* 6 trax/era flash layout */
#define GENRE_CFG_MAGIC  0x534F5531u /* SOU1 */
#define GENRE_APP_NAME   "KO SOUL"
#define GENRE_FELUCCA_ID "FM-1_56414"

#define GENRE_PRESET_LABEL "SIDE"
#define GENRE_PRESET_HDR   "SID"

#define GENRE_C_ACC RGB(235, 160, 60)

#define GENRE_BANK_COL \
    { RGB(180, 220, 255), RGB(255, 180, 90), RGB(120, 200, 160), RGB(200, 140, 255), \
      RGB(255, 120, 160), RGB(140, 180, 255) }

#define GENRE_ERA_COL \
    { RGB(150, 100, 210), RGB(240, 190, 70), RGB(220, 90, 60), RGB(90, 150, 240), \
      RGB(235, 120, 175), RGB(70, 205, 180) }

#endif
