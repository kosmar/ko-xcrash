/* SPDX-License-Identifier: GPL-3.0-only */
/* FM-1 platform shim for wasm32 (DINGHY_WEB): same contracts as tests/host.c. */
#include <stddef.h>
#include <stdint.h>

#include "../../firmware/src/libc.c"

#ifndef APP_NAME
#define APP_NAME "FM-1"
#endif
#ifndef APP_VER
#define APP_VER "web"
#endif
#define FS 44100u
#define HALF_FRAMES 256u

static volatile uint32_t fm1_ms;
static struct {
    uint32_t notes;
    uint32_t buttons;
} fm1_in;

static volatile int32_t master_q12 = 2048;

static uint32_t host_edges;
static uint32_t host_released;
static uint32_t host_btn_prev;
static int32_t enc_acc[7];

static uint32_t fm1_input_edges(uint32_t *released)
{
    uint32_t b = host_edges;
    host_edges = 0;
    *released = host_released;
    host_released = 0;
    return b;
}

static uint32_t fm1_input_note_edges(void) { return 0; }

static int32_t fm1_enc_take(uint32_t e)
{
    int32_t v = 0;
    if (e < 7u) {
        v = enc_acc[e];
        enc_acc[e] = 0;
    }
    return v;
}

static uint32_t lit_btn;
static uint32_t lit_btn_dim;
static uint32_t lit_keys;
static uint8_t lit_btn_level[14u];

static void fm1_led_key_dim(uint32_t id, int on)
{
    if (id < 14u) {
        if (on)
            lit_btn_dim |= 1u << id;
        else
            lit_btn_dim &= ~(1u << id);
    }
}

static void fm1_led_key_level(uint32_t id, uint8_t bright)
{
    if (id < 14u) {
        lit_btn_level[id] = bright;
        if (bright > 127u)
            lit_btn |= 1u << id;
        else
            lit_btn &= ~(1u << id);
        if (bright == 0u || bright >= 255u)
            lit_btn_dim &= ~(1u << id);
    } else if (id >= 14u && id < 41u) {
        uint32_t k = id - 14u;
        if (bright > 127u)
            lit_keys |= 1u << k;
        else
            lit_keys &= ~(1u << k);
    }
}

static void fm1_led_key(uint32_t id, int on)
{
    if (id < 14u)
        fm1_led_key_level(id, on ? 255u : 0u);
    else if (id < 41u) {
        uint32_t k = id - 14u;
        if (on)
            lit_keys |= 1u << k;
        else
            lit_keys &= ~(1u << k);
    }
}

static uint16_t fb[240 * 240];
static uint32_t blits;

static void lcd_sync(void) {}

static void lcd_fill(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t c)
{
    uint32_t i, j;
    for (j = y; j < y + h && j < 240u; j++)
        for (i = x; i < x + w && i < 240u; i++)
            fb[j * 240u + i] = c;
}

static void lcd_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *px)
{
    uint32_t i, j;
    blits++;
    for (j = 0; j < h; j++)
        for (i = 0; i < w; i++)
            if (x + i < 240u && y + j < 240u) {
                uint16_t c = px[j * w + i];
                fb[(y + j) * 240u + x + i] = (uint16_t)(c >> 8 | c << 8);
            }
}

#define CV_MAX (240u * 216u)
#include "gfx.c"

static uint8_t host_store[SETTINGS_MAX + 4];
static uint32_t host_store_writes;
static uint32_t host_store_payload;

static uint32_t host_store_bytes(void)
{
    uint32_t n;

    if (host_store[0] != 1)
        return 0u;
    n = (uint32_t)host_store[1] | ((uint32_t)host_store[2] << 8) | ((uint32_t)host_store[3] << 16);
    if (n == 0u || n > SETTINGS_MAX)
        n = host_store_payload;
    if (n == 0u || n > SETTINGS_MAX)
        n = SETTINGS_MAX; /* legacy blobs before payload-length prefix */
    return 4u + n;
}

static int settings_load(void *p, uint32_t n)
{
    uint32_t stored_n;

    if (host_store[0] != 1 || n > SETTINGS_MAX)
        return -1;
    stored_n = (uint32_t)host_store[1] | ((uint32_t)host_store[2] << 8) | ((uint32_t)host_store[3] << 16);
    if (stored_n == 0u || stored_n > SETTINGS_MAX)
        stored_n = host_store_payload;
    if (n != stored_n)
        return -1;
    memcpy(p, host_store + 4, n);
    return 0;
}

static int settings_save(const void *p, uint32_t n)
{
    if (n > SETTINGS_MAX)
        return -1;
    host_store[0] = 1;
    host_store[1] = (uint8_t)n;
    host_store[2] = (uint8_t)(n >> 8);
    host_store[3] = (uint8_t)(n >> 16);
    host_store_payload = n;
    memcpy(host_store + 4, p, n);
    host_store_writes++;
    return 0;
}

static uint8_t mq[64][3];
static uint32_t mq_w, mq_r;

static int midi_get(uint8_t *st, uint8_t *d1, uint8_t *d2)
{
    if (mq_r == mq_w)
        return 0;
    *st = mq[mq_r % 64u][0];
    *d1 = mq[mq_r % 64u][1];
    *d2 = mq[mq_r % 64u][2];
    mq_r++;
    return 1;
}

#define MOQ 64u
static uint8_t midi_out_q[MOQ][3];
static uint32_t mo_w, mo_r;

static void midi_send(uint32_t st, uint32_t d1, uint32_t d2)
{
    midi_out_q[mo_w % MOQ][0] = (uint8_t)st;
    midi_out_q[mo_w % MOQ][1] = (uint8_t)d1;
    midi_out_q[mo_w % MOQ][2] = (uint8_t)d2;
    mo_w++;
}

static void platform_poll(void) {}
