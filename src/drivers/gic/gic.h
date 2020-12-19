#ifndef GIC_H
#define GIC_H

uint32_t interrupt_vector();

#define BIT_32(nr)			(U(1) << (nr))
#define BIT_64(nr)			(ULL(1) << (nr))


/*******************************************************************************
 * GIC Distributor interface general definitions
 ******************************************************************************/
/* Constants to categorise interrupts */
#define MIN_SGI_ID		U(0)
#define MIN_SEC_SGI_ID		U(8)
#define MIN_PPI_ID		U(16)
#define MIN_SPI_ID		U(32)
#define MAX_SPI_ID		U(1019)

#define TOTAL_SPI_INTR_NUM	(MAX_SPI_ID - MIN_SPI_ID + U(1))
#define TOTAL_PCPU_INTR_NUM	(MIN_SPI_ID - MIN_SGI_ID)

/* Mask for the priority field common to all GIC interfaces */
#define GIC_PRI_MASK			U(0xff)

/* Mask for the configuration field common to all GIC interfaces */
#define GIC_CFG_MASK			U(0x3)

/* Constant to indicate a spurious interrupt in all GIC versions */
#define GIC_SPURIOUS_INTERRUPT		U(1023)

/* Interrupt configurations: 2-bit fields with LSB reserved */
#define GIC_INTR_CFG_LEVEL		(0 << 1)
#define GIC_INTR_CFG_EDGE		(1 << 1)

/* Highest possible interrupt priorities */
#define GIC_HIGHEST_SEC_PRIORITY	U(0x00)
#define GIC_HIGHEST_NS_PRIORITY		U(0x80)

/*******************************************************************************
 * Common GIC Distributor interface register offsets
 ******************************************************************************/
#define GICD_CTLR		U(0x0)
#define GICD_TYPER		U(0x4)
#define GICD_IIDR		U(0x8)
#define GICD_IGROUPR		U(0x80)
#define GICD_ISENABLER		U(0x100)
#define GICD_ICENABLER		U(0x180)
#define GICD_ISPENDR		U(0x200)
#define GICD_ICPENDR		U(0x280)
#define GICD_ISACTIVER		U(0x300)
#define GICD_ICACTIVER		U(0x380)
#define GICD_IPRIORITYR		U(0x400)
#define GICD_ICFGR		U(0xc00)
#define GICD_NSACR		U(0xe00)

/* GICD_CTLR bit definitions */
#define CTLR_ENABLE_G0_SHIFT		0
#define CTLR_ENABLE_G0_MASK		U(0x1)
#define CTLR_ENABLE_G0_BIT		BIT_32(CTLR_ENABLE_G0_SHIFT)

#define CTLR_ENABLE_G1_SHIFT		1
#define CTLR_ENABLE_G1_MASK		U(0x1)
#define CTLR_ENABLE_G1_BIT		BIT_32(CTLR_ENABLE_G1_SHIFT)

/* Interrupt ID mask for HPPIR, AHPPIR, IAR and AIAR CPU Interface registers */
#define INT_ID_MASK		U(0x3ff)

/* GICC_CTLR bit definitions */
#define EOI_MODE_NS		BIT_32(10)
#define EOI_MODE_S		BIT_32(9)
#define IRQ_BYP_DIS_GRP1	BIT_32(8)
#define FIQ_BYP_DIS_GRP1	BIT_32(7)
#define IRQ_BYP_DIS_GRP0	BIT_32(6)
#define FIQ_BYP_DIS_GRP0	BIT_32(5)
#define CBPR			BIT_32(4)
#define FIQ_EN_SHIFT		3
#define FIQ_EN_BIT		BIT_32(FIQ_EN_SHIFT)
#define ACK_CTL			BIT_32(2)

/*******************************************************************************
 * Common GIC Distributor interface register constants
 ******************************************************************************/
#define PIDR2_ARCH_REV_SHIFT	4
#define PIDR2_ARCH_REV_MASK	U(0xf)

/* GIC revision as reported by PIDR2.ArchRev register field */
#define ARCH_REV_GICV1		U(0x1)
#define ARCH_REV_GICV2		U(0x2)
#define ARCH_REV_GICV3		U(0x3)
#define ARCH_REV_GICV4		U(0x4)

#define IGROUPR_SHIFT		5
#define ISENABLER_SHIFT		5
#define ICENABLER_SHIFT		ISENABLER_SHIFT
#define ISPENDR_SHIFT		5
#define ICPENDR_SHIFT		ISPENDR_SHIFT
#define ISACTIVER_SHIFT		5
#define ICACTIVER_SHIFT		ISACTIVER_SHIFT
#define IPRIORITYR_SHIFT	2
#define ITARGETSR_SHIFT		2
#define ICFGR_SHIFT		4
#define NSACR_SHIFT		4

/* GICD_TYPER shifts and masks */
#define TYPER_IT_LINES_NO_SHIFT	U(0)
#define TYPER_IT_LINES_NO_MASK	U(0x1f)

/* Value used to initialize Normal world interrupt priorities four at a time */
#define GICD_IPRIORITYR_DEF_VAL			\
	(GIC_HIGHEST_NS_PRIORITY)	|	\
	(GIC_HIGHEST_NS_PRIORITY << 8)	|	\
	(GIC_HIGHEST_NS_PRIORITY << 16)	|	\
	(GIC_HIGHEST_NS_PRIORITY << 24)


