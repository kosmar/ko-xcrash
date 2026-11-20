/* SPDX-License-Identifier: GPL-3.0-only */
/* A fake FM-1 on a computer: the app (firmware/src/dinghy.c) runs unchanged on it. platform.c includes
 * this file instead of the hardware when DINGHY_HOST is set. A script presses keys over time; out come
 * the screen (PPM, at the times SHOT asks) and the sound (WAV, as the FM-1 sends it over USB).
 *
 *   mkdir -p build/host
 *   cc -O2 -Wall -DDINGHY_HOST -Ifirmware/src -Ifirmware/gen -o build/host/dinghy firmware/src/dinghy.c
 *   build/host/dinghy build/host/dinghy        (-> build/host/dinghy.wav, build/host/dinghy-<ms>.ppm)
 *   build/host/dinghy OUT script.txt [FPS]    (your own script; with FPS: OUT-NNNNN.ppm, a frame every 1/FPS s:
 *                                             a film of your firmware, e.g. ffmpeg -r 30 -i OUT-%05d.ppm ...)
 *
 * Change SCRIPT below to try your own app. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef APP_NAME
#define APP_NAME "DINGHY"
#endif
#ifndef APP_VER
#define APP_VER "0.0"
#endif
#define FS 44100u
#define HALF_FRAMES 256u

static volatile uint32_t fm1_ms;
static volatile int host_done;
static struct { uint32_t notes, buttons; } fm1_in;
static volatile int32_t master_q12 = 2048;
static uint32_t host_edges;
static uint32_t host_btn_rel;
static int32_t host_enc_q[8];
static uint32_t host_shots[64];
static uint32_t host_shot_n;
static uint32_t fm1_input_edges(uint32_t *released)
{
    uint32_t b = host_edges;
    *released = host_btn_rel;
    host_edges = 0;
    host_btn_rel = 0;
    return b;
}
static uint32_t fm1_input_note_edges(void) { return 0; }   /* (the script sets fm1_in.notes) */
static int32_t fm1_enc_take(uint32_t e)
{
    int32_t s;
    if (e >= 8u)
        return 0;
    s = host_enc_q[e];
    host_enc_q[e] = 0;
    return s;
}
static void fm1_led_key_dim(uint32_t id, int on) { (void)id; (void)on; }
static void fm1_led_key_level(uint32_t id, uint8_t bright) { (void)id; (void)bright; }
static void fm1_led_key(uint32_t id, int on) { (void)id; (void)on; }

/* the screen: 240 x 240 RGB565, as the ST7789 gets it (gfx.c sends the bytes swapped) */
static uint16_t fb[240 * 240];
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
static uint32_t host_store_n;
static int host_store_in_done;

static void host_store_read_in(void)
{
    const char *path;
    FILE *f;

    if (host_store_in_done)
        return;
    host_store_in_done = 1;
    path = getenv("HOST_STORE_IN");
    if (!path || !path[0])
        return;
    f = fopen(path, "rb");
    if (!f)
        return;
    if (fread(&host_store_n, sizeof host_store_n, 1, f) != 1) {
        fclose(f);
        return;
    }
    if (fread(host_store, 1, 4, f) != 4) {
        fclose(f);
        return;
    }
    if (host_store_n > SETTINGS_MAX) {
        fclose(f);
        return;
    }
    if (fread(host_store + 4, 1, host_store_n, f) != host_store_n) {
        fclose(f);
        return;
    }
    fclose(f);
}

static void host_store_write_out(void)
{
    const char *path;
    FILE *f;

    path = getenv("HOST_STORE_OUT");
    if (!path || !path[0] || host_store[0] != 1)
        return;
    f = fopen(path, "wb");
    if (!f)
        return;
    fwrite(&host_store_n, sizeof host_store_n, 1, f);
    fwrite(host_store, 1, 4 + host_store_n, f);
    fclose(f);
}

