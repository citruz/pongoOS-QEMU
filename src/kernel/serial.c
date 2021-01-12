#include <pongo.h>
#include "serial.h"
#include "uart/uart.h"

#ifdef QEMU
#include "pl011/pl011.h"
#endif

void nop_putc(char c) {};
void nop() {};

uint32_t uart_should_drop_rx;

serial_ops_t gSerialOps = {
    .putc = nop_putc,
    .init = nop,
};

extern uint32_t gLogoBitmap[32];

static inline void put_serial_modifier(const char* str) {
    while (*str) serial_putc(*str++);
}

void serial_early_init() {
    disable_interrupts();

#ifdef QEMU
    if (pl011_early_init()) {
        gSerialOps = pl011_serial_ops;
    } else
#endif
    if(uart_early_init()) {
        gSerialOps = uart_serial_ops;
    } else {
        screen_puts("no supported serial interface found");
        enable_interrupts();
        return;
    }

    // print logo
    char reorder[6] = {'1','3','2','6','4','5'};
    char modifier[] = {'\x1b', '[', '4', '1', ';', '1', 'm', 0};
    int cnt = 0;
    for (int y=0; y < 32; y++) {
        uint32_t b = gLogoBitmap[y];
        for (int x=0; x < 32; x++) {
            if (b & (1 << (x))) {
                modifier[3] = reorder[((cnt) % 6)];
                put_serial_modifier(modifier);
            }
            serial_putc(' ');
            serial_putc(' ');
            if (b & (1 << (x))) {
                put_serial_modifier("\x1b[0m");
            }
            cnt = (x+1) + y;
        }
        serial_putc('\n');
    }
    enable_interrupts();
}

void serial_init(void) {
    gSerialOps.init();
}

void serial_putc(char c) {
    gSerialOps.putc(c);
}

void serial_disable_rx() {
    uart_should_drop_rx = 1;
}
void serial_enable_rx() {
    uart_should_drop_rx = 0;
}
