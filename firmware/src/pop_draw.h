/* SPDX-License-Identifier: GPL-3.0-only */
/* Included from fm_app.c when GENRE_POP — after dub_app_t g and helpers exist. */
#ifndef POP_DRAW_H
#define POP_DRAW_H

static void draw_view_desk(void)
{
    const dub_params_t *p = &g.voice.p;
    const pop_plate_t *tx = pop_plate(p->plate);
    static const char *const VNAME[4] = {"KICK", "SNAR", "HAT", "LEAD"};
    static const uint8_t PULSE_SP_TAB[9] = {188, 176, 88, 58, 44, 35, 29, 25, 22};
    uint8_t kit = p->drum_kit % DKIT_N;
    uint16_t kcol = ERA_COL[DKIT_ERA[kit] % GENRE_ERA_N];
    uint16_t ecol = ERA_COL[p->era % GENRE_ERA_N];
    uint8_t sel_ix = g.sel_drm % 4u;
    int32_t dly_step = 24 + ((int32_t)dub_dly_div_ix(p->dly_time) * 28) / (int32_t)DUB_DLY_DIV_N;
    uint32_t row, x, b;
    uint32_t beat_w = 47u;

    for (b = 0u; b < 4u; b++) {
        uint32_t bx = 46u + b * beat_w;
        cv_line(bx, 0, bx, 123, C_LINE);
    }
    if (g.playing) {
        uint32_t ph_x = 46u + (((uint32_t)(g.step & 15u) * 188u) / 16u);
        cv_line((int32_t)ph_x, 0, (int32_t)ph_x, 123, C_ACC);
    }

    for (row = gv_desk_row0(); row < gv_desk_row_end(); row++) {
        int32_t dens = row == 3u ? 96 : (int32_t)p->drm_dens[row];
        uint32_t npulses = dens < 8 ? 0u : (uint32_t)(1 + (dens * 7) / 127);
        uint32_t pulse_sp = PULSE_SP_TAB[npulses > 8u ? 8u : npulses];
        int32_t mid = gv_desk_mid(row);
        int muted = !dub_lane_audible((uint8_t)row);
        int solo = dub_lane_is_solo((uint8_t)row);
        uint8_t lvl = row == 0u ? p->kick_boom :
                      row == 1u ? p->rim_tone :
                      row == 2u ? p->hat_lvl : p->siren_pitch;
        int32_t live_env = row == 0u ? (g.synth.kick_env >> 8) :
                           row == 1u ? (g.synth.rim_env >> 8) :
                           row == 2u ? (g.synth.hat_env >> 8) : (g.synth.siren_env >> 8);
        uint8_t pan = p->drm_pan[row] ? p->drm_pan[row] :
                      (row == 0u ? 64u : row == 1u ? 74u : row == 2u ? 52u : 82u);
        int32_t pan_x = 46 + ((int32_t)pan * 184) / 127;
        uint16_t lcol = solo ? C_ACC : (muted || lvl == 0u ? C_DIM :
                                        (row == sel_ix ? C_WHITE : (row == 3u ? C_ACC : kcol)));
        uint16_t base_col = muted || lvl == 0u || (row < 3u && dens == 0) ? C_DIM :
                            (live_env > 32 ? C_WHITE :
                             (row == 0u ? kcol : row == 1u ? ecol : row == 2u ? C_HI : C_ACC));
        uint32_t bpm_v = p->bpm < 40u ? 40u : (p->bpm > 180u ? 180u : p->bpm);
        uint32_t speed = (fm1_ms * (bpm_v / 14u + (uint32_t)(row == 3u ? p->siren_rate : (uint8_t)dens) / 14u)) << 17;
        uint32_t p_cnt = (row == 1u) ? (pulse_sp >> 1) : (row == 2u ? (pulse_sp >> 2) : 0u);
        uint32_t d_cnt = 0u;
        int32_t py = -1;
        int32_t main_env = (row == 3u) ? (120 + (live_env >> 1)) : (dens == 0 ? 0 : (120 + (dens * 100) / 127));
        int32_t echo_env = 0;
        int32_t dly_d = (int32_t)p->drm_dly[row];
        int32_t kt = (int32_t)(p->drm_tune[row] ? p->drm_tune[row] : 64u) - 64;
        int32_t kd = (int32_t)(p->drm_dec[row] ? p->drm_dec[row] : 64u) - 64;
        int32_t kdrv = (int32_t)p->drm_drv[row];
        int32_t dec_drop = 960 - (((int32_t)lvl * 4 + (kd + 64) * 4) * 760) / (127 * 8);
        uint16_t pat = row == 0u ? tx->kick :
                       row == 1u ? tx->rim :
                       row == 2u ? tx->hat : tx->siren;

        if (dec_drop < 110)
            dec_drop = 110;
        if (dec_drop > 1100)
            dec_drop = 1100;

        if (row == sel_ix) {
            int32_t band_y = mid - 13;
            int32_t band_h = 26;

            cv_rect(0, band_y, 240, band_h, RGB(18, 22, 42));
            if (!muted) {
                cv_rect(1, mid - 7, 38, 14, solo ? C_ACC : C_WHITE);
                cv_text(4, mid - 7, &FONT_S, VNAME[row], C_BLACK);
            } else {
                cv_rect(1, mid - 7, 38, 14, C_WHITE);
                cv_rect(3, mid - 5, 34, 10, C_BLACK);
                cv_text(4, mid - 7, &FONT_S, VNAME[row], C_WHITE);
            }
        } else
            cv_text(4, mid - 7, &FONT_S, VNAME[row], lcol);
        cv_line(46, mid, 234, mid, row == sel_ix ? C_DIM : C_LINE);
        cv_line(pan_x, mid - 3, pan_x, mid + 3, row == sel_ix ? C_WHITE : C_DIM);

        for (x = 0u; x < 188u; x++) {
            int32_t xp = 46 + (int32_t)x;
            int32_t w = 0;
            int32_t max_amp = ((4 + ((int32_t)lvl * 9) / 127) + (live_env >> 3));
            int32_t env, amp, dy, y;
            uint16_t col = base_col;
            uint8_t st_ix = (uint8_t)((x * 16u) / 188u);

            if (max_amp > 14)
                max_amp = 14;

            p_cnt++;
            if (p_cnt >= pulse_sp)
                p_cnt = 0u;
            d_cnt++;
            if (d_cnt >= (uint32_t)dly_step)
                d_cnt = 0u;

            if (st_ix < 16u && dub_step_bit(pat, st_ix) && x > 5u) {
                int32_t pat_burst = 150 + (live_env >> 1);
                if (pat_burst > main_env)
                    main_env = pat_burst;
            }

            if (row < 3u) {
                if (npulses > 0u && x > 6u && p_cnt == 0u) {
                    int32_t re_lvl = 120 + (dens * 110) / 127;
                    if (re_lvl > main_env)
                        main_env = re_lvl;
                }
                if (dly_d > 8 && x > 14u && d_cnt == 0u) {
                    int32_t burst = (200 * dly_d) / 127;
                    if (burst > echo_env)
                        echo_env = burst;
                }
                main_env -= (main_env * dec_drop) >> 12;
                if (main_env < (dens >> 2))
                    main_env = (dens >> 2);
                if (echo_env > 0)
                    echo_env -= (echo_env * (dec_drop + 160)) >> 13;
                env = main_env > echo_env ? main_env : echo_env;
                if (echo_env > main_env && !muted)
                    col = echo_env > 110 ? C_ACC : C_HI;
            } else {
                if (dly_d > 12 && x > 16u && d_cnt == 0u)
                    echo_env = (160 * dly_d) / 127;
                if (echo_env > 0)
                    echo_env -= (echo_env * 280) >> 13;
                env = 100 + (live_env >> 2) + (echo_env >> 2);
                if (echo_env > 110 && !muted)
                    col = C_HI;
            }

            if (row == 0u) {
                uint32_t sweep = (uint32_t)(main_env * pop_kit_k_sweep[kit]) << 17;
                uint32_t kph = (x * (uint32_t)(600 + (kt + 64) * 8 + dens * 3) << 15) + sweep + speed;
                int32_t fm = (dub_sine_q64(kph << 1) * (int32_t)pop_kit_k_fm[kit] * env) >> 14;
                w = dub_sine_q64(kph + (uint32_t)(fm << 14));
            } else if (row == 1u) {
                uint32_t rph = (x * (uint32_t)(1200 + (kt + 64) * 14 + dens * 6) << 15) + (speed + (speed >> 1));
                int32_t sh = (dub_sine_q64(rph) + (dub_square(rph + (rph >> 1)) >> 1)) >> 1;
                int32_t wr = (dub_sine_q64((rph << 2) + speed) * (int32_t)pop_kit_r_wire[kit]) >> 7;
                w = sh + (wr >> 1);
            } else if (row == 2u) {
                uint32_t hph = (x * (uint32_t)(1800 + (kt + 64) * 18 + dens * 8) << 15) + (speed << 1);
                int32_t h_met = (dub_sine_q64(hph) * (int32_t)pop_hat_metal[kit]) >> 7;
                int32_t h_nz = (dub_noise(&g.synth.noise) * (int32_t)pop_hat_noise[kit]) >> 9;
                uint8_t hst = pop_hat_style[kit];

                if (hst == HAT_STYLE_HISS)
                    h_met >>= 1;
                else if (hst == HAT_STYLE_TR6)
                    h_met = (h_met >> 6) << 6;
                w = h_met + h_nz + (dub_square(hph + ((uint32_t)pop_hat_fm[kit] << 20)) >> 3);
            } else {
                uint32_t sph = (x * (uint32_t)(900 + (kt + 64) * 10) << 15) + speed;
                w = dub_square(sph) + (dub_sine_q64(sph + (sph >> 1)) >> 1);
                w += dub_square(sph + (sph >> 2)) >> 1;
            }

            if (kdrv > 0 && row < 3u) {
                w = (w * (256 + kdrv * 2)) >> 8;
                if (w > 32767) {
                    w = 65534 - w;
                    if (!muted && kdrv > 40)
                        col = C_ACC;
                } else if (w < -32767) {
                    w = -65534 - w;
                    if (!muted && kdrv > 40)
                        col = C_ACC;
                }
            }

            amp = (max_amp * env) >> 8;
            dy = (w * amp) / 32767;
            if (dy > 12)
                dy = 12;
            else if (dy < -12)
                dy = -12;
            y = mid - dy;
            if (py >= 0 && !muted)
                cv_line(xp - 1, py, xp, y, col);
            py = y;
        }
    }
}

