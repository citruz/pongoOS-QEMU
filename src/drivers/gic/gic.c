
#import <pongo.h>
#import "gic.h"

// This is taken in large parts from https://github.com/ARM-software/arm-trusted-firmware/blob/master/drivers/arm/gic/v2/

#define mmio_read_32(x) *((volatile uint32_t *)(x))
#define mmio_write_32(x, val) *((volatile uint32_t *)(x)) = (val)
#define mmio_read_8(x) *((volatile uint8_t *)(x))
#define mmio_write_8(x, val) *((volatile uint8_t *)(x)) = (val)

#define INTR_PROP_DESC(num, pri, grp, cfg) \
	{ \
		.intr_num = (num), \
		.intr_pri = (pri), \
		.intr_grp = (grp), \
		.intr_cfg = (cfg), \
	}

typedef struct interrupt_prop {
	unsigned int intr_num:10;
	unsigned int intr_pri:8;
	unsigned int intr_grp:2;
	unsigned int intr_cfg:2;
} interrupt_prop_t;

#define CSS_G1S_IRQ_PROPS(grp) \
	INTR_PROP_DESC(CSS_IRQ_MHU, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_LEVEL), \
	INTR_PROP_DESC(CSS_IRQ_GPU_SMMU_0, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_LEVEL), \
	INTR_PROP_DESC(CSS_IRQ_TZC, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_LEVEL), \
	INTR_PROP_DESC(CSS_IRQ_TZ_WDOG, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_LEVEL), \
	INTR_PROP_DESC(CSS_IRQ_SEC_SYS_TIMER, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_LEVEL)

#define ARM_G1S_IRQ_PROPS(grp) \
	INTR_PROP_DESC(ARM_IRQ_SEC_PHY_TIMER, GIC_HIGHEST_SEC_PRIORITY, (grp), \
			GIC_INTR_CFG_LEVEL), \
	INTR_PROP_DESC(ARM_IRQ_SEC_SGI_1, GIC_HIGHEST_SEC_PRIORITY, (grp), \
			GIC_INTR_CFG_EDGE), \
	INTR_PROP_DESC(ARM_IRQ_SEC_SGI_2, GIC_HIGHEST_SEC_PRIORITY, (grp), \
			GIC_INTR_CFG_EDGE), \
	INTR_PROP_DESC(ARM_IRQ_SEC_SGI_3, GIC_HIGHEST_SEC_PRIORITY, (grp), \
			GIC_INTR_CFG_EDGE), \
	INTR_PROP_DESC(ARM_IRQ_SEC_SGI_4, GIC_HIGHEST_SEC_PRIORITY, (grp), \
			GIC_INTR_CFG_EDGE), \
	INTR_PROP_DESC(ARM_IRQ_SEC_SGI_5, GIC_HIGHEST_SEC_PRIORITY, (grp), \
			GIC_INTR_CFG_EDGE), \
	INTR_PROP_DESC(ARM_IRQ_SEC_SGI_7, GIC_HIGHEST_SEC_PRIORITY, (grp), \
			GIC_INTR_CFG_EDGE)

#define ARM_G0_IRQ_PROPS(grp) \
	INTR_PROP_DESC(ARM_IRQ_SEC_SGI_0, PLAT_SDEI_NORMAL_PRI, (grp), \
			GIC_INTR_CFG_EDGE), \
	INTR_PROP_DESC(ARM_IRQ_SEC_SGI_6, GIC_HIGHEST_SEC_PRIORITY, (grp), \
			GIC_INTR_CFG_EDGE)

#define PLAT_ARM_G1S_IRQ_PROPS(grp)	ARM_G1S_IRQ_PROPS(grp)
#define PLAT_ARM_G0_IRQ_PROPS(grp)	ARM_G0_IRQ_PROPS(grp)

struct {
    uint64_t dist_base;
    uint64_t cpu_base;
	unsigned int interrupt_props_num;
    interrupt_prop_t interrupt_props[];
} gGicInfo = {
    .dist_base = 0,
    .interrupt_props_num = 2,
    .interrupt_props = {
        PLAT_ARM_G1S_IRQ_PROPS(GICV2_INTR_GROUP0),
	    PLAT_ARM_G0_IRQ_PROPS(GICV2_INTR_GROUP0)
    },
};

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

unsigned int gicd_read_isenabler(uintptr_t base, unsigned int id)
{
	unsigned int n = id >> ISENABLER_SHIFT;

	return mmio_read_32(base + GICD_ISENABLER + (n << 2));
}


