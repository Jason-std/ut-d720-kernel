/* linux/arch/arm/mach-exynos/include/mach/regs-pmu-4212.h
 *
 * Copyright (c) 2010-2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * EXYNOS4 - 4212 Power management unit definition
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ASM_ARCH_REGS_PMU_4212_H
#define __ASM_ARCH_REGS_PMU_4212_H __FILE__

#define S5P_LPI_MASK0				S5P_PMUREG(0x0004)
#define S5P_LPI_MASK1				S5P_PMUREG(0x0008)
#define S5P_LPI_MASK2				S5P_PMUREG(0x000C)

#define S5P_LPI_DENIAL_MASK0			S5P_PMUREG(0x0018)
#define S5P_LPI_DENIAL_MASK1			S5P_PMUREG(0x001C)
#define S5P_LPI_DENIAL_MASK2			S5P_PMUREG(0x0020)

#define S5P_C2C_CTRL				S5P_PMUREG(0x0024)

#define S5P_CENTRAL_SEQ_CONFIGURATION_COREBLK	S5P_PMUREG(0x0240)

#define S5P_CENTRAL_SEQ_COREBLK_CONF		(0x1 << 16)

#define S5P_SYS_WDTRESET			(0x1 << 20)
#define S5P_AUTOMATIC_WDT_RESET_DISABLE		S5P_PMUREG(0x0408)
#define S5P_MASK_WDT_RESET_REQUEST		S5P_PMUREG(0x040C)
#define S5P_SWRESET				S5P_PMUREG(0x0400)
#define S5P_PS_HOLD_CONTROL                     S5P_PMUREG(0x330C)

#define S5P_WAKEUP_STAT_COREBLK			S5P_PMUREG(0x0620)
#define S5P_WAKEUP_MASK_COREBLK			S5P_PMUREG(0x0628)

#define S5P_USB_PHY_CONTROL			S5P_PMUREG(0x0704)
#define S5P_HSIC_1_PHY_CONTROL			S5P_PMUREG(0x0708)
#define S5P_HSIC_2_PHY_CONTROL			S5P_PMUREG(0x070C)
#define S5P_USB_PHY_ENABLE			(0x1 << 0)
#define S5P_HSIC_1_PHY_ENABLE			(0x1 << 0)
#define S5P_HSIC_2_PHY_ENABLE			(0x1 << 0)

#define S5P_ABB_INT				S5P_PMUREG(0x0780)
#define S5P_ABB_MIF				S5P_PMUREG(0x0784)
#define S5P_ABB_G3D				S5P_PMUREG(0x0788)
#define S5P_ABB_ARM				S5P_PMUREG(0x078C)

#define S5P_ABB_MEMBER(_nr)			(S5P_ABB_INT + (_nr * 0x4))

#define ABB_MODE_060V				0
#define ABB_MODE_065V				1
#define ABB_MODE_070V				2
#define ABB_MODE_075V				3
#define ABB_MODE_080V				4
#define ABB_MODE_085V				5
#define ABB_MODE_090V				6
#define ABB_MODE_095V				7
#define ABB_MODE_100V				8
#define ABB_MODE_105V				9
#define ABB_MODE_110V				10
#define ABB_MODE_115V				11
#define ABB_MODE_120V				12
#define ABB_MODE_125V				13
#define ABB_MODE_130V				14
#define ABB_MODE_135V				15
#define ABB_MODE_140V				16
#define ABB_MODE_145V				17
#define ABB_MODE_150V				18
#define ABB_MODE_155V				19
#define ABB_MODE_160V				20
#define ABB_MODE_BYPASS				255

#define S5P_ABB_INIT				(0x80000080)
#define S5P_ABB_INIT_BYPASS			(0x80000000)

#define S5P_PMU_DEBUG				S5P_PMUREG(0x0A00)
#define S5P_PMU_CLKOUT_SEL_MASK         (0xF<<8)
#define S5P_PMU_CLKOUT_SEL_SHIFT        (8)
#define S5P_PMU_CLKOUT_SEL_CMU_DMC      (0x0)
#define S5P_PMU_CLKOUT_SEL_CMU_TOP      (0x1)
#define S5P_PMU_CLKOUT_SEL_CMU_LEFTBUS  (0x2)
#define S5P_PMU_CLKOUT_SEL_CMU_RIGHTBUS (0x3)
#define S5P_PMU_CLKOUT_SEL_CMU_CPU      (0x4)
#define S5P_PMU_CLKOUT_SEL_CMU_ISP      (0x5)
#define S5P_PMU_CLKOUT_SEL_XXTI         (0x8)
#define S5P_PMU_CLKOUT_SEL_XUSBXTI      (0x9)
#define S5P_PMU_CLKOUT_SEL_RTCTICCLK    (0xC)
#define S5P_PMU_CLKOUT_SEL_RTCCLK       (0xD)
#define S5P_PMU_CLKOUT_SEL_CLKOUT_DEBUG (0xE)

/* SYS_PWR registers */
#define S5P_ARM_CORE2_SYS			S5P_PMUREG(0x1020)
#define S5P_DIS_IRQ_ARM_CORE2_LOCAL_SYS		S5P_PMUREG(0x1024)
#define S5P_DIS_IRQ_ARM_CORE2_CENTRAL_SYS	S5P_PMUREG(0x1028)
#define S5P_ARM_CORE3_SYS			S5P_PMUREG(0x1030)
#define S5P_DIS_IRQ_ARM_CORE3_LOCAL_SYS		S5P_PMUREG(0x1034)
#define S5P_DIS_IRQ_ARM_CORE3_CENTRAL_SYS	S5P_PMUREG(0x1038)
#define S5P_ISP_ARM_SYS				S5P_PMUREG(0x1050)
#define S5P_DIS_IRQ_ISP_ARM_LOCAL_SYS		S5P_PMUREG(0x1054)
#define S5P_DIS_IRQ_ISP_ARM_CENTRAL_SYS		S5P_PMUREG(0x1058)
#define S5P_CMU_ACLKSTOP_COREBLK_SYS		S5P_PMUREG(0x1110)
#define S5P_CMU_SCLKSTOP_COREBLK_SYS		S5P_PMUREG(0x1114)
#define S5P_CMU_RESET_COREBLK_SYS		S5P_PMUREG(0x111C)
#define S5P_MPLLUSER_SYSCLK_SYS			S5P_PMUREG(0x1130)
#define S5P_CMU_CLKSTOP_ISP_SYS			S5P_PMUREG(0x1154)
#define S5P_CMU_RESET_ISP_SYS			S5P_PMUREG(0x1174)
#define S5P_TOP_BUS_COREBLK_SYS			S5P_PMUREG(0x1190)
#define S5P_TOP_RETENTION_COREBLK_SYS		S5P_PMUREG(0x1194)
#define S5P_TOP_PWR_COREBLK_SYS			S5P_PMUREG(0x1198)
#define S5P_OSCCLK_GATE_SYS			S5P_PMUREG(0x11A4)
#define S5P_LOGIC_RESET_COREBLK_SYS		S5P_PMUREG(0x11B0)
#define S5P_OSCCLK_GATE_COREBLK_SYS		S5P_PMUREG(0x11B4)
#define S5P_HSI_MEM_SYS				S5P_PMUREG(0x11C4)
#define S5P_ROTATOR_MEM_SYS			S5P_PMUREG(0x11DC)
#define S5P_PAD_RETENTION_GPIO_COREBLK_SYS	S5P_PMUREG(0x123C)
#define S5P_PAD_ISOLATION_COREBLK_SYS		S5P_PMUREG(0x1250)
#define S5P_GPIO_MODE_COREBLK_SYS		S5P_PMUREG(0x1320)
#define S5P_TOP_ASB_RESET_SYS			S5P_PMUREG(0x1344)
#define S5P_TOP_ASB_ISOLATION_SYS		S5P_PMUREG(0x1348)
#define S5P_ISP_SYS				S5P_PMUREG(0x1394)
#define S5P_DRAM_FREQ_DOWN_SYS			S5P_PMUREG(0x13B0)
#define S5P_DDRPHY_DLLOFF_SYS			S5P_PMUREG(0x13B4)
#define S5P_CMU_SYSCLK_ISP_SYS			S5P_PMUREG(0x13B8)
#define S5P_CMU_SYSCLK_GPS_SYS			S5P_PMUREG(0x13BC)
#define S5P_LPDDR_PHY_DLL_LOCK_SYS		S5P_PMUREG(0x13C0)

