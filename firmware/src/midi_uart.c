/* SPDX-License-Identifier: GPL-3.0-only */
/* TRS MIDI IN at 31250 baud: bytes from fm1_uart.h DMA ring -> the same midi_in_q as USB. */
#include "../hal/fm1_uart.h"

#define UART_RX_N 256u
static volatile uint8_t uart_rx_ring[UART_RX_N] __attribute__((aligned(16)));
static uint32_t uart_rx_pos;

static uint8_t mu_st, mu_d1, mu_expect;

static void mu_emit(uint32_t pkt)
{
    midi_enqueue(pkt, 2u);
}

static void mu_byte(uint8_t b)
{
    if (b >= 0xF8u) {
        if (b == 0xF8u || b == 0xFAu || b == 0xFBu || b == 0xFCu)
            mu_emit(0xFu | (uint32_t)b << 8);
        return; /* realtime keeps running status and in-flight data bytes */
    }
    if (b >= 0xF0u) {
        mu_st = 0;
        mu_expect = 0;
        return;
    }
    if (b & 0x80u) {
        mu_st = b;
        mu_expect = (b & 0xF0u) == 0xC0u || (b & 0xF0u) == 0xD0u ? 1u : 2u;
        return;
    }
    if (!mu_expect) {
        if (mu_st < 0x80u)
            return;
        mu_expect = (mu_st & 0xF0u) == 0xC0u || (mu_st & 0xF0u) == 0xD0u ? 1u : 2u;
    }
    if (mu_expect == 2u) {
        mu_d1 = b;
        mu_expect = 1u;
        return;
    }
    {
        uint32_t cin = mu_st >> 4;
        uint32_t d1v = mu_d1;
        uint32_t d2v = b;
        if ((mu_st & 0xF0u) == 0xC0u || (mu_st & 0xF0u) == 0xD0u) {
            d1v = b;
            d2v = 0;
        }
        if ((mu_st & 0xF0u) >= 0x80u && (mu_st & 0xF0u) <= 0xE0u)
            mu_emit(cin | (uint32_t)mu_st << 8 | d1v << 16 | d2v << 24);
        mu_expect = (mu_st & 0xF0u) == 0xC0u || (mu_st & 0xF0u) == 0xD0u ? 0u : 2u;
    }
}

void uart_midi_init(void)
{
    uart_rx_pos = 0;
    mu_st = mu_d1 = mu_expect = 0;
    fm1_uart1_midi_init(uart_rx_ring, UART_RX_N);
}

void uart_midi_poll(void)
{
    uint32_t n = fm1_uart1_rx_take();
    while (n--) {
        mu_byte(uart_rx_ring[uart_rx_pos++ & (UART_RX_N - 1u)]);
    }
}
