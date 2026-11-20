/* SPDX-License-Identifier: GPL-3.0-only */
#define DINGHY_WEB 1
#ifdef EMU_APP_RYTM
#include "../../firmware/src/rytm.c"
#elif defined(EMU_APP_CTRL)
#include "../../firmware/src/ctrl.c"
#elif defined(EMU_APP_DUB)
#include "../../firmware/src/fm_app.c"
#elif defined(EMU_APP_HOUSE)
#define GENRE_HOUSE 1
#include "../../firmware/src/fm_app.c"
#elif defined(EMU_APP_HEAVY_PSYCH)
#define GENRE_HEAVY_PSYCH 1
#include "../../firmware/src/fm_app.c"
#elif defined(EMU_APP_JAZZ)
#define GENRE_JAZZ 1
#include "../../firmware/src/fm_app.c"
#elif defined(EMU_APP_SOUL)
#define GENRE_SOUL 1
#include "../../firmware/src/fm_app.c"
#elif defined(EMU_APP_IDM)
#define GENRE_IDM 1
#include "../../firmware/src/fm_app.c"
#elif defined(EMU_APP_HIPHOP)
#define GENRE_HIPHOP 1
#include "../../firmware/src/fm_app.c"
#elif defined(EMU_APP_POP)
#define GENRE_POP 1
#include "../../firmware/src/fm_app.c"
#elif defined(EMU_APP_FUNK)
#define GENRE_FUNK 1
#include "../../firmware/src/fm_app.c"
#elif defined(EMU_APP_SNOWCRASH)
#define GENRE_SNOWCRASH 1
#include "../../firmware/src/fm_app.c"
#elif defined(EMU_APP_TECHNO)
#define GENRE_TECHNO 1
#include "../../firmware/src/fm_app.c"
#elif defined(EMU_APP_TRIPHOP)
#define GENRE_TRIPHOP 1
#include "../../firmware/src/fm_app.c"
#elif defined(EMU_APP_BASS)
#define GENRE_BASS 1
#include "../../firmware/src/fm_app.c"
#elif defined(EMU_APP_DECK)
#include "../../firmware/src/deck.c"
#elif defined(EMU_APP_JET)
#include "../../firmware/src/jet.c"
#else
#include "../../firmware/src/dinghy.c"
#endif

#define OUT_CAP 65536u
#define OUT_MASK (OUT_CAP - 1u)

static float ring_l[OUT_CAP];
static float ring_r[OUT_CAP];
static uint32_t ring_w;

static uint32_t sample_acc;
static uint32_t held_btn;
static uint8_t web_app_inited;

static void push_sample(float l, float r)
{
    ring_l[ring_w & OUT_MASK] = l;
    ring_r[ring_w & OUT_MASK] = r;
    ring_w++;
}

static void run_ms(void)
{
    static int32_t blk[2u * HALF_FRAMES];
    uint32_t btn = held_btn;
    uint32_t rising = btn & ~host_btn_prev;
    uint32_t falling = host_btn_prev & ~btn;

    host_btn_prev = btn;
    host_edges |= rising;
    host_released |= falling;
    fm1_in.buttons = btn;
    fm1_ms++;

    app_step();

    sample_acc += 441u;
    while (sample_acc >= 2560u) {
        uint32_t i;
        sample_acc -= 2560u;
        for (i = 0; i < 2u * HALF_FRAMES; i++)
            blk[i] = 0;
        app_audio(blk, HALF_FRAMES);
        for (i = 0; i < HALF_FRAMES; i++) {
            int32_t l = (blk[2u * i] * master_q12) >> 12;
            int32_t r = (blk[2u * i + 1u] * master_q12) >> 12;
            if (l > 32767)
                l = 32767;
            else if (l < -32767)
                l = -32767;
            if (r > 32767)
                r = 32767;
            else if (r < -32767)
                r = -32767;
            push_sample((float)l / 32768.f, (float)r / 32768.f);
        }
    }
}

__attribute__((used, visibility("default"))) void web_boot(uint32_t ms0)
{
    fm1_ms = ms0;
}

