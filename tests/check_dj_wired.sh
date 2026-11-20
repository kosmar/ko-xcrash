#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Smoke: DJ FX longpress glue must stay in fm_app.c (do not git checkout fm_app.c blindly).
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
APP="$ROOT/firmware/src/fm_app.c"

fail() {
  echo "check_dj_wired: $1" >&2
  exit 1
}

[ -f "$APP" ] || fail "missing fm_app.c"

rg -q 'fm_dj_app_phrase_step' "$APP" || fail "fm_dj_app_phrase_step not called — DJ phrase step unwired"
rg -q 'fm_dj_app_fx_long_press_poll' "$APP" || fail "fm_dj_app_fx_long_press_poll not called — FX longpress unwired"
rg -q '#include "fm_dj_app.h"' "$APP" || fail "fm_dj_app.h not included"
rg -q 'fm_dj_t dj' "$APP" || fail "g.dj state missing"
rg -q 'fm_dj_break_seq_thin' "$APP" || fail "fm_dj_break_seq_thin not used — WASH break seq gate unwired"
rg -q 'fm_dj_is_wash_climb_bar' "$ROOT/firmware/src/fm_dj.h" || fail "fm_dj_is_wash_climb_bar missing"
rg -q 'FM_DJ_BUILD_ISO_TANK' "$ROOT/firmware/src/fm_dj.h" || fail "FM_DJ_BUILD_ISO_TANK missing"
rg -q 'fm_dj_is_iso_tank_climb_bar' "$ROOT/firmware/src/fm_dj.h" || fail "fm_dj_is_iso_tank_climb_bar missing"
rg -q 'fm_dj_layout_phrase' "$ROOT/firmware/src/fm_dj.h" || fail "variable phrase layout missing"
rg -q 'FM_DJ_TRANS_CROSSFADE' "$ROOT/firmware/src/fm_dj.h" || fail "transition planner types missing"
rg -q 'fm_dj_on_trans' "$APP" || fail "fm_dj_on_trans not wired"
rg -q 'fm_dj_fx_apply' "$APP" || fail "fm_dj_fx_apply not wired"
rg -q 'lfo_slots_eff' "$APP" || fail "DJ LFO effective slots missing"
rg -q 'dub_dj_gest_on_bar' "$APP" || fail "dub_dj_gest_on_bar not wired"

PRED="$ROOT/tests/dj_wash_predicate.c"
HOST_BIN="$ROOT/build/host/dj_wash_predicate"
mkdir -p "$ROOT/build/host"
cc -O2 -Wall -Wextra -I"$ROOT/firmware/src" -o "$HOST_BIN" "$PRED" || fail "dj_wash_predicate compile failed"
"$HOST_BIN" || fail "dj_wash_predicate failed"

TOAST_AUD="$ROOT/tests/dj_toast_audible_predicate.c"
TOAST_AUD_BIN="$ROOT/build/host/dj_toast_audible_predicate"
cc -O2 -Wall -Wextra -I"$ROOT/firmware/src" -o "$TOAST_AUD_BIN" "$TOAST_AUD" || fail "dj_toast_audible_predicate compile failed"
"$TOAST_AUD_BIN" || fail "dj_toast_audible_predicate failed"

COMBO_OV="$ROOT/tests/dj_seq_combo_overlay.c"
COMBO_OV_BIN="$ROOT/build/host/dj_seq_combo_overlay"
cc -O2 -Wall -Wextra -I"$ROOT/firmware/src" -o "$COMBO_OV_BIN" "$COMBO_OV" || fail "dj_seq_combo_overlay compile failed"
"$COMBO_OV_BIN" || fail "dj_seq_combo_overlay failed"

