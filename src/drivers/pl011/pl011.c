#ifdef QEMU 

#include <pongo.h>
#include "pl011.h"

console_t gConsole;
uint32_t uart_should_drop_rx;

#define write_32(offset, val) *((uint32_t *)(gConsole.base + (offset))) = (val)

static inline void put_serial_modifier(const char* str) {
    while (*str) serial_putc(*str++);
}

void serial_pinmux_init() {

}
void serial_putc(char c) {
    gConsole.putc(c, &gConsole);
}

char serial_getc() {
    return gConsole.getc(&gConsole);
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

extern uint32_t gLogoBitmap[32];
void serial_early_init() {
    struct {
        uint64_t base_addr;
        uint64_t size;
    } __packed uart_base;

    if (!fdtree_find_prop("pl011@", "reg", &uart_base, sizeof(uart_base))) {
        screen_puts("did not find uart");
    }
    uart_base.base_addr = __bswap64(uart_base.base_addr);
    uart_base.size = __bswap64(uart_base.size);

    // TODO get frequency from device tree
    if (!console_pl011_register(uart_base.base_addr, 24000000, 115200, &gConsole)) {
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

extern void queue_rx_char(char inch);
void uart_main() {
    while(1) {
        disable_interrupts();
        char cmd_l = serial_getc();
        do {
            if (!uart_should_drop_rx) {
                enable_interrupts();
                queue_rx_char(cmd_l); // may take stdin lock
                disable_interrupts();
            }
            cmd_l = serial_getc();
        } while(cmd_l != ERROR_NO_PENDING_CHAR);
        enable_interrupts();
        task_exit_irq();
    }
}

void serial_init() {
    struct task* irq_task = task_create_extended("uart", uart_main, TASK_IRQ_HANDLER|TASK_PREEMPT, 0);

    struct {
        uint32_t type;
        uint32_t num;
        uint32_t flags;
    } __packed irq;

    if (!fdtree_find_prop("pl011@", "interrupts", &irq, sizeof(irq))) {
        panic("did not find uart interrupts");
    }
    irq.type = __bswap32(irq.type);
    irq.num = __bswap32(irq.num);
    irq.flags = __bswap32(irq.flags);

#ifdef DEBUG
    char buf[100];
    snprintf(buf, 100, "serial irq: type=%u num=%u flags=%#x", irq.type, irq.num, irq.flags);
    puts(buf);
#endif

    disable_interrupts();
    serial_disable_rx();

    task_bind_to_irq(irq_task, irq.num + 32);
    enable_interrupts();
}

#endif