static int settings_load(void *p, uint32_t n)
{
    host_store_read_in();
    if (host_store[0] != 1 || n > SETTINGS_MAX || n != host_store_n)
        return -1;
    memcpy(p, host_store + 4, n);
    return 0;
}
static int settings_save(const void *p, uint32_t n)
{
    if (n > SETTINGS_MAX)
        return -1;
    host_store[0] = 1;
    host_store_n = n;
    memcpy(host_store + 4, p, n);
    printf("%6u ms  settings saved (%u B)\n", fm1_ms, n);
    host_store_write_out();
    return 0;
}

static uint8_t mq[64][3];
static uint32_t mq_w, mq_r;
static int midi_get(uint8_t *st, uint8_t *d1, uint8_t *d2)
{
    if (mq_r == mq_w) return 0;
    *st = mq[mq_r % 64u][0]; *d1 = mq[mq_r % 64u][1]; *d2 = mq[mq_r % 64u][2];
    mq_r++;
    return 1;
}
static void midi_send(uint32_t st, uint32_t d1, uint32_t d2) { printf("%6u ms  MIDI out %02X %u %u\n", fm1_ms, st, d1, d2); }

static void app_audio(int32_t *out, uint32_t n);
static void app_main(void);

/* the script: at t ms, keys down (bit k = key k, 0 = F3), a button edge, a MIDI message. A script file (the second
 * argument) replaces it, a line each: "T keys MASK" (hex or decimal), "T btn ID" (a 40 ms tap), "T midi ST D1 D2", "T end" */
typedef struct { uint32_t t, keys; int btn; uint8_t st, d1, d2; } host_ev_t;
static const host_ev_t SCRIPT0[] = {
    {200, 1u << 7, -1, 0, 0, 0},                                /* C4 */
    {700, 1u << 7 | 1u << 11 | 1u << 14, -1, 0, 0, 0},          /* + E4 G4: Coltrane */
    {800, 0, -1, 0xB0, 1, 108},                                 /* CC -> HUD @ SHOT[0] */
    {1500, 0, -1, 0, 0, 0},
    {1700, 0, -1, 0x90, 81, 120},                               /* MIDI in: A5 root */
    {2000, 0, -1, 0xB0, 31, 1},                                 /* MODE_CONTURA */
    {2020, 0, BTN_SEL, 0, 0, 0},                                /* ENGINE bank */
    {2050, 0, -1, 0xB0, 16, 64},                                /* scale / engine A */
    {2100, 0, -1, 0xB0, 18, 96},                                /* density */
    {2150, 0, -1, 0xB0, 19, 110},                               /* full trio mix */
    {3400, 0, -1, 0, 0, 0},                                     /* HUD gone -> Contura stage @ SHOT[1] */
    {3600, 0, -1, 0xB0, 31, 2},                                 /* MODE_DITDAH (preset SOS) */
    {3800, 1u << 18, -1, 0, 0, 0},                               /* key S -> CUSTOM */
    {3850, 0, -1, 0, 0, 0},
    {3900, 1u << 14, -1, 0, 0, 0},                               /* key O */
    {3950, 0, -1, 0, 0, 0},
    {4000, 1u << 18, -1, 0, 0, 0},                               /* key S */
    {4200, 0, -1, 0, 0, 0},
};
static const host_ev_t *script = SCRIPT0;
static uint32_t script_n = sizeof SCRIPT0 / sizeof SCRIPT0[0], end_ms = 5000, frame_fps;
static const uint32_t SHOT[] = {1000, 3500, 4600};