SEQ_FX_PRED="$ROOT/tests/dj_seq_fx_predicate.c"
SEQ_FX_BIN="$ROOT/build/host/dj_seq_fx_predicate"
cc -O2 -Wall -Wextra -I"$ROOT/firmware/src" -o "$SEQ_FX_BIN" "$SEQ_FX_PRED" || fail "dj_seq_fx_predicate compile failed"
"$SEQ_FX_BIN" || fail "dj_seq_fx_predicate failed"

SEQ_FX_SPAWN="$ROOT/tests/dj_seq_fx_spawn.c"
SEQ_FX_SPAWN_BIN="$ROOT/build/host/dj_seq_fx_spawn"
cc -O2 -Wall -Wextra -I"$ROOT/firmware/src" -o "$SEQ_FX_SPAWN_BIN" "$SEQ_FX_SPAWN" || fail "dj_seq_fx_spawn compile failed"
"$SEQ_FX_SPAWN_BIN" || fail "dj_seq_fx_spawn failed"

SEQ_FX_LABELS="$ROOT/tests/dj_seq_fx_labels.c"
SEQ_FX_LABELS_BIN="$ROOT/build/host/dj_seq_fx_labels"
cc -O2 -Wall -Wextra -I"$ROOT/firmware/src" -o "$SEQ_FX_LABELS_BIN" "$SEQ_FX_LABELS" \
  -DEXPECT_ENS='"SYSTEM"' -DEXPECT_FLIP='"INVERT"' || fail "dj_seq_fx_labels dub compile failed"
"$SEQ_FX_LABELS_BIN" || fail "dj_seq_fx_labels dub failed"
cc -O2 -Wall -Wextra -I"$ROOT/firmware/src" -o "$SEQ_FX_LABELS_BIN" "$SEQ_FX_LABELS" \
  -DGENRE_POP -DEXPECT_ENS='"TRK"' -DEXPECT_FLIP='"INVERT"' || fail "dj_seq_fx_labels pop compile failed"
"$SEQ_FX_LABELS_BIN" || fail "dj_seq_fx_labels pop failed"
cc -O2 -Wall -Wextra -I"$ROOT/firmware/src" -o "$SEQ_FX_LABELS_BIN" "$SEQ_FX_LABELS" \
  -DGENRE_HOUSE -DEXPECT_ENS='"CREW"' -DEXPECT_FLIP='"INVERT"' || fail "dj_seq_fx_labels house compile failed"
"$SEQ_FX_LABELS_BIN" || fail "dj_seq_fx_labels house failed"
cc -O2 -Wall -Wextra -I"$ROOT/firmware/src" -o "$SEQ_FX_LABELS_BIN" "$SEQ_FX_LABELS" \
  -DGENRE_JAZZ -DEXPECT_ENS='"COMBO"' -DEXPECT_FLIP='"INVERT"' || fail "dj_seq_fx_labels jazz compile failed"
"$SEQ_FX_LABELS_BIN" || fail "dj_seq_fx_labels jazz failed"
cc -O2 -Wall -Wextra -I"$ROOT/firmware/src" -o "$SEQ_FX_LABELS_BIN" "$SEQ_FX_LABELS" \
  -DGENRE_SOUL -DEXPECT_ENS='"REVUE"' -DEXPECT_FLIP='"INVERT"' || fail "dj_seq_fx_labels soul compile failed"
"$SEQ_FX_LABELS_BIN" || fail "dj_seq_fx_labels soul failed"

JAZZ_PRED="$ROOT/tests/dj_jazz_predicate.c"
JAZZ_BIN="$ROOT/build/host/dj_jazz_predicate"
cc -O2 -Wall -Wextra -I"$ROOT/firmware/src" -DGENRE_JAZZ -o "$JAZZ_BIN" "$JAZZ_PRED" || fail "dj_jazz_predicate compile failed"
"$JAZZ_BIN" || fail "dj_jazz_predicate failed"

PHRASE_PRED="$ROOT/tests/dj_phrase_interval.c"
PHRASE_BIN="$ROOT/build/host/dj_phrase_interval"
cc -O2 -Wall -Wextra -I"$ROOT/firmware/src" -o "$PHRASE_BIN" "$PHRASE_PRED" || fail "dj_phrase_interval compile failed"
"$PHRASE_BIN" || fail "dj_phrase_interval failed"

