/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef BASS_CONF_H
#define BASS_CONF_H

#define GENRE_ERA_N      6u
#define GENRE_PRESETS_PER_ERA 10u
/* 60 style lines in bass_data.h; user plates sized for full preset grid */
#define GENRE_PRESET_N   60u
/* Flash budget: dub_cfg_t + bass_cfg_ext must fit SETTINGS_MAX (see fm_app static assert). */
#define GENRE_CFG_USER_PLATE_N 55u
#define GENRE_PRESET_N_LEGACY 48u
#define GENRE_CFG_MAGIC  0x42415331u /* BAS1 */
#define GENRE_APP_NAME   "KO BASS"
#define GENRE_FELUCCA_ID "FM-1_56410"

#define GENRE_PRESET_LABEL "LINE"
#define GENRE_PRESET_HDR   "LIN"

#define GENRE_C_ACC RGB(40, 180, 255)

#define GENRE_BANK_COL \
    { RGB(30, 120, 200), RGB(20, 200, 220), RGB(10, 80, 160), RGB(60, 140, 255), \
      RGB(0, 100, 140), RGB(80, 220, 255), 0, RGB(0, 200, 160) }

#define GENRE_ERA_COL \
    { RGB(255, 60, 120), RGB(0, 180, 255), RGB(120, 40, 220), RGB(40, 220, 200), \
      RGB(200, 80, 255), RGB(0, 255, 200) }

#endif
