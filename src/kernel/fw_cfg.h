#ifndef fw_cfg_h
#define fw_cfg_h

#include <pongo.h>

#define FILETABLE_MAX_SIZE 512

void init_fw_cfg();

typedef struct fw_cfg_file {      /* an individual file entry, 64 bytes total */
    uint32_t size;      /* size of referenced fw_cfg item, big-endian */
    uint16_t select;        /* selector key of fw_cfg item, big-endian */
    uint16_t reserved;
    char name[56];      /* fw_cfg item name, NUL-terminated ascii */
} fw_cfg_file;


typedef struct ramfb_config {
  uint64_t address;
  uint32_t fourCC;
  uint32_t flags;
  uint32_t width;
  uint32_t height;
  uint32_t stride;
} __packed ramfb_config;

typedef struct fw_cfg_dma_access {
    uint32_t control;
    uint32_t length;
    uint64_t address;
} __packed fw_cfg_dma_access;

#endif