TRANS_PLAN="$ROOT/tests/dj_trans_plan.c"
TRANS_BIN="$ROOT/build/host/dj_trans_plan"
cc -O2 -Wall -Wextra -I"$ROOT/firmware/src" -o "$TRANS_BIN" "$TRANS_PLAN" || fail "dj_trans_plan compile failed"
"$TRANS_BIN" || fail "dj_trans_plan failed"
cc -O2 -Wall -Wextra -I"$ROOT/firmware/src" -DGENRE_TECHNO -o "$TRANS_BIN" "$TRANS_PLAN" || fail "dj_trans_plan techno compile failed"
"$TRANS_BIN" || fail "dj_trans_plan techno failed"
cc -O2 -Wall -Wextra -I"$ROOT/firmware/src" -DGENRE_POP -o "$TRANS_BIN" "$TRANS_PLAN" || fail "dj_trans_plan pop compile failed"
"$TRANS_BIN" || fail "dj_trans_plan pop failed"

LFO_LC="$ROOT/tests/dj_lfo_lifecycle.c"
LFO_BIN="$ROOT/build/host/dj_lfo_lifecycle"
cc -O2 -Wall -Wextra -I"$ROOT/firmware/src" -o "$LFO_BIN" "$LFO_LC" || fail "dj_lfo_lifecycle compile failed"
"$LFO_BIN" || fail "dj_lfo_lifecycle failed"

GEST_SC="$ROOT/tests/dj_gest_scheduler.c"
GEST_BIN="$ROOT/build/host/dj_gest_scheduler"
cc -O2 -Wall -Wextra -I"$ROOT/firmware/src" -o "$GEST_BIN" "$GEST_SC" || fail "dj_gest_scheduler compile failed"
"$GEST_BIN" || fail "dj_gest_scheduler failed"

FLAM_POL="$ROOT/tests/dj_gest_flam_policy.c"
FLAM_POL_BIN="$ROOT/build/host/dj_gest_flam_policy"
cc -O2 -Wall -Wextra -I"$ROOT/firmware/src" -o "$FLAM_POL_BIN" "$FLAM_POL" || fail "dj_gest_flam_policy compile failed"
"$FLAM_POL_BIN" || fail "dj_gest_flam_policy failed"
cc -O2 -Wall -Wextra -I"$ROOT/firmware/src" -DGENRE_HOUSE -o "$FLAM_POL_BIN" "$FLAM_POL" || fail "dj_gest_flam_policy house compile failed"
"$FLAM_POL_BIN" || fail "dj_gest_flam_policy house failed"

TOAST_EP="$ROOT/tests/dj_toast_ephemeral_policy.c"
TOAST_EP_BIN="$ROOT/build/host/dj_toast_ephemeral_policy"
cc -O2 -Wall -Wextra -I"$ROOT/firmware/src" -o "$TOAST_EP_BIN" "$TOAST_EP" || fail "dj_toast_ephemeral_policy compile failed"
"$TOAST_EP_BIN" || fail "dj_toast_ephemeral_policy failed"

LFO_SC="$ROOT/tests/dj_lfo_scheduler.c"
LFO_SC_BIN="$ROOT/build/host/dj_lfo_scheduler"
cc -O2 -Wall -Wextra -I"$ROOT/firmware/src" -o "$LFO_SC_BIN" "$LFO_SC" || fail "dj_lfo_scheduler compile failed"
"$LFO_SC_BIN" || fail "dj_lfo_scheduler failed"