/* OPTION registers */
#define S5P_ISP_ARM_OPTION			S5P_PMUREG(0x2288)
#define S5P_ARM_L2_0_OPTION			S5P_PMUREG(0x2608)
#define S5P_ARM_L2_1_OPTION			S5P_PMUREG(0x2628)
#define S5P_ONENAND_MEM_OPTION			S5P_PMUREG(0x2E08)
#define S5P_HSI_MEM_OPTION			S5P_PMUREG(0x2E28)
#define S5P_G2D_ACP_MEM_OPTION			S5P_PMUREG(0x2E48)
#define S5P_USBOTG_MEM_OPTION			S5P_PMUREG(0x2E68)
#define S5P_SDMMC_MEM_OPTION			S5P_PMUREG(0x2E88)
#define S5P_CSSYS_MEM_OPTION			S5P_PMUREG(0x2EA8)
#define S5P_SECSS_MEM_OPTION			S5P_PMUREG(0x2EC8)
#define S5P_ROTATOR_MEM_OPTION			S5P_PMUREG(0x2F48)
#define S5P_PAD_RETENTION_GPIO_COREBLK_OPTION	S5P_PMUREG(0x31E8)
#define S5P_PMU_PS_HOLD_CONTROL			S5P_PMUREG(0x330C)

#define S5P_PMU_ISP_CONF			S5P_PMUREG(0x3CA0)
#define S5P_PMU_GPS_ALIVE_CONF			S5P_PMUREG(0x3D00)
#define S5P_PMU_GPS_ALIVE_CONF			S5P_PMUREG(0x3D00)
#define S5P_PMU_MAUDIO_CONF			S5P_PMUREG(0x3CC0)
#define S5P_PS_HOLD_EN				(0x1 << 31)
#endif /* __ASM_ARCH_REGS_PMU_4212_H */
