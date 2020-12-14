#ifndef fdtree_h
#define fdtree_h

#include <pongo.h>

void print_reserved_map(void *addr, fdt_header_t *header);
bool fdt_parse_header(void *addr, fdt_header_t *header);
void dump_fdtree(void *addr, fdt_header_t *header);

#endif