DROP_SIL="$ROOT/tests/dj_drop_sil_lane.c"
DROP_SIL_BIN="$ROOT/build/host/dj_drop_sil_lane"
cc -O2 -Wall -Wextra -I"$ROOT/firmware/src" -o "$DROP_SIL_BIN" "$DROP_SIL" || fail "dj_drop_sil_lane compile failed"
"$DROP_SIL_BIN" || fail "dj_drop_sil_lane dub failed"
cc -O2 -Wall -Wextra -I"$ROOT/firmware/src" -DGENRE_POP -o "$DROP_SIL_BIN" "$DROP_SIL" || fail "dj_drop_sil_lane pop compile failed"
"$DROP_SIL_BIN" || fail "dj_drop_sil_lane pop failed"
cc -O2 -Wall -Wextra -I"$ROOT/firmware/src" -DGENRE_JAZZ -o "$DROP_SIL_BIN" "$DROP_SIL" || fail "dj_drop_sil_lane jazz compile failed"
"$DROP_SIL_BIN" || fail "dj_drop_sil_lane jazz failed"
cc -O2 -Wall -Wextra -I"$ROOT/firmware/src" -DGENRE_HOUSE -o "$DROP_SIL_BIN" "$DROP_SIL" || fail "dj_drop_sil_lane house compile failed"
"$DROP_SIL_BIN" || fail "dj_drop_sil_lane house failed"

FX_SC="$ROOT/tests/dj_fx_scene.c"
FX_BIN="$ROOT/build/host/dj_fx_scene"
cc -O2 -Wall -Wextra -I"$ROOT/firmware/src" -o "$FX_BIN" "$FX_SC" || fail "dj_fx_scene compile failed"
"$FX_BIN" || fail "dj_fx_scene failed"

GEST_LC="$ROOT/tests/dj_gest_lifecycle.c"
GEST_LC_BIN="$ROOT/build/host/dj_gest_lifecycle"
cc -O2 -Wall -Wextra -I"$ROOT/firmware/src" -o "$GEST_LC_BIN" "$GEST_LC" || fail "dj_gest_lifecycle compile failed"
"$GEST_LC_BIN" || fail "dj_gest_lifecycle dub failed"
cc -O2 -Wall -Wextra -I"$ROOT/firmware/src" -DGENRE_JAZZ -o "$GEST_LC_BIN" "$GEST_LC" || fail "dj_gest_lifecycle jazz compile failed"
"$GEST_LC_BIN" || fail "dj_gest_lifecycle jazz failed"

ISO_TANK_PRED="$ROOT/tests/dj_iso_tank_predicate.c"
ISO_TANK_BIN="$ROOT/build/host/dj_iso_tank_predicate"
cc -O2 -Wall -Wextra -I"$ROOT/firmware/src" -o "$ISO_TANK_BIN" "$ISO_TANK_PRED" || fail "dj_iso_tank_predicate compile failed"
"$ISO_TANK_BIN" || fail "dj_iso_tank_predicate failed"

ISO_LC="$ROOT/tests/dj_iso_lifecycle.c"
ISO_LC_BIN="$ROOT/build/host/dj_iso_lifecycle"
cc -O2 -Wall -Wextra -I"$ROOT/firmware/src" -o "$ISO_LC_BIN" "$ISO_LC" || fail "dj_iso_lifecycle compile failed"
"$ISO_LC_BIN" || fail "dj_iso_lifecycle failed"

TOAST_BAR="$ROOT/tests/dj_toast_bar_spawn_stability.c"
TOAST_BAR_BIN="$ROOT/build/host/dj_toast_bar_spawn_stability"
cc -O2 -Wall -Wextra -I"$ROOT/firmware/src" -o "$TOAST_BAR_BIN" "$TOAST_BAR" || fail "dj_toast_bar_spawn_stability compile failed"
"$TOAST_BAR_BIN" || fail "dj_toast_bar_spawn_stability failed"

