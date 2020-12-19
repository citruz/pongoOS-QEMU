#ifdef QEMU 

#include <pongo.h>
#include "pl011.h"

console_t gConsole;
uint32_t uart_should_drop_rx;


static inline void put_serial_modifier(const char* str) {
    while (*str) serial_putc(*str++);
}

extern uint32_t gLogoBitmap[32];
void serial_init() {
    struct {
        uint64_t base_addr;
        uint64_t size;
    } __packed uart_base;

    if (!fdtree_find_prop("pl011@", "reg", &uart_base, sizeof(uart_base))) {
        screen_puts("did not find uart");
    }
    uart_base.base_addr = __bswap64(uart_base.base_addr);
    uart_base.size = __bswap64(uart_base.size);
    char buf[100];
    snprintf(buf, 100, "uart reg=%#llx size%#llx", uart_base.base_addr, uart_base.size);
    screen_puts(buf);
    map_range(0xf20000000ULL, uart_base.base_addr, uart_base.size, 3, 1, true);

    if (!console_pl011_register(0xf20000000ULL, 24000000, 115200, &gConsole)) {
        screen_puts("console_pl011_register FAILED");
        panic("console_pl011_register failed");
    }

    gConsole.putc = &console_pl011_putc;
    gConsole.getc = &console_pl011_getc;
    gConsole.flush = &console_pl011_flush;

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
}

void serial_early_init() {

}

void serial_pinmux_init() {

}
void serial_putc(char c) {
    gConsole.putc(c, &gConsole);
}
void serial_disable_rx() {
    uart_should_drop_rx = 1;
}
void serial_enable_rx() {
    uart_should_drop_rx = 0;
}
void uart_flush() {
    gConsole.flush(&gConsole);
}

#endif