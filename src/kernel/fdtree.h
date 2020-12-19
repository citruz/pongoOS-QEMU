#ifndef fdtree_h
#define fdtree_h

#include <pongo.h>

extern fdt_header_t gFdtHeader;

void print_reserved_map(void *addr);
bool fdt_parse_header(void *addr);
void dump_fdtree(void *addr);

#endif