TOAST_ARP_POL="$ROOT/tests/dj_toast_arp_policy.c"
TOAST_ARP_POL_BIN="$ROOT/build/host/dj_toast_arp_policy"
cc -O2 -Wall -Wextra -I"$ROOT/firmware/src" -o "$TOAST_ARP_POL_BIN" "$TOAST_ARP_POL" || fail "dj_toast_arp_policy compile failed"
"$TOAST_ARP_POL_BIN" || fail "dj_toast_arp_policy failed"

TOAST_BADGE="$ROOT/tests/dj_toast_badge_layout.c"
TOAST_BADGE_BIN="$ROOT/build/host/dj_toast_badge_layout"
cc -O2 -Wall -Wextra -I"$ROOT/firmware/src" -I"$ROOT/firmware/gen" -o "$TOAST_BADGE_BIN" "$TOAST_BADGE" || fail "dj_toast_badge_layout compile failed"
"$TOAST_BADGE_BIN" || fail "dj_toast_badge_layout failed"

TOAST_INK="$ROOT/tests/dj_toast_ink_margin.c"
TOAST_INK_BIN="$ROOT/build/host/dj_toast_ink_margin"
cc -O2 -Wall -Wextra -I"$ROOT/firmware/src" -I"$ROOT/firmware/gen" -o "$TOAST_INK_BIN" "$TOAST_INK" || fail "dj_toast_ink_margin compile failed"
"$TOAST_INK_BIN" || fail "dj_toast_ink_margin failed"

TOAST_FADE_BG="$ROOT/tests/toast_fade_bg.c"
TOAST_FADE_BG_BIN="$ROOT/build/host/toast_fade_bg"
cc -O2 -Wall -Wextra -I"$ROOT/firmware/src" -I"$ROOT/firmware/gen" -o "$TOAST_FADE_BG_BIN" "$TOAST_FADE_BG" || fail "toast_fade_bg compile failed"
"$TOAST_FADE_BG_BIN" || fail "toast_fade_bg failed"

GEST_ARP_LC="$ROOT/tests/dj_gest_arp_lifecycle.c"
GEST_ARP_LC_BIN="$ROOT/build/host/dj_gest_arp_lifecycle"
cc -O2 -Wall -Wextra -I"$ROOT/firmware/src" -o "$GEST_ARP_LC_BIN" "$GEST_ARP_LC" || fail "dj_gest_arp_lifecycle compile failed"
"$GEST_ARP_LC_BIN" || fail "dj_gest_arp_lifecycle failed"

COMBO_GUARD="$ROOT/tests/dj_combo_mask_guard.c"
COMBO_GUARD_BIN="$ROOT/build/host/dj_combo_mask_guard"
cc -O2 -Wall -Wextra -I"$ROOT/firmware/src" -o "$COMBO_GUARD_BIN" "$COMBO_GUARD" || fail "dj_combo_mask_guard dub compile failed"
"$COMBO_GUARD_BIN" || fail "dj_combo_mask_guard dub failed"
cc -O2 -Wall -Wextra -I"$ROOT/firmware/src" -DGENRE_HOUSE -o "$COMBO_GUARD_BIN" "$COMBO_GUARD" || fail "dj_combo_mask_guard house compile failed"
"$COMBO_GUARD_BIN" || fail "dj_combo_mask_guard house failed"

TOAST_STAB="$ROOT/tests/toast_stability.c"
TOAST_STAB_BIN="$ROOT/build/host/toast_stability"
cc -O2 -Wall -Wextra -I"$ROOT/firmware/src" -o "$TOAST_STAB_BIN" "$TOAST_STAB" || fail "toast_stability compile failed"
"$TOAST_STAB_BIN" || fail "toast_stability failed"

TOAST_LANE_Y="$ROOT/tests/toast_lane_y.c"
TOAST_LANE_Y_BIN="$ROOT/build/host/toast_lane_y"
cc -O2 -Wall -Wextra -I"$ROOT/firmware/src" -o "$TOAST_LANE_Y_BIN" "$TOAST_LANE_Y" || fail "toast_lane_y compile failed"
"$TOAST_LANE_Y_BIN" || fail "toast_lane_y failed"

