/* SPDX-License-Identifier: GPL-3.0-only */
/* Native check: DINGHY_WEB jet path produces non-silent audio. */
#include <stdint.h>
#include <stdio.h>

void web_render(uint32_t n);
void web_keys(uint32_t m);
float *web_out_l(void);
uint32_t web_out_w(void);

int main(void)
{
    uint32_t w0;
    uint32_t w;
    uint32_t i;
    float pk = 0.f;

    web_render(50u);
    web_keys(1u << 7u); /* key 7 = C4 */
    w0 = web_out_w();
    web_render(800u);
    w = web_out_w();
    for (i = w0; i < w; i++) {
        float a = web_out_l()[i & 65535u];
        if (a < 0.f)
            a = -a;
        if (a > pk)
            pk = a;
    }
    printf("web_jet_audio_harness: peak=%g samples=%u\n", pk, w - w0);
    return pk < 0.01f ? 1 : 0;
}