static void read_script(const char *path)
{
    static host_ev_t ev[4096];
    char line[256], what[16];
    FILE *f = fopen(path, "r");
    uint32_t n = 0;
    if (!f) { perror(path); exit(1); }
    while (fgets(line, sizeof line, f) && n < 4096u) {
        host_ev_t e = {0, 0, -1, 0, 0, 0};
        unsigned a = 0, b = 0, c = 0;
        if (line[0] == '#' || sscanf(line, "%u %15s", &e.t, what) != 2) continue;
        if (!strcmp(what, "end")) { end_ms = e.t; continue; }
        if (!strcmp(what, "shot") && host_shot_n < 64u) {
            host_shots[host_shot_n++] = e.t;
            continue;
        }
        if (!strcmp(what, "keys")) { sscanf(line, "%*u %*s %i", (int *)&e.keys); }
        else if (!strcmp(what, "btn")) {
            sscanf(line, "%*u %*s %d", &e.btn);
        } else if (!strcmp(what, "press")) {
            unsigned id = 0;
            sscanf(line, "%*u %*s %u", &id);
            e.btn = (int)(id | 256u);
        } else if (!strcmp(what, "release")) {
            unsigned id = 0;
            sscanf(line, "%*u %*s %u", &id);
            e.btn = (int)(id | 512u);
        } else if (!strcmp(what, "enc")) {
            int slot = 0, delta = 0;
            if (sscanf(line, "%*u %*s %d %d", &slot, &delta) == 2 && slot >= 0 && slot < 4) {
                e.btn = -2;
                e.st = (uint8_t)slot;
                e.d1 = (uint8_t)(delta < 0 ? 256u + delta : (uint8_t)delta);
            } else
                continue;
        } else if (!strcmp(what, "encid")) {      /* any encoder by id (ENC_SELECT = 0 .. ENC_PRESETS = 6) */
            int id = 0, delta = 0;
            if (sscanf(line, "%*u %*s %d %d", &id, &delta) == 2 && id >= 0 && id < 8) {
                e.btn = -3;
                e.d2 = (uint8_t)id;
                e.d1 = (uint8_t)(delta < 0 ? 256 + delta : delta);
            } else
                continue;
        } else if (!strcmp(what, "midi")) { sscanf(line, "%*u %*s %i %u %u", (int *)&a, &b, &c); e.st = (uint8_t)a; e.d1 = (uint8_t)b; e.d2 = (uint8_t)c; }
        else continue;
        ev[n++] = e;
    }
    fclose(f);
    script = ev;
    script_n = n;
}

static const char *host_out = "build/host/dinghy";
static int16_t *wav;
static uint32_t wav_n;

static void write_ppm_as(const char *name)
{
    FILE *f;
    uint32_t i;
    if (!(f = fopen(name, "wb"))) { perror(name); exit(1); }
    fprintf(f, "P6 240 240 255\n");
    for (i = 0; i < 240u * 240u; i++) {
        uint16_t c = fb[i];
        fputc((c >> 11) * 255 / 31, f);
        fputc((c >> 5 & 63) * 255 / 63, f);
        fputc((c & 31) * 255 / 31, f);
    }
    fclose(f);
}
static void write_ppm(uint32_t t)
{
    char name[512];
    snprintf(name, sizeof name, "%s-%u.ppm", host_out, t);
    write_ppm_as(name);
    printf("%6u ms  screen -> %s\n", t, name);
}

static void write_wav(void)
{
    char name[512];
    FILE *f;
    uint32_t bytes = wav_n * 4u, v[] = {16, 0x00020001u, FS, FS * 4u, 0x00100004u};
    snprintf(name, sizeof name, "%s.wav", host_out);
    if (!(f = fopen(name, "wb"))) { perror(name); exit(1); }
    fwrite("RIFF", 1, 4, f); bytes += 36; fwrite(&bytes, 4, 1, f); bytes -= 36;
    fwrite("WAVEfmt ", 1, 8, f); fwrite(v, 4, 5, f);
    fwrite("data", 1, 4, f); fwrite(&bytes, 4, 1, f);
    fwrite(wav, 4, wav_n, f);
    fclose(f);
    printf("sound  -> %s (%u frames)\n", name, wav_n);
}

