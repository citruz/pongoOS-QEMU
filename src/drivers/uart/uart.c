// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// 
//
//  Copyright (c) 2019-2020 checkra1n team
//  This file is part of pongoOS.
//
#define UART_INTERNAL 1
#include <pongo.h>

char uart_queue[64];
uint8_t uart_queue_idx;
void uart_flush() {
    if (!uart_queue_idx) return;
    int local_queue_idx = 0;
    while (rUTRSTAT0 & 0x04)
    {
        if (local_queue_idx == uart_queue_idx) break;
        rUTXH0 = (unsigned)(uart_queue[local_queue_idx]);
        local_queue_idx++;
    }
    if (!local_queue_idx) return;

    if (local_queue_idx != uart_queue_idx) {
        memcpy(&uart_queue[0], &uart_queue[local_queue_idx], uart_queue_idx - local_queue_idx);
    }
    uart_queue_idx -= local_queue_idx;
}
void uart_force_flush() {
    for (int i = 0; i < uart_queue_idx; i++) {
        while (!(rUTRSTAT0 & 0x04)) {}
        rUTXH0 = (unsigned)(uart_queue[i]);
    }
    uart_queue_idx = 0;
}
void uart_update_tx_irq() {
    return;
    if (!uart_queue_idx)
        rUCON0 = 0x5885;
    else
        rUCON0 = 0x5F85;
}
uint32_t uart_should_drop_rx;
extern void queue_rx_char(char inch);
void uart_main() {
    while(1) {
        disable_interrupts();
        uint32_t utrst = rUTRSTAT0;
        rUTRSTAT0 = utrst;
        if (utrst & 0x40) {
            (void)rURXH0; // force read
        } else
        if (utrst & 1) {
            int rxh0 = rURXH0;
            if (!uart_should_drop_rx) {
                char cmd_l = rxh0;
                enable_interrupts();
                queue_rx_char(cmd_l); // may take stdin lock
                disable_interrupts();
            }
        }
        uint32_t uerst = rUERSTAT0;
        rUERSTAT0 = uerst;

        // clear interrupt
        rUINTP0 = UART_RX_INT;
        enable_interrupts();
        task_exit_irq();
    }
}

uint64_t gUartBase;
uint16_t uart_irq;

bool uart_early_init() {
    // Pinmux debug UART on ATV4K
    // This will also pinmux uart0 on iPad Pro 2G
    if((strcmp(soc_name, "t8011") == 0)) {
        rT8011TX = UART_TX_MUX;
        rT8011RX = UART_RX_MUX;
    }

#ifdef QEMU
    struct {
        uint64_t base_addr;
        uint64_t size;
    } __packed uart_info;
    struct {
        uint32_t type;
        uint32_t num;
        uint32_t flags;
    } __packed irq_info;
    if (fdtree_find_prop("exynos4210@", "reg", &uart_info, sizeof(uart_info)) &&
        fdtree_find_prop("exynos4210@", "interrupts", &irq_info, sizeof(irq_info))) {
        gUartBase = __bswap64(uart_info.base_addr);
        uart_irq = __bswap32(irq_info.num) + 32;
    } else {
        return false;
    }
#else
    gUartBase = dt_get_u32_prop("uart0", "reg");
    gUartBase += gIOBase;
    uart_irq = dt_get_u32_prop("uart0", "interrupts");
#endif
    rULCON0 = 3;
    rUCON0 = 0x405;
    rUFCON0 = 0;
    rUMCON0 = 0;

    return true;
}

void uart_init() {
    struct task* irq_task = task_create_extended("uart", uart_main, TASK_IRQ_HANDLER|TASK_PREEMPT, 0);

    disable_interrupts();
    serial_disable_rx();
    task_bind_to_irq(irq_task, uart_irq);
    rUCON0 = 0x5885;
    // ignore tx interrupts
    rUINTM0 = UART_TX_INT;
    // clear rx interrupts
    rUINTP0 = UART_RX_INT;
    enable_interrupts();
}

void uart_putc(char c) {
    if (c == '\n') uart_putc('\r');
    while (!(rUTRSTAT0 & 0x04)) {}
    rUTXH0 = (unsigned)(c);
    return;
}

serial_ops_t uart_serial_ops = {
    .putc = uart_putc,
    .init = uart_init,
};