__attribute__((used, visibility("default"))) void web_render(uint32_t n)
{
    if (!web_app_inited) {
        app_init();
        web_app_inited = 1u;
    }
    while (n--)
        run_ms();
}

__attribute__((used, visibility("default"))) float *web_out_l(void) { return ring_l; }

__attribute__((used, visibility("default"))) float *web_out_r(void) { return ring_r; }

__attribute__((used, visibility("default"))) uint32_t web_out_w(void) { return ring_w; }

__attribute__((used, visibility("default"))) void web_buttons(uint32_t m) { held_btn = m; }

__attribute__((used, visibility("default"))) void web_keys(uint32_t m) { fm1_in.notes = m; }

__attribute__((used, visibility("default"))) void web_enc(int role, int32_t n)
{
    if (role >= 0 && role < 7)
        enc_acc[role] += n;
}

__attribute__((used, visibility("default"))) void web_master(uint32_t v)
{
    uint32_t k10 = (v > 4096u ? 4096u : v) >> 2;
    master_q12 = (int32_t)((k10 * k10) >> 8);
}

__attribute__((used, visibility("default"))) void web_midi_in(uint32_t st, uint32_t d1, uint32_t d2)
{
    mq[mq_w % 64u][0] = (uint8_t)st;
    mq[mq_w % 64u][1] = (uint8_t)d1;
    mq[mq_w % 64u][2] = (uint8_t)d2;
    mq_w++;
}

__attribute__((used, visibility("default"))) uint16_t *web_fb(void) { return fb; }

__attribute__((used, visibility("default"))) uint32_t web_blits(void) { return blits; }

__attribute__((used, visibility("default"))) uint32_t web_lit_buttons(void) { return lit_btn; }

__attribute__((used, visibility("default"))) uint32_t web_dim_buttons(void) { return lit_btn_dim; }

__attribute__((used, visibility("default"))) uint32_t web_btn_level(uint32_t id)
{
    return id < 14u ? lit_btn_level[id] : 0u;
}

__attribute__((used, visibility("default"))) uint32_t web_lit_keys(void) { return lit_keys; }

#if defined(EMU_APP_RYTM) || defined(EMU_APP_DUB) || defined(EMU_APP_HOUSE) || defined(EMU_APP_HEAVY_PSYCH) || defined(EMU_APP_JAZZ) || defined(EMU_APP_SOUL) || defined(EMU_APP_IDM) || defined(EMU_APP_HIPHOP) || defined(EMU_APP_SNOWCRASH) || defined(EMU_APP_POP) || defined(EMU_APP_FUNK) || defined(EMU_APP_TECHNO) || defined(EMU_APP_TRIPHOP) || defined(EMU_APP_BASS) || defined(EMU_APP_DECK) || defined(EMU_APP_JET)
__attribute__((used, visibility("default"))) uint32_t web_mode(void) { return g.view; }
#if defined(EMU_APP_BASS)
__attribute__((used, visibility("default"))) uint32_t web_deck_tag(void) { return 0x42415331u; } /* BAS1 / FM-1_56410 */
#elif defined(EMU_APP_DECK)
__attribute__((used, visibility("default"))) uint32_t web_deck_tag(void) { return 0x4445434Bu; } /* DECK / FM-1_56413 */
#elif defined(EMU_APP_JET)
__attribute__((used, visibility("default"))) uint32_t web_deck_tag(void) { return 0x4A455431u; } /* JET1 / FM-1_56415 */
#endif
#elif defined(EMU_APP_CTRL)
__attribute__((used, visibility("default"))) uint32_t web_mode(void) { return g_ctrl.profile; }
#else
__attribute__((used, visibility("default"))) uint32_t web_mode(void) { return g_cfg.mode; }
#endif

__attribute__((used, visibility("default"))) uint8_t *web_store_ptr(void) { return host_store; }

__attribute__((used, visibility("default"))) uint32_t web_store_len(void) { return sizeof host_store; }

__attribute__((used, visibility("default"))) uint32_t web_store_bytes(void) { return host_store_bytes(); }

__attribute__((used, visibility("default"))) uint32_t web_store_writes(void) { return host_store_writes; }
