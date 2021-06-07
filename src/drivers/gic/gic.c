
#import <pongo.h>
#import "gic.h"

// This is taken in large parts from https://github.com/ARM-software/arm-trusted-firmware/blob/master/drivers/arm/gic/v2/

#define mmio_read_32(x) *((volatile uint32_t *)(x))
#define mmio_write_32(x, val) *((volatile uint32_t *)(x)) = (val)
#define mmio_read_8(x) *((volatile uint8_t *)(x))
#define mmio_write_8(x, val) *((volatile uint8_t *)(x)) = (val)

struct {
    uint64_t dist_base;
    uint64_t cpu_base;
} gGicInfo;

static inline unsigned int gicd_read_pidr2(uintptr_t base)
{
	return mmio_read_32(base + GICD_PIDR2_GICV2);
}

static inline unsigned int gicd_read_ctlr(uintptr_t base)
{
	return mmio_read_32(base + GICD_CTLR);
}

static inline void gicd_write_ctlr(uintptr_t base, unsigned int val)
{
	mmio_write_32(base + GICD_CTLR, val);
}

static inline unsigned int gicd_read_typer(uintptr_t base)
{
	return mmio_read_32(base + GICD_TYPER);
}

void gicd_write_igroupr(uintptr_t base, unsigned int id, unsigned int val)
{
	unsigned int n = id >> IGROUPR_SHIFT;

	mmio_write_32(base + GICD_IGROUPR + (n << 2), val);
}

void gicd_write_ipriorityr(uintptr_t base, unsigned int id, unsigned int val)
{
	unsigned int n = id >> IPRIORITYR_SHIFT;

	mmio_write_32(base + GICD_IPRIORITYR + (n << 2), val);
}

void gicd_set_ipriorityr(uintptr_t base, unsigned int id, unsigned int pri)
{
	uint8_t val = pri & GIC_PRI_MASK;

	mmio_write_8(base + GICD_IPRIORITYR + id, val);
}

void gicd_write_icfgr(uintptr_t base, unsigned int id, unsigned int val)
{
	unsigned int n = id >> ICFGR_SHIFT;

	mmio_write_32(base + GICD_ICFGR + (n << 2), val);
}

unsigned int gicd_read_icfgr(uintptr_t base, unsigned int id)
{
	unsigned int n = id >> ICFGR_SHIFT;

	return mmio_read_32(base + GICD_ICFGR + (n << 2));
}


void gicd_set_icfgr(uintptr_t base, unsigned int id, unsigned int cfg)
{
	/* Interrupt configuration is a 2-bit field */
	unsigned int bit_num = id & ((1U << ICFGR_SHIFT) - 1U);
	unsigned int bit_shift = bit_num << 1;

	uint32_t reg_val = gicd_read_icfgr(base, id);

	/* Clear the field, and insert required configuration */
	reg_val &= ~(GIC_CFG_MASK << bit_shift);
	reg_val |= ((cfg & GIC_CFG_MASK) << bit_shift);

	gicd_write_icfgr(base, id, reg_val);
}

void gicd_write_isenabler(uintptr_t base, unsigned int id, unsigned int val)
{
	unsigned int n = id >> ISENABLER_SHIFT;

	mmio_write_32(base + GICD_ISENABLER + (n << 2), val);
}

void gicd_set_isenabler(uintptr_t base, unsigned int id) {
	unsigned int bit_num = id & ((1U << ISENABLER_SHIFT) - 1U);

	gicd_write_isenabler(base, id, (1U << bit_num));
}

static inline void gicd_set_itargetsr(uintptr_t base, unsigned int id,
		unsigned int target)
{
	uint8_t val = target & GIC_TARGET_CPU_MASK;

	mmio_write_8(base + GICD_ITARGETSR + id, val);
}

unsigned int gicd_read_itargetsr(uintptr_t base, unsigned int id)
{
	unsigned n = id >> ITARGETSR_SHIFT;
	return mmio_read_32(base + GICD_ITARGETSR + (n << 2));
}

unsigned int gicv2_get_cpuif_id(uintptr_t base)
{
	unsigned int val;

	val = gicd_read_itargetsr(base, 0);
	return val & GIC_TARGET_CPU_MASK;
}

static inline void gicc_write_pmr(uintptr_t base, unsigned int val)
{
	mmio_write_32(base + GICC_PMR, val);
}

static inline void gicc_write_ctlr(uintptr_t base, unsigned int val)
{
	mmio_write_32(base + GICC_CTLR, val);
}

static inline unsigned int gicc_read_IAR(uintptr_t base)
{
	return mmio_read_32(base + GICC_IAR);
}

static inline unsigned int gicc_read_hppir(uintptr_t base)
{
	return mmio_read_32(base + GICC_HPPIR);
}

static inline unsigned int gicc_read_ahppir(uintptr_t base)
{
	return mmio_read_32(base + GICC_AHPPIR);
}

static inline void gicc_write_EOIR(uintptr_t base, unsigned int val)
{
	mmio_write_32(base + GICC_EOIR, val);
}

