/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef GENRE_CONF_H
#define GENRE_CONF_H

#ifdef GENRE_HEAVY_PSYCH
#include "heavy_psych_conf.h"
#elif defined(GENRE_HOUSE)
#include "house_conf.h"
#elif defined(GENRE_JAZZ)
#include "jazz_conf.h"
#elif defined(GENRE_IDM)
#include "idm_conf.h"
#elif defined(GENRE_HIPHOP)
#include "hiphop_conf.h"
#elif defined(GENRE_SNOWCRASH)
#include "snowcrash_conf.h"
#elif defined(GENRE_FUNK)
#include "funk_conf.h"
#elif defined(GENRE_POP)
#include "pop_conf.h"
#elif defined(GENRE_TECHNO)
#include "techno_conf.h"
#elif defined(GENRE_TRIPHOP)
#include "triphop_conf.h"
#elif defined(GENRE_SOUL)
#include "soul_conf.h"
#elif defined(GENRE_BASS)
#include "bass_conf.h"
#else
#include "dub_conf.h"
#endif

#ifndef GENRE_CFG_USER_PLATE_N
#define GENRE_CFG_USER_PLATE_N GENRE_PRESET_N
#endif

#endif