static void draw_view_home(void)
{
    /* DUB HOME engine (6826–7097): 11 perspective rays, 12–22 planes, 40-seg polylines,
     * black occlusion under ridge, side struts — house macro remap in plan §A. */
    const dub_params_t *p = &g.voice.p;
    uint8_t m_jack = dub_eff_macro(0);
    uint8_t m_layer = dub_eff_macro(1);
    uint8_t m_cut = dub_eff_macro(2);
    uint8_t m_iso = dub_eff_macro(3);
    uint8_t m_space = dub_eff_macro(4);
    uint8_t m_pump = dub_eff_macro(5);
    uint8_t m_grit = dub_eff_macro(6);
    uint8_t m_floor = dub_eff_macro(7);
    uint8_t m_motion = dub_eff_macro(8);
    uint8_t m_flux = dub_eff_macro(9);
    int32_t flip_sign = (g.seq_flip_state & 1u) ? -1 : 1;
    uint32_t row, i;
    int32_t iso = ((int32_t)p->isolator - 64) + ((int32_t)m_iso >> 1);
    uint16_t ecol = ERA_COL[p->era % GENRE_ERA_N];
    int32_t duck = (g.synth.pump_env * (int32_t)(p->pump + m_pump)) >> 17;
    int32_t amp_scale = 256 + ((int32_t)m_jack * 360) / 127;
    int32_t cut_n = 48 + ((int32_t)m_cut * 208) / 127;
    int32_t fov_zoom = 92 + ((int32_t)m_space * 52) / 127 + ((int32_t)m_cut * 12) / 127;
    int32_t sub_e  = !dub_lane_heard_audible(4u) ? 0 : ((int32_t)g.synth.meter[0] + (g.synth.sub_env >> 7));
    int32_t chd_e  = !dub_lane_heard_audible(5u) ? 0 : ((int32_t)g.synth.meter[1] + (g.synth.ch_env >> 7));
    int32_t kick_e = !dub_lane_heard_audible(0u) ? 0 : (g.synth.kick_env >> 7);
    int32_t sn_e   = !dub_lane_heard_audible(1u) ? 0 : (g.synth.rim_env >> 7);
    int32_t hat_e  = !dub_lane_heard_audible(2u) ? 0 : (g.synth.hat_env >> 7);
    int32_t stb_e  = !dub_lane_heard_audible(3u) ? 0 : ((int32_t)g.synth.meter[3] + (g.synth.siren_env >> 7));
    int32_t drm_e  = kick_e + sn_e + (hat_e >> 1);
    uint32_t step_frac256 = g.playing ? dub_sample_acc_frac256(g.sample_acc, g.samples_per_step) : 0u;
    uint32_t motion_mul = 1u + ((uint32_t)m_motion * 5u) / 127u;
    uint32_t beat_ph = g.playing ? (((uint32_t)g.step << 24) + (step_frac256 * motion_mul << 16)) : 0u;
    uint32_t z_frac = (step_frac256 * motion_mul) & 255u;
    int32_t pan_bias = ((int32_t)(p->pan[1] ? p->pan[1] : 64u) - 64) / 2;
    int32_t flux_orb = m_flux > 0u
                       ? ((dub_sine_q64(beat_ph + ((uint32_t)m_flux << 18)) * ((int32_t)m_flux >> 2)) >> 15) : 0;
    int32_t flux_orb_y = m_flux > 0u
                         ? ((dub_sine_q64(beat_ph + ((uint32_t)m_flux << 20) + 0x40000000u) *
                             ((int32_t)m_flux >> 3)) >> 15) : 0;
    int32_t sway_x = (g.playing ? ((dub_sine_q64(beat_ph << 2) * (6 + (m_jack >> 4))) >> 15) : 0) +
                     pan_bias + flux_orb;
    int32_t vp_x = 120 + sway_x;
    int32_t vp_y = 12 - (iso / 3) - ((kick_e > 120 ? 120 : kick_e) >> 5) +
                   ((int32_t)m_iso * 36) / 127 + flux_orb_y;
    uint32_t nplanes = 12u + (uint32_t)(((int32_t)m_layer * 10) / 127);
    int32_t floor_amp = 256 - duck * 2;

    if (fov_zoom < 70)
        fov_zoom = 70;
    if (floor_amp < 96)
        floor_amp = 96;
    if (nplanes > 22u)
        nplanes = 22u;
    if (m_motion > 96u)
        z_frac = (uint8_t)(255u - z_frac);

    for (i = 0u; i < 11u; i++) {
        int32_t fx = -16 + (int32_t)i * 27;
        int32_t bx = vp_x + ((fx - 120) * 74) / 128;
        int32_t fx_cl = fx < 1 ? 1 : (fx > 238 ? 238 : fx);
        uint16_t rcol = ((i & 1u) && (chd_e > 60 || m_jack > 90)) ? C_DIM : C_LINE;

        cv_line(bx, vp_y + 4, fx_cl, 126, rcol);
    }

    for (row = 0u; row < nplanes; row++) {
        uint32_t dist_from_front = (nplanes - 1u) - row;
        uint8_t st_hist = (uint8_t)((g.step + DUB_STEPS - (dist_from_front & 15u)) & 15u);
        uint8_t gate_h = step_gate(st_hist);
        int on_beat = (st_hist & 3u) == 0u;
        int offbeat = (st_hist & 3u) == 2u;
        int32_t echo_att = 256 - (int32_t)(dist_from_front * 150u) / (int32_t)nplanes;
        int32_t live_blend = dist_from_front == 0u ? 256 :
                             (dist_from_front == 1u ? 140 : (dist_from_front == 2u ? 64 : 0));
        int32_t h_sub  = ((dub_lane_heard_audible(4u) && (gate_h & 16u)) ? (18 + (p->sub_oct >> 3)) : 3) * echo_att >> 8;
        int32_t h_kick = ((dub_lane_heard_audible(0u) && (gate_h & 1u)) ?
                          (22 + (p->kick_boom >> 3) + (on_beat ? (m_jack >> 2) : 0)) : 0) * echo_att >> 8;
        int32_t h_sn   = ((dub_lane_heard_audible(1u) && (gate_h & 2u)) ? (16 + (p->rim_tone >> 3)) : 0) * echo_att >> 8;
        int32_t h_hat  = ((dub_lane_heard_audible(2u) && (gate_h & 4u)) ?
                          (8 + (p->hat_lvl >> 4) + (offbeat ? (m_jack >> 1) : (m_jack >> 4))) : 0) * echo_att >> 8;
        int32_t h_chd  = ((dub_lane_heard_audible(5u) && (gate_h & 32u)) ? (14 + (p->cutoff_c >> 3)) : 2) * echo_att >> 8;
        int32_t h_stb  = ((dub_lane_heard_audible(3u) && (gate_h & 8u)) ? (16 + (p->siren_pitch >> 2)) : 0) * echo_att >> 8;
        int32_t z_lin = (int32_t)(((row << 8) + z_frac) / nplanes);
        int32_t z_persp = ((84 + ((z_lin * z_lin * 172) >> 16)) * fov_zoom) / 100;
        int32_t base_y = vp_y + ((z_lin * (118 - vp_y)) >> 8);
        int32_t skew_shift = ((256 - z_persp) * (18 + sway_x)) >> 8;
        int32_t half_w = (122 * z_persp) >> 8;
        int32_t cx = vp_x + skew_shift;
        int32_t x0 = cx - half_w;
        int32_t x1 = cx + half_w;
        uint32_t segs = 40u, s_ix;
        int32_t py = -1, px = -1;

        h_hat = (h_hat * cut_n) >> 8;
        h_stb = (h_stb * cut_n) >> 8;
        h_sub += (sub_e * live_blend) >> 9;
        h_kick += (kick_e * live_blend) >> 9;
        h_sn += (sn_e * live_blend) >> 9;
        h_hat += (hat_e * live_blend) >> 10;
        h_chd += (chd_e * live_blend) >> 9;
        h_stb += (stb_e * live_blend) >> 9;
        h_sub = (h_sub * floor_amp) >> 8;
        h_chd = (h_chd * floor_amp) >> 8;
        h_kick = (h_kick * (256 - duck)) >> 8;

        if (x0 < 2)
            x0 = 2;
        if (x1 > 237)
            x1 = 237;
        if (x1 <= x0 + 8)
            continue;

        for (s_ix = 0u; s_ix <= segs; s_ix++) {
            int32_t xp = x0 + ((int32_t)s_ix * (x1 - x0)) / (int32_t)segs;
            int32_t d_sub = (int32_t)s_ix - 10;
            int32_t env_sub_x = 256 - (d_sub * d_sub * 3) / 2;
            int32_t d_kck = (int32_t)s_ix - 20;
            int32_t env_kck_x = 256 - d_kck * d_kck * 5;
            int32_t d_sn = (int32_t)s_ix - 23;
            int32_t env_sn_x = 256 - d_sn * d_sn * 6;
            int32_t d_chd = (int32_t)s_ix - 29;
            int32_t env_chd_x = 256 - d_chd * d_chd * 3;
            int32_t d_stb = (int32_t)s_ix - 35;
            int32_t env_stb_x = 256 - d_stb * d_stb * 7;
            uint32_t ph_anim = g.playing ? (beat_ph << 2) : (uint32_t)(g.synth.sub_ph >> 4);
            int32_t w_sub, w_kck, w_sn, w_hat, w_chd, w_stb, w_hf, dy, y;
            int32_t depth_bright, crest_glow, glow;
            uint16_t col;
            uint32_t flux_era = ((uint32_t)m_flux * 5u) / 127u;

            if (env_sub_x < 0) env_sub_x = 0;
            if (env_kck_x < 0) env_kck_x = 0;
            if (env_sn_x < 0) env_sn_x = 0;
            if (env_chd_x < 0) env_chd_x = 0;
            if (env_stb_x < 0) env_stb_x = 0;

            w_sub = ((dub_sine_q64(s_ix * 0x07000000u + st_hist * 0x20000000u - ph_anim) * h_sub) >> 15) *
                    env_sub_x >> 8;
            w_kck = ((dub_sine_q64((uint32_t)(d_kck + 8) * 0x14000000u) * h_kick) >> 15) * env_kck_x >> 8;
            if (on_beat && h_kick > 0)
                w_kck += (h_kick * env_kck_x) >> 8;
            w_sn = ((dub_sine_q64(s_ix * 0x2A000000u + st_hist * 0x50000000u) * h_sn) >> 15) * env_sn_x >> 8;
            w_hat = 0;
            if (h_hat > 0 && ((s_ix + (offbeat ? 0u : 1u)) & 1u))
                w_hat = (h_hat * (80 + env_kck_x)) >> 9;
            w_chd = (((dub_sine_q64(s_ix * 0x16000000u + st_hist * 0x30000000u) +
                       (dub_sine_q64(s_ix * 0x22000000u - ph_anim) >> 1)) * h_chd) >> 15) * env_chd_x >> 8;
            w_stb = ((dub_square(s_ix * (0x2C000000u + (uint32_t)p->siren_rate * 0x00300000u) +
                                 (uint32_t)g.synth.siren_lfo_ph) * h_stb) >> 15) * env_stb_x >> 8;
            w_hf = ((dub_sine_q64(s_ix * 0x3C000000u + ph_anim) * (h_hat + h_stb + (h_chd >> 2))) >> 15) *
                   cut_n >> 8;
            if (m_grit > 0u) {
                int32_t gj = ((s_ix & 3u) ? 1 : -1) * ((int32_t)m_grit >> 2);

                w_sub += (w_sub * (int32_t)m_grit) / 512 + (gj >> 1);
                w_kck += (w_kck * (int32_t)m_grit) / 256 + gj;
                w_stb += (h_stb * (int32_t)m_grit) >> 9;
                w_chd = (w_chd * (256 + (int32_t)m_grit)) / 256;
            }

            {
                int32_t cx_rel = (int32_t)s_ix - 20;
                int32_t pump_crater = ((200 - cx_rel * cx_rel) * (duck + ((int32_t)m_pump >> 2)) * z_persp) /
                                      (1600 * 20);

                dy = ((((w_sub + w_kck + w_sn + w_hat + w_chd + w_stb + w_hf) * z_persp) >> 8) *
                      amp_scale * flip_sign) / 256;
                y = base_y - dy + pump_crater;
            }
            if (m_jack > 48u && offbeat && (s_ix & 7u) == 0u && dy > 4)
                cv_line(xp, y, xp, y - (dy >> 2), C_ACC);

            depth_bright = ((z_persp - 104) * 3) >> 1;
            crest_glow = ((dy < 0 ? -dy : dy) * 14) + ((sub_e + chd_e + drm_e) >> 2);
            glow = depth_bright + crest_glow - (row == 0u ? (int32_t)(255u - z_frac) >> 1 : 56);

            if (y < 3)
                y = 3;
            if (y > 127)
                y = 127;

            if (env_stb_x > 64 && h_stb > 8)
                col = C_ACC;
            else if (env_kck_x > 80 && h_kick > 12 && (dy > 5 || dy < -5))
                col = C_WHITE;
            else if (env_sn_x > 70 && h_sn > 8)
                col = color_sat_wave(ecol, glow + 20);
            else if (env_chd_x > 80 && h_chd > 10)
                col = C_HI;
            else if (st_hist & 1u)
                col = color_sat_wave(ERA_COL[((p->era + flux_era) % GENRE_ERA_N)], glow);
            else
                col = color_sat_wave(ERA_COL[((genre_era_of_preset(p->plate) + flux_era) % GENRE_ERA_N)],
                                     glow - 20);
            if (m_grit > 0u && (dy > 8 || dy < -8) && (s_ix & 3u) == 0u &&
                ((int32_t)m_grit * 2) > (int32_t)(dy < 0 ? -dy : dy))
                cv_pset(xp, y, (s_ix & 1u) ? C_ACC : C_WHITE);

            if (px >= 0) {
                int32_t drop = 4 + (z_persp >> 5);
                int32_t mx = (px + xp) >> 1;
                int32_t my = (py + y) >> 1;

                if (my + drop <= 128)
                    cv_line(mx, my + 1, mx, my + drop, C_BLACK);
                if ((s_ix & 3u) == 0u && (dy > 4 || m_layer > 40u)) {
                    int32_t sx = xp - ((skew_shift >> 3) + 2);
                    int32_t sy = base_y + 2;

                    if (sx >= 2 && sx <= 237 && sy <= 127 && y < sy)
                        cv_line(xp, y, sx, sy, z_persp > 165 ? C_DIM : C_LINE);
                }
                cv_line(px, py, xp, y, col);
                if (m_floor > 0u && row + 3u >= nplanes) {
                    int32_t ug = 2 + (m_floor >> 5);
                    uint16_t fcol = color_scale_rgb(ecol, 28 + (m_floor >> 1));

                    if (py + ug <= 127 && y + ug <= 127)
                        cv_line(px, py + ug, xp, y + ug, fcol);
                    if (m_floor > 40u && py + ug + 2 <= 127 && y + ug + 2 <= 127)
                        cv_line(px, py + ug + 2, xp, y + ug + 2,
                                color_scale_rgb(C_ACC, 20 + (m_floor >> 2)));
                }
                if (m_layer > 16u) {
                    int32_t lgap = 2 + (m_layer >> 5);

                    if (py + lgap <= 127 && y + lgap <= 127)
                        cv_line(px, py + lgap, xp, y + lgap, z_persp > 155 ? C_DIM : C_LINE);
                }
                if ((z_persp > 195 || m_pump > 32u) && (h_kick > 14 || h_sub > 12) && y + 1 <= 127)
                    cv_line(px, py + 1, xp, y + 1, col);
            }
            px = xp;
            py = y;
        }
    }
}

#endif