void gicv2_spis_configure_defaults(uintptr_t gicd_base)
{
	unsigned int index, num_ints;

	num_ints = gicd_read_typer(gicd_base);
	num_ints &= TYPER_IT_LINES_NO_MASK;
	num_ints = (num_ints + 1U) << 5;

	/*
	 * Treat all SPIs as G1NS by default. The number of interrupts is
	 * calculated as 32 * (IT_LINES + 1). We do 32 at a time.
	 */
	for (index = MIN_SPI_ID; index < num_ints; index += 32U)
		gicd_write_igroupr(gicd_base, index, ~0U);

	/* Setup the default SPI priorities doing four at a time */
	for (index = MIN_SPI_ID; index < num_ints; index += 4U)
		gicd_write_ipriorityr(gicd_base,
				      index,
				      GICD_IPRIORITYR_DEF_VAL);

	/* Treat all SPIs as level triggered by default, 16 at a time */
	for (index = MIN_SPI_ID; index < num_ints; index += 16U)
		gicd_write_icfgr(gicd_base, index, 0U);
}

void gicv2_distif_init(void)
{
	unsigned int ctlr;

	/* Disable the distributor before going further */
	ctlr = gicd_read_ctlr(gGicInfo.dist_base);
	gicd_write_ctlr(gGicInfo.dist_base, ctlr & ~(CTLR_ENABLE_G0_BIT | CTLR_ENABLE_G1_BIT));

	/* Set the default attribute of all SPIs */
	gicv2_spis_configure_defaults(gGicInfo.dist_base);

	/* Re-enable the SPIs now that they have been configured */
	gicd_write_ctlr(gGicInfo.dist_base, ctlr | CTLR_ENABLE_G0_BIT | CTLR_ENABLE_G1_BIT);
}

void gicv2_cpuif_enable(void)
{
	unsigned int val;

	/*
	 * Enable the Group 0 and 1 interrupts and FIQEn.
	 */
	val = CTLR_ENABLE_G0_BIT | CTLR_ENABLE_G1_BIT | FIQ_EN_BIT | ACK_CTL;

	/* Program the idle priority in the PMR */
	gicc_write_pmr(gGicInfo.cpu_base, GIC_PRI_MASK);
	gicc_write_ctlr(gGicInfo.cpu_base, val);
}

void interrupt_init() {
     struct {
        uint64_t dist_addr;
        uint64_t dist_size;
        uint64_t cpu_addr;
        uint64_t cpu_size;
    } __packed gic_base;

    if (!fdtree_find_prop("intc@", "reg", &gic_base, sizeof(gic_base))) {
        panic("did not find gic in device tree");
    }
    gic_base.dist_addr = __bswap64(gic_base.dist_addr);
    gic_base.dist_size = __bswap64(gic_base.dist_size);
    gic_base.cpu_addr = __bswap64(gic_base.cpu_addr);
    gic_base.cpu_size = __bswap64(gic_base.cpu_size);

    unsigned int gic_version = gicd_read_pidr2(gic_base.dist_addr);
    gic_version = (gic_version >> PIDR2_ARCH_REV_SHIFT) & PIDR2_ARCH_REV_MASK;

#ifdef DEBUG
    char buf[100];
    snprintf(buf, sizeof(buf), "gic addr=%#llx size=%#llx version=%u", gic_base.dist_addr, gic_base.dist_size, gic_version);
    puts(buf);
#endif

    if (gic_version != ARCH_REV_GICV2) {
        //panic("gic version not supported");
    }

    gGicInfo.dist_base = gic_base.dist_addr;
    gGicInfo.cpu_base = gic_base.cpu_addr;

    gicv2_distif_init();
	gicv2_cpuif_enable();
}

void interrupt_teardown() {
	// TODO
}

void unmask_interrupt(uint32_t reg) {
    uintptr_t gicd_base = gGicInfo.dist_base;

    /* Set the priority of this interrupt */
    gicd_set_ipriorityr(gicd_base, reg, GIC_HIGHEST_SEC_PRIORITY);

    /* Target the secure interrupts to primary CPU */
    gicd_set_itargetsr(gicd_base, reg, gicv2_get_cpuif_id(gicd_base));

    /* Set interrupt configuration */
    gicd_set_icfgr(gicd_base, reg, GIC_INTR_CFG_EDGE);

    /* Enable this interrupt */
    gicd_set_isenabler(gicd_base, reg);
}

unsigned int gicv2_get_pending_interrupt_id(void)
{
	unsigned int id;

	id = gicc_read_hppir(gGicInfo.cpu_base) & INT_ID_MASK;

	/*
	 * Find out which non-secure interrupt it is under the assumption that
	 * the GICC_CTLR.AckCtl bit is 0.
	 */
	if (id == PENDING_G1_INTID)
		id = gicc_read_ahppir(gGicInfo.cpu_base) & INT_ID_MASK;

	return id;
}

// get the interrupt number
uint32_t interrupt_vector() {
	uint32_t interrupt_id = gicv2_get_pending_interrupt_id();

	if (interrupt_id == GIC_SPURIOUS_INTERRUPT) {
		return 0;
	}

	// acknowledge interrupt
	gicc_read_IAR(gGicInfo.cpu_base);

	// mark end of interrupt
	__asm volatile("dsb ishst");
	gicc_write_EOIR(gGicInfo.cpu_base, interrupt_id);

	return interrupt_id;
}