TOAST_PLAY_HIDE="$ROOT/tests/dj_toast_play_hide.c"
TOAST_PLAY_HIDE_BIN="$ROOT/build/host/dj_toast_play_hide"
cc -O2 -Wall -Wextra -I"$ROOT/firmware/src" -o "$TOAST_PLAY_HIDE_BIN" "$TOAST_PLAY_HIDE" || fail "dj_toast_play_hide compile failed"
"$TOAST_PLAY_HIDE_BIN" || fail "dj_toast_play_hide failed"

TIME_GUARD="$ROOT/tests/dj_time_div_guard.c"
TIME_GUARD_BIN="$ROOT/build/host/dj_time_div_guard"
cc -O2 -Wall -Wextra -I"$ROOT/firmware/src" -o "$TIME_GUARD_BIN" "$TIME_GUARD" || fail "dj_time_div_guard compile failed"
"$TIME_GUARD_BIN" || fail "dj_time_div_guard failed"

LOOP_POL="$ROOT/tests/dj_toast_loop_policy.c"
LOOP_POL_BIN="$ROOT/build/host/dj_toast_loop_policy"
cc -O2 -Wall -Wextra -I"$ROOT/firmware/src" -o "$LOOP_POL_BIN" "$LOOP_POL" || fail "dj_toast_loop_policy compile failed"
"$LOOP_POL_BIN" || fail "dj_toast_loop_policy failed"

LOOP_HOUSE="$ROOT/tests/dj_gest_loop_house.c"
LOOP_HOUSE_BIN="$ROOT/build/host/dj_gest_loop_house"
cc -O2 -Wall -Wextra -I"$ROOT/firmware/src" -o "$LOOP_HOUSE_BIN" "$LOOP_HOUSE" || fail "dj_gest_loop_house dub compile failed"
"$LOOP_HOUSE_BIN" || fail "dj_gest_loop_house dub failed"
cc -O2 -Wall -Wextra -I"$ROOT/firmware/src" -DGENRE_HOUSE -o "$LOOP_HOUSE_BIN" "$LOOP_HOUSE" || fail "dj_gest_loop_house house compile failed"
"$LOOP_HOUSE_BIN" || fail "dj_gest_loop_house house failed"

rg -q 'perf_toast_strip_dj_arp_stale' "$APP" || fail "DJ ARP toast strip unwired"
rg -q 'dub_dj_gest_arp_toast_live' "$APP" || fail "DJ ARP overlay toast unwired"

rg -q 'FM_DJ_TGT_ENSEMBLE' "$ROOT/firmware/src/fm_dj.h" || fail "FM_DJ_TGT_ENSEMBLE missing"
rg -q 'dub_apply_dj_seq_overlay' "$APP" || fail "dub_apply_dj_seq_overlay not wired"
rg -q 'fm_dj_delta_for_tgt_lane' "$APP" || fail "DJ SEQ horsemen per-lane delta unwired"
rg -q 'dub_eff_vel_rng' "$APP" || fail "dub_eff_vel_rng missing"
rg -q 'dub_eff_trig_raw' "$APP" || fail "dub_eff_trig_raw missing"
rg -q 'dub_eff_gate_rng' "$APP" || fail "dub_eff_gate_rng missing"
rg -q 'dub_eff_pitch_rng' "$APP" || fail "dub_eff_pitch_rng missing"

SEQ_PARAM="$ROOT/tests/dj_seq_param_lane.c"
SEQ_PARAM_BIN="$ROOT/build/host/dj_seq_param_lane"
cc -O2 -Wall -Wextra -I"$ROOT/firmware/src" -o "$SEQ_PARAM_BIN" "$SEQ_PARAM" || fail "dj_seq_param_lane compile failed"
"$SEQ_PARAM_BIN" || fail "dj_seq_param_lane failed"

echo "check_dj_wired: OK"