unsigned int gicd_read_igroupr(uintptr_t base, unsigned int id)
{
	unsigned int n = id >> IGROUPR_SHIFT;

	return mmio_read_32(base + GICD_IGROUPR + (n << 2));
}


void gicd_clr_igroupr(uintptr_t base, unsigned int id)
{
	unsigned int bit_num = id & ((1U << IGROUPR_SHIFT) - 1U);
	unsigned int reg_val = gicd_read_igroupr(base, id);

	gicd_write_igroupr(base, id, reg_val & ~(1U << bit_num));
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

void gicd_write_icenabler(uintptr_t base, unsigned int id, unsigned int val)
{
	unsigned int n = id >> ICENABLER_SHIFT;

	mmio_write_32(base + GICD_ICENABLER + (n << 2), val);
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


void gicv2_enable_interrupt(unsigned int id) {
    asm volatile("dsb ishst");
	gicd_set_isenabler(gGicInfo.dist_base, id);
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

void gicv2_secure_spis_configure_props(uintptr_t gicd_base,
		const interrupt_prop_t *interrupt_props,
		unsigned int interrupt_props_num)
{
	unsigned int i;
	const interrupt_prop_t *prop_desc;

	for (i = 0; i < interrupt_props_num; i++) {
		prop_desc = &interrupt_props[i];

		if (prop_desc->intr_num < MIN_SPI_ID)
			continue;

		/* Configure this interrupt as a secure interrupt */
		gicd_clr_igroupr(gicd_base, prop_desc->intr_num);

		/* Set the priority of this interrupt */
		gicd_set_ipriorityr(gicd_base, prop_desc->intr_num,
				prop_desc->intr_pri);

		/* Target the secure interrupts to primary CPU */
		gicd_set_itargetsr(gicd_base, prop_desc->intr_num,
				gicv2_get_cpuif_id(gicd_base));

		/* Set interrupt configuration */
		gicd_set_icfgr(gicd_base, prop_desc->intr_num,
				prop_desc->intr_cfg);

		/* Enable this interrupt */
		gicd_set_isenabler(gicd_base, prop_desc->intr_num);
	}
}

void gicv2_secure_ppi_sgi_setup_props(uintptr_t gicd_base,
		const interrupt_prop_t *interrupt_props,
		unsigned int interrupt_props_num)
{
	unsigned int i;
	uint32_t sec_ppi_sgi_mask = 0;
	const interrupt_prop_t *prop_desc;

	/*
	 * Disable all SGIs (imp. def.)/PPIs before configuring them. This is a
	 * more scalable approach as it avoids clearing the enable bits in the
	 * GICD_CTLR.
	 */
	gicd_write_icenabler(gicd_base, 0U, ~0U);

	/* Setup the default PPI/SGI priorities doing four at a time */
	for (i = 0U; i < MIN_SPI_ID; i += 4U)
		gicd_write_ipriorityr(gicd_base, i, GICD_IPRIORITYR_DEF_VAL);

	for (i = 0U; i < interrupt_props_num; i++) {
		prop_desc = &interrupt_props[i];

		if (prop_desc->intr_num >= MIN_SPI_ID)
			continue;

		/*
		 * Set interrupt configuration for PPIs. Configuration for SGIs
		 * are ignored.
		 */
		if ((prop_desc->intr_num >= MIN_PPI_ID) &&
				(prop_desc->intr_num < MIN_SPI_ID)) {
			gicd_set_icfgr(gicd_base, prop_desc->intr_num,
					prop_desc->intr_cfg);
		}

		/* We have an SGI or a PPI. They are Group0 at reset */
		sec_ppi_sgi_mask |= (1u << prop_desc->intr_num);

		/* Set the priority of this interrupt */
		gicd_set_ipriorityr(gicd_base, prop_desc->intr_num,
				prop_desc->intr_pri);
	}

	/*
	 * Invert the bitmask to create a mask for non-secure PPIs and SGIs.
	 * Program the GICD_IGROUPR0 with this bit mask.
	 */
	gicd_write_igroupr(gicd_base, 0, ~sec_ppi_sgi_mask);

	/* Enable the Group 0 SGIs and PPIs */
	gicd_write_isenabler(gicd_base, 0, sec_ppi_sgi_mask);
}

void gicv2_distif_init(void)
{
	unsigned int ctlr;

	/* Disable the distributor before going further */
	ctlr = gicd_read_ctlr(gGicInfo.dist_base);
	gicd_write_ctlr(gGicInfo.dist_base, ctlr & ~(CTLR_ENABLE_G0_BIT | CTLR_ENABLE_G1_BIT));

	/* Set the default attribute of all SPIs */
	gicv2_spis_configure_defaults(gGicInfo.dist_base);

	gicv2_secure_spis_configure_props(gGicInfo.dist_base,
			gGicInfo.interrupt_props,
			gGicInfo.interrupt_props_num);


	/* Re-enable the secure SPIs now that they have been configured */
	gicd_write_ctlr(gGicInfo.dist_base, ctlr | CTLR_ENABLE_G0_BIT| CTLR_ENABLE_G1_BIT);
}

void gicv2_pcpu_distif_init(void)
{
	unsigned int ctlr;

	gicv2_secure_ppi_sgi_setup_props(gGicInfo.dist_base,
			gGicInfo.interrupt_props,
			gGicInfo.interrupt_props_num);

	/* Enable G0 interrupts if not already */
	ctlr = gicd_read_ctlr(gGicInfo.dist_base);
	if ((ctlr & CTLR_ENABLE_G0_BIT) == 0U) {
		gicd_write_ctlr(gGicInfo.dist_base,
				ctlr | CTLR_ENABLE_G0_BIT);
	}
}

void gicv2_cpuif_enable(void)
{
	unsigned int val;

	/*
	 * Enable the Group 0 interrupts, FIQEn and disable Group 0/1
	 * bypass.
	 */
	val = CTLR_ENABLE_G0_BIT | CTLR_ENABLE_G1_BIT | FIQ_EN_BIT | FIQ_BYP_DIS_GRP0 | ACK_CTL;
	val |= IRQ_BYP_DIS_GRP0 | FIQ_BYP_DIS_GRP1 | IRQ_BYP_DIS_GRP1;

	/* Program the idle priority in the PMR */
	gicc_write_pmr(gGicInfo.cpu_base, GIC_PRI_MASK);
	gicc_write_ctlr(gGicInfo.cpu_base, val);
}

void interrupt_init() {
    puts("gic interrupt_init");
     struct {
        uint64_t dist_addr;
        uint64_t dist_size;
        uint64_t cpu_addr;
        uint64_t cpu_size;
    } __packed gic_base;

    if (!fdtree_find_prop("intc@", "reg", &gic_base, sizeof(gic_base))) {
        screen_puts("did not find gic in device tree");
    }
    gic_base.dist_addr = __bswap64(gic_base.dist_addr);
    gic_base.dist_size = __bswap64(gic_base.dist_size);
    gic_base.cpu_addr = __bswap64(gic_base.cpu_addr);
    gic_base.cpu_size = __bswap64(gic_base.cpu_size);

    unsigned int gic_version = gicd_read_pidr2(gic_base.dist_addr);
    gic_version = (gic_version >> PIDR2_ARCH_REV_SHIFT) & PIDR2_ARCH_REV_MASK;
    char buf[100];
    snprintf(buf, sizeof(buf), "gic addr=%#llx size=%#llx version=%u", gic_base.dist_addr, gic_base.dist_size, gic_version);
    puts(buf);

    if (gic_version != 2) {
        panic("gic version not supported");
    }

    gGicInfo.dist_base = gic_base.dist_addr;
    gGicInfo.cpu_base = gic_base.cpu_addr;

    gicv2_distif_init();
    gicv2_pcpu_distif_init();
	gicv2_cpuif_enable();

    puts("gic initialized");
}
void interrupt_teardown() {
}

void unmask_interrupt(uint32_t reg) {
    interrupt_prop_t prop_desc = {
        .intr_num = reg,
        .intr_cfg = GIC_INTR_CFG_EDGE,
        .intr_pri = GIC_HIGHEST_SEC_PRIORITY,
    };
    uintptr_t gicd_base = gGicInfo.dist_base;
    /* Set the priority of this interrupt */
    gicd_set_ipriorityr(gicd_base, prop_desc.intr_num, prop_desc.intr_pri);

    /* Target the secure interrupts to primary CPU */
    gicd_set_itargetsr(gicd_base, prop_desc.intr_num, gicv2_get_cpuif_id(gicd_base));

    /* Set interrupt configuration */
    gicd_set_icfgr(gicd_base, prop_desc.intr_num, prop_desc.intr_cfg);

    /* Enable this interrupt */
    gicd_set_isenabler(gicd_base, prop_desc.intr_num);
    //gicv2_enable_interrupt(reg);
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
