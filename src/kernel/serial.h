#ifndef serial_h
#define serial_h

#ifndef __ASSEMBLER__

typedef struct {
    void (*putc)(char c);
    void (*flush)(void);
    void (*init)(void);
} serial_ops_t;

void serial_early_init(void);
void serial_putc(char c);
void serial_flush();
void serial_enable_rx();
void serial_disable_rx();

#endif

#endif