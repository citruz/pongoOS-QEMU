#ifdef QEMU 

#include <pongo.h>
#include "pl011.h"

console_t gConsole;
uint32_t uart_should_drop_rx;

#define write_32(offset, val) *((uint32_t *)(gConsole.base + (offset))) = (val)

void pl011_putc(char c) {
    gConsole.putc(c, &gConsole);
}

char pl011_getc() {
    return gConsole.getc(&gConsole);
}

void pl011_flush() {
    gConsole.flush(&gConsole);
}

bool pl011_early_init() {
    struct {
        uint64_t base_addr;
        uint64_t size;
    } __packed uart_base;

    if (!fdtree_find_prop("pl011@", "reg", &uart_base, sizeof(uart_base))) {
        return false;
    }
    uart_base.base_addr = __bswap64(uart_base.base_addr);
    uart_base.size = __bswap64(uart_base.size);

    // TODO get frequency from device tree
    if (!console_pl011_register(uart_base.base_addr, 24000000, 115200, &gConsole)) {
        return false;
    }

    gConsole.putc = &console_pl011_putc;
    gConsole.getc = &console_pl011_getc;
    gConsole.flush = &console_pl011_flush;

    return true;
}

extern void queue_rx_char(char inch);
void pl011_main() {
    while(1) {
        disable_interrupts();
        char cmd_l = pl011_getc();
        do {
            if (!uart_should_drop_rx) {
                enable_interrupts();
                queue_rx_char(cmd_l); // may take stdin lock
                disable_interrupts();
            }
            cmd_l = pl011_getc();
        } while(cmd_l != ERROR_NO_PENDING_CHAR);
        enable_interrupts();
        task_exit_irq();
    }
}

void pl011_init() {
    struct task* irq_task = task_create_extended("pl011", pl011_main, TASK_IRQ_HANDLER|TASK_PREEMPT, 0);
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

serial_ops_t pl011_serial_ops = {
    .putc = pl011_putc,
    .flush = pl011_flush,
    .init = pl011_init,
};

#endif