static void platform_poll(void)                 /* one call = one millisecond */
{
    static uint32_t step, shot, owed, frame;
    static uint32_t tap_up[32];                 /* "btn" taps: release time per button, 0 = none */
    static int32_t blk[2u * HALF_FRAMES];
    uint32_t id_up;
    fm1_ms++;
    for (id_up = 0; id_up < 32u; id_up++)
        if (tap_up[id_up] && fm1_ms >= tap_up[id_up]) {
            tap_up[id_up] = 0;
            fm1_in.buttons &= ~(1u << id_up);
            host_btn_rel |= 1u << id_up;
        }
    while (step < script_n && script[step].t <= fm1_ms) {
        if (script[step].btn == -2) {
            int delta = script[step].d1 >= 128 ? (int)script[step].d1 - 256 : (int)script[step].d1;
            host_enc_q[2u + script[step].st] += delta;
        } else if (script[step].btn == -3) {
            host_enc_q[script[step].d2 & 7u] += script[step].d1 >= 128 ? (int)script[step].d1 - 256 : (int)script[step].d1;
        } else if (script[step].st) {
            mq[mq_w % 64u][0] = script[step].st; mq[mq_w % 64u][1] = script[step].d1; mq[mq_w % 64u][2] = script[step].d2;
            mq_w++;
        } else if (script[step].btn >= 0) {
            unsigned id = (unsigned)script[step].btn;
            if (id & 512u) {
                id &= 255u;
                fm1_in.buttons &= (uint32_t)~(1u << id);
                host_btn_rel |= 1u << id;
            } else if (id & 256u) {
                id &= 255u;
                fm1_in.buttons |= 1u << id;
                host_edges |= 1u << id;
            } else {
                host_edges |= 1u << id;
                fm1_in.buttons |= 1u << id;
                tap_up[id & 31u] = fm1_ms + 40u;
            }
        } else
            fm1_in.notes = script[step].keys;
        step++;
    }
    owed += FS;                                 /* the frames this millisecond is worth, in blocks as the ISR renders */
    while (owed >= 1000u * HALF_FRAMES) {
        uint32_t i;
        owed -= 1000u * HALF_FRAMES;
        memset(blk, 0, sizeof blk);
        app_audio(blk, HALF_FRAMES);
        wav = realloc(wav, (wav_n + HALF_FRAMES) * 4u);
        for (i = 0; i < 2u * HALF_FRAMES; i++) {
            int32_t v = blk[i];                 /* (USB audio at the MASTER knob all the way up) */
            wav[2u * wav_n + i] = (int16_t)(v > 32767 ? 32767 : v < -32767 ? -32767 : v);
        }
        wav_n += HALF_FRAMES;
    }
    if (frame_fps) {                            /* a film: a frame every 1/fps s, as the screen is then */
        if ((uint64_t)frame * 1000u <= (uint64_t)fm1_ms * frame_fps) {
            char name[512];
            snprintf(name, sizeof name, "%s-%05u.ppm", host_out, frame++);
            write_ppm_as(name);
        }
    }
    {
        static uint32_t shot_ix;
        while (shot_ix < host_shot_n && host_shots[shot_ix] <= fm1_ms) {
            if (host_shots[shot_ix] == fm1_ms)
                write_ppm(fm1_ms);
            shot_ix++;
        }
    }
    if (!host_shot_n && shot < sizeof SHOT / sizeof SHOT[0] && fm1_ms == SHOT[shot])
        write_ppm(SHOT[shot++]);
    if (fm1_ms >= end_ms)
        host_done = 1;
}

int host_sim_should_stop(void) { return host_done; }

void host_sim_finalize(void)
{
    write_wav();
    exit(0);
}

#if defined(POP_LEAD_VERIFY)
extern int pop_lead_verify_main(int argc, char **argv);
#endif

int main(int argc, char **argv)
{
#if defined(POP_LEAD_VERIFY)
    return pop_lead_verify_main(argc, argv);
#else
    if (argc > 1) host_out = argv[1];
    if (argc > 2) read_script(argv[2]);
    if (argc > 3) frame_fps = (uint32_t)atoi(argv[3]);
    app_main();
    return 0;
#endif
}
