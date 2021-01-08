#include "fw_cfg.h"

#define FW_CFG_SEL (((uint8_t *)gFwCfgBase) + 8)
#define FW_CFG_DATA (((uint8_t *)gFwCfgBase) + 0)
#define FW_CFG_DMA (((uint8_t *)gFwCfgBase) + 16)


#define FW_CFG_SIGNATURE	0x0
#define FW_CFG_ID 			0x1
#define FW_CFG_UUID			0x2
#define FW_CFG_RAM_SIZE		0x3
#define FW_CFG_FILE_DIR		0x19


#define RAMFB_FORMAT  0x34325258 /* DRM_FORMAT_XRGB8888 */
#define RAMFB_BPP     4

void *gFwCfgBase;

void fw_cfg_select(uint16_t selector) {
    *((uint16_t *)FW_CFG_SEL) = __bswap16(selector | (1 << 14));
}

uint16_t fw_cfg_read16() {
    return __bswap16(*((uint16_t *)FW_CFG_DATA));
}

uint32_t fw_cfg_read32() {
    return __bswap32(*((uint32_t *)FW_CFG_DATA));
}

uint64_t fw_cfg_read64_raw() {
    return *((uint64_t *)FW_CFG_DATA);
}

uint64_t fw_cfg_read64() {
    return __bswap64(fw_cfg_read64_raw());
}


void fw_cfg_read_bytes(uint8_t *buf, size_t size) {
    size_t i = 0;
    size_t rem_size = size;

    while (rem_size) {
        uint64_t val = fw_cfg_read64_raw();
         size_t to_read = sizeof(val);

        if (rem_size < sizeof(val)) {
            to_read = rem_size;
        }

        memcpy(&buf[i], &val, to_read);
        rem_size -= to_read;
        i += to_read;
    }
}

#define TO_PHYS(addr) (addr - 0x100000000 + 0x818000000)

void fw_cfg_write_dma(uint8_t *buf, uint32_t size) {
    fw_cfg_dma_access dma_ctrl;

    dma_ctrl.control = __bswap32((1 << 4));
    dma_ctrl.length = __bswap32(size);
    dma_ctrl.address = __bswap64(TO_PHYS((uint64_t)buf));

    *((uint64_t *)FW_CFG_DMA) = __bswap64(TO_PHYS((uint64_t)&dma_ctrl));
}

bool fw_cfg_find_file(char *name, uint16_t *store_select, uint32_t *store_size) {
    uint32_t count;

    fw_cfg_select(FW_CFG_FILE_DIR);
    count = fw_cfg_read32();

    for (uint32_t i = 0; i < count; i++) {
        fw_cfg_file file;

        file.size = fw_cfg_read32();
        file.select = fw_cfg_read16();
        file.reserved = fw_cfg_read16();
        fw_cfg_read_bytes((uint8_t *)&file.name, sizeof(file.name));

        if (!strcmp(file.name, name)) {
            *store_select = file.select;
            *store_size = file.size;
            return true;
        }
    }
    return false;
}

void init_fw_cfg() {
    uint16_t ramfb_select;
    uint32_t ramfb_size;
    ramfb_config config;
    struct {
        uint64_t base_addr;
        uint64_t size;
    } __packed fw_cfg;
    size_t fbsize;

    if(!fdtree_find_prop("fw-cfg@", "reg", &fw_cfg, sizeof(fw_cfg))) {
        panic("did not find fw-cfg in device tree");
    }
    fw_cfg.base_addr = __bswap64(fw_cfg.base_addr);
    fw_cfg.size = __bswap64(fw_cfg.size);
    if (fw_cfg.size != 0x18) {
        panic("fw_cfg info has unexpected size (%#llx)", fw_cfg.size);
    }

    // round size to next pagesize
    if(is_16k()) {
        fw_cfg.size = (fw_cfg.size + 0x3fffULL) & ~0x3fffULL;
    } else {
        fw_cfg.size = (fw_cfg.size + 0xfffULL) & ~0xfffULL;
    }
    gFwCfgBase = (void *)fw_cfg.base_addr;

    fw_cfg_select(FW_CFG_SIGNATURE);
    if (fw_cfg_read32() != 0x51454d55) { // 'QEMU'
        panic("could not detect QEMU fw_cfg");
    }

    if (*((uint64_t *) FW_CFG_DMA) != 0x47464320554d4551) {
        panic("DMA interface is not available");
    }

    if (!fw_cfg_find_file("etc/ramfb", &ramfb_select, &ramfb_size)) {
        panic("did not find ramfb device");
    }

    gBootArgs->Video.v_width = 800;
    gBootArgs->Video.v_height = 600;
    fbsize = gBootArgs->Video.v_width * gBootArgs->Video.v_height * RAMFB_BPP;
    void *fbmem = alloc_static(fbsize);
    uint64_t fbmem_phys = (uint64_t)vatophys_static(fbmem);
    gBootArgs->Video.v_baseAddr = fbmem_phys;
    gBootArgs->Video.v_rowBytes = gBootArgs->Video.v_width * RAMFB_BPP;
    gBootArgs->Video.v_depth = 32;

    // configure frame buffer
    config.address = __bswap64(fbmem_phys);
    config.fourCC = __bswap32(RAMFB_FORMAT);
    config.flags = __bswap32(0);
    config.width = __bswap32(gBootArgs->Video.v_width);
    config.height = __bswap32(gBootArgs->Video.v_height);
    config.stride = __bswap32(gBootArgs->Video.v_rowBytes);

    fw_cfg_select(ramfb_select);
    fw_cfg_write_dma((uint8_t *)&config, sizeof(config));

    // clear fb memory
    extern volatile void smemset(void*, uint8_t, uint64_t);
    smemset(fbmem, 0, fbsize);
}