/*******************************************************************************
 * GICv2 miscellaneous definitions
 ******************************************************************************/

/* Interrupt group definitions */
#define GICV2_INTR_GROUP0	U(0)
#define GICV2_INTR_GROUP1	U(1)

/* Interrupt IDs reported by the HPPIR and IAR registers */
#define PENDING_G1_INTID	U(1022)

/* GICv2 can only target up to 8 PEs */
#define GICV2_MAX_TARGET_PE	U(8)

/*******************************************************************************
 * GICv2 specific Distributor interface register offsets and constants.
 ******************************************************************************/
#define GICD_ITARGETSR		U(0x800)
#define GICD_SGIR		U(0xF00)
#define GICD_CPENDSGIR		U(0xF10)
#define GICD_SPENDSGIR		U(0xF20)
#define GICD_PIDR2_GICV2	U(0xFE8)

#define ITARGETSR_SHIFT		2
#define GIC_TARGET_CPU_MASK	U(0xff)

#define CPENDSGIR_SHIFT		2
#define SPENDSGIR_SHIFT		CPENDSGIR_SHIFT

#define SGIR_TGTLSTFLT_SHIFT	24
#define SGIR_TGTLSTFLT_MASK	U(0x3)
#define SGIR_TGTLST_SHIFT	16
#define SGIR_TGTLST_MASK	U(0xff)
#define SGIR_INTID_MASK		ULL(0xf)

#define SGIR_TGT_SPECIFIC	U(0)

#define GICV2_SGIR_VALUE(tgt_lst_flt, tgt, intid) \
	((((tgt_lst_flt) & SGIR_TGTLSTFLT_MASK) << SGIR_TGTLSTFLT_SHIFT) | \
	 (((tgt) & SGIR_TGTLST_MASK) << SGIR_TGTLST_SHIFT) | \
	 ((intid) & SGIR_INTID_MASK))

#define IGROUPR_SHIFT		5
#define ISENABLER_SHIFT		5
#define ICENABLER_SHIFT		ISENABLER_SHIFT
#define ISPENDR_SHIFT		5
#define ICPENDR_SHIFT		ISPENDR_SHIFT
#define ISACTIVER_SHIFT		5
#define ICACTIVER_SHIFT		ISACTIVER_SHIFT
#define IPRIORITYR_SHIFT	2
#define ITARGETSR_SHIFT		2
#define ICFGR_SHIFT		4
#define NSACR_SHIFT		4

/*******************************************************************************
 * GICv2 specific CPU interface register offsets and constants.
 ******************************************************************************/
/* Physical CPU Interface registers */
#define GICC_CTLR		U(0x0)
#define GICC_PMR		U(0x4)
#define GICC_BPR		U(0x8)
#define GICC_IAR		U(0xC)
#define GICC_EOIR		U(0x10)
#define GICC_RPR		U(0x14)
#define GICC_HPPIR		U(0x18)
#define GICC_AHPPIR		U(0x28)
#define GICC_IIDR		U(0xFC)
#define GICC_DIR		U(0x1000)
#define GICC_PRIODROP		GICC_EOIR

/* GICC_CTLR bit definitions */
#define EOI_MODE_NS		BIT_32(10)
#define EOI_MODE_S		BIT_32(9)
#define IRQ_BYP_DIS_GRP1	BIT_32(8)
#define FIQ_BYP_DIS_GRP1	BIT_32(7)
#define IRQ_BYP_DIS_GRP0	BIT_32(6)
#define FIQ_BYP_DIS_GRP0	BIT_32(5)
#define CBPR			BIT_32(4)
#define FIQ_EN_SHIFT		3
#define FIQ_EN_BIT		BIT_32(FIQ_EN_SHIFT)
#define ACK_CTL			BIT_32(2)

/* GICC_IIDR bit masks and shifts */
#define GICC_IIDR_PID_SHIFT	20
#define GICC_IIDR_ARCH_SHIFT	16
#define GICC_IIDR_REV_SHIFT	12
#define GICC_IIDR_IMP_SHIFT	0

#define GICC_IIDR_PID_MASK	U(0xfff)
#define GICC_IIDR_ARCH_MASK	U(0xf)
#define GICC_IIDR_REV_MASK	U(0xf)
#define GICC_IIDR_IMP_MASK	U(0xfff)

/* Interrupt handling constants */
#define CSS_IRQ_MHU			69
#define CSS_IRQ_GPU_SMMU_0		71
#define CSS_IRQ_TZC			80
#define CSS_IRQ_TZ_WDOG			86
#define CSS_IRQ_SEC_SYS_TIMER		91

#define ARM_IRQ_SEC_PHY_TIMER		29

#define ARM_IRQ_SEC_SGI_0		8
#define ARM_IRQ_SEC_SGI_1		9
#define ARM_IRQ_SEC_SGI_2		10
#define ARM_IRQ_SEC_SGI_3		11
#define ARM_IRQ_SEC_SGI_4		12
#define ARM_IRQ_SEC_SGI_5		13
#define ARM_IRQ_SEC_SGI_6		14
#define ARM_IRQ_SEC_SGI_7		15

/* Priority levels for ARM platforms */
#define PLAT_RAS_PRI			0x10
#define PLAT_SDEI_CRITICAL_PRI		0x60
#define PLAT_SDEI_NORMAL_PRI		0x70

#endif