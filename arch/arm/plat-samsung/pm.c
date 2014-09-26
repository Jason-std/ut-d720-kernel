/* linux/arch/arm/plat-s3c/pm.c
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2004-2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * S3C common power management (suspend to ram) support.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/suspend.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/serial_core.h>
#include <linux/io.h>

#include <asm/cacheflush.h>
#include <mach/hardware.h>
#include <mach/map.h>

#include <plat/regs-serial.h>
#include <mach/regs-clock.h>
#include <mach/regs-irq.h>
#include <asm/irq.h>

#include <plat/pm.h>
#include <mach/pm-core.h>

#include <mach/regs-gpio.h>
#include <mach/regs-pmu.h>
#include <plat/gpio-cfg.h>
#include <mach/gpio.h>
#include <mach/regs-gpio.h>
#include <mach/gpio-exynos4.h>


/* for external use */

unsigned long s3c_pm_flags;
extern char g_selected_utmodel[];
/* Debug code:
 *
 * This code supports debug output to the low level UARTs for use on
 * resume before the console layer is available.
*/
int (*pm_prepare)(void);

#ifdef CONFIG_SAMSUNG_PM_DEBUG
extern void printascii(const char *);

void s3c_pm_dbg(const char *fmt, ...)
{
	va_list va;
	char buff[256];

	va_start(va, fmt);
	vsprintf(buff, fmt, va);
	va_end(va);

#ifdef CONFIG_DEBUG_LL
	printascii(buff);
#endif
}

static inline void s3c_pm_debug_init(void)
{
	/* restart uart clocks so we can use them to output */
	s3c_pm_debug_init_uart();
}

#else
#define s3c_pm_debug_init() do { } while(0)

#endif /* CONFIG_SAMSUNG_PM_DEBUG */

/* Save the UART configurations if we are configured for debug. */

unsigned char pm_uart_udivslot;

#ifdef CONFIG_SAMSUNG_PM_DEBUG

struct pm_uart_save uart_save[CONFIG_SERIAL_SAMSUNG_UARTS];

static void s3c_pm_save_uart(unsigned int uart, struct pm_uart_save *save)
{
	void __iomem *regs = S3C_VA_UARTx(uart);

	save->ulcon = __raw_readl(regs + S3C2410_ULCON);
	save->ucon = __raw_readl(regs + S3C2410_UCON);
	save->ufcon = __raw_readl(regs + S3C2410_UFCON);
	save->umcon = __raw_readl(regs + S3C2410_UMCON);
	save->ubrdiv = __raw_readl(regs + S3C2410_UBRDIV);

	if (pm_uart_udivslot)
		save->udivslot = __raw_readl(regs + S3C2443_DIVSLOT);

	S3C_PMDBG("UART[%d]: ULCON=%04x, UCON=%04x, UFCON=%04x, UBRDIV=%04x\n",
		  uart, save->ulcon, save->ucon, save->ufcon, save->ubrdiv);
}

static void s3c_pm_save_uarts(void)
{
	struct pm_uart_save *save = uart_save;
	unsigned int uart;

	for (uart = 0; uart < CONFIG_SERIAL_SAMSUNG_UARTS; uart++, save++)
		s3c_pm_save_uart(uart, save);
}

static void s3c_pm_restore_uart(unsigned int uart, struct pm_uart_save *save)
{
	void __iomem *regs = S3C_VA_UARTx(uart);

	s3c_pm_arch_update_uart(regs, save);

	__raw_writel(save->ulcon, regs + S3C2410_ULCON);
	__raw_writel(save->ucon,  regs + S3C2410_UCON);
	__raw_writel(save->ufcon, regs + S3C2410_UFCON);
	__raw_writel(save->umcon, regs + S3C2410_UMCON);
	__raw_writel(save->ubrdiv, regs + S3C2410_UBRDIV);

	if (pm_uart_udivslot)
		__raw_writel(save->udivslot, regs + S3C2443_DIVSLOT);
}

static void s3c_pm_restore_uarts(void)
{
	struct pm_uart_save *save = uart_save;
	unsigned int uart;

	for (uart = 0; uart < CONFIG_SERIAL_SAMSUNG_UARTS; uart++, save++)
		s3c_pm_restore_uart(uart, save);
}
#else

#ifdef CONFIG_DEBUG_LL//CONFIG_KERNEL_PANIC_DUMP  //ly 20120412

static volatile  struct pm_uart_save console_uart_save;
//extern int console_suspend_enabled;
static void s3c_pm_save_uarts(void)
{
	void __iomem *regs = S3C_VA_UARTx(CONFIG_S3C_LOWLEVEL_UART_PORT);
	struct pm_uart_save *save = &console_uart_save;
	u32 uart_clk_gate;

//	if(console_suspend_enabled)
//		return;

	/* CONSOLE UART clock may be gated in serial driver suspend, so make sure it be enabled*/
	uart_clk_gate = __raw_readl(EXYNOS4_CLKGATE_IP_PERIL);
	__raw_writel((uart_clk_gate | 0x1 << CONFIG_S3C_LOWLEVEL_UART_PORT), EXYNOS4_CLKGATE_IP_PERIL);

	save->ulcon = __raw_readl(regs + S3C2410_ULCON);
	save->ucon = __raw_readl(regs + S3C2410_UCON);
	save->ufcon = __raw_readl(regs + S3C2410_UFCON);
	save->umcon = __raw_readl(regs + S3C2410_UMCON);
	save->ubrdiv = __raw_readl(regs + S3C2410_UBRDIV);
	save->udivslot = __raw_readl(regs + S3C2443_DIVSLOT);


}
static void s3c_pm_restore_uarts(void)
{
	void __iomem *regs = S3C_VA_UARTx(CONFIG_S3C_LOWLEVEL_UART_PORT);
	struct pm_uart_save *save = &console_uart_save;
	u32 uart_clk_gate;

//	if(console_suspend_enabled)
//		return;

	/* make sure CONSOLE UART clock  it be enabled*/
	uart_clk_gate = __raw_readl(EXYNOS4_CLKGATE_IP_PERIL);
	__raw_writel((uart_clk_gate | 0x1 << CONFIG_S3C_LOWLEVEL_UART_PORT), EXYNOS4_CLKGATE_IP_PERIL);

	__raw_writel(save->ulcon, regs + S3C2410_ULCON);
	__raw_writel(save->ucon,  regs + S3C2410_UCON);
	__raw_writel(save->ufcon, regs + S3C2410_UFCON);
	__raw_writel(save->umcon, regs + S3C2410_UMCON);
	__raw_writel(save->ubrdiv, regs + S3C2410_UBRDIV);
	__raw_writel(save->udivslot, regs + S3C2443_DIVSLOT);
}
/* end */
#else
static void s3c_pm_save_uarts(void) { }
static void s3c_pm_restore_uarts(void) { }
#endif
#endif

/* The IRQ ext-int code goes here, it is too small to currently bother
 * with its own file. */

unsigned long s3c_irqwake_intmask	= 0xffffffffL;
unsigned long s3c_irqwake_eintmask	= 0xffffffffL;

unsigned int wake_type0 = 0;
unsigned int wake_type1 = 0;
unsigned int wake_type2 = 0;
unsigned int wake_type3 = 0;

int s3c_irqext_wake(struct irq_data *data, unsigned int state)
{
	unsigned long bit = 1L << IRQ_EINT_BIT(data->irq);

	if (!(s3c_irqwake_eintallow & bit))
		return -ENOENT;

	printk(KERN_INFO "wake %s for irq %d\n",
	       state ? "enabled" : "disabled", data->irq);

	if (!state)
		s3c_irqwake_eintmask |= bit;
	else
		s3c_irqwake_eintmask &= ~bit;

	return 0;
}

/* helper functions to save and restore register state */

/**
 * s3c_pm_do_save() - save a set of registers for restoration on resume.
 * @ptr: Pointer to an array of registers.
 * @count: Size of the ptr array.
 *
 * Run through the list of registers given, saving their contents in the
 * array for later restoration when we wakeup.
 */
void s3c_pm_do_save(struct sleep_save *ptr, int count)
{
	for (; count > 0; count--, ptr++) {
		ptr->val = __raw_readl(ptr->reg);
		S3C_PMDBG("saved %p value %08lx\n", ptr->reg, ptr->val);
	}
}

/**
 * s3c_pm_do_restore() - restore register values from the save list.
 * @ptr: Pointer to an array of registers.
 * @count: Size of the ptr array.
 *
 * Restore the register values saved from s3c_pm_do_save().
 *
 * Note, we do not use S3C_PMDBG() in here, as the system may not have
 * restore the UARTs state yet
*/

void s3c_pm_do_restore(struct sleep_save *ptr, int count)
{
	for (; count > 0; count--, ptr++) {
		printk(KERN_DEBUG "restore %p (restore %08lx, was %08x)\n",
		       ptr->reg, ptr->val, __raw_readl(ptr->reg));

		__raw_writel(ptr->val, ptr->reg);
	}
}

/**
 * s3c_pm_do_restore_core() - early restore register values from save list.
 *
 * This is similar to s3c_pm_do_restore() except we try and minimise the
 * side effects of the function in case registers that hardware might need
 * to work has been restored.
 *
 * WARNING: Do not put any debug in here that may effect memory or use
 * peripherals, as things may be changing!
*/

void s3c_pm_do_restore_core(struct sleep_save *ptr, int count)
{
	for (; count > 0; count--, ptr++)
		__raw_writel(ptr->val, ptr->reg);
}

/* s3c2410_pm_show_resume_irqs
 *
 * print any IRQs asserted at resume time (ie, we woke from)
*/
static void __maybe_unused s3c_pm_show_resume_irqs(int start,
						   unsigned long which,
						   unsigned long mask)
{
	int i;

	which &= ~mask;

	for (i = 0; i <= 31; i++) {
		if (which & (1L<<i)) {
			S3C_PMDBG("IRQ %d asserted at resume\n", start+i);
		}
	}
}


void (*pm_cpu_prep)(void);
void (*pm_cpu_sleep)(void);
void (*pm_cpu_restore)(void);

#define any_allowed(mask, allow) (((mask) & (allow)) != (allow))

/* s3c_pm_enter
 *
 * central control for sleep/resume process
*/

#if 1
/******************************************************************************
 * ut4412_setup_sleepin_eint: 
 *
 * Parameter:
 * Input: 
 * Output: 
 * Returns: 
 * 
 * modification history
 * --------------------
 *    2014/04/23, Albert_lee create this function
 * 
 ******************************************************************************/
static void ut4412_setup_sleepin_eint(void)
{
	unsigned int tmp;

       printk("%s(); +\n",__func__);

	if (is_axp_pmu) {
		s3c_gpio_cfgpin(GPIO_PMU_INT, S3C_GPIO_SFN(0xF));
		s3c_gpio_setpull(GPIO_PMU_INT, S3C_GPIO_PULL_UP);
		enable_irq_wake(IRQ_PMU_INT);
	} else {
		s3c_gpio_cfgpin(GPIO_ON_OUT, S3C_GPIO_SFN(0xF));
		s3c_gpio_setpull(GPIO_ON_OUT, S3C_GPIO_PULL_UP);
		enable_irq_wake(IRQ_ON_OUT);
	}

	s3c_gpio_cfgpin(GPIO_KP_VOLUMEUP, S3C_GPIO_SFN(0xF));
	s3c_gpio_cfgpin(GPIO_KP_VOLUMEDOWN, S3C_GPIO_SFN(0xF));
//	s3c_gpio_cfgpin(GPIO_KEY_HOME, S3C_GPIO_SFN(0xF));

	s3c_gpio_setpull(GPIO_KP_VOLUMEUP, S3C_GPIO_PULL_UP);
	s3c_gpio_setpull(GPIO_KP_VOLUMEDOWN, S3C_GPIO_PULL_UP);
//	s3c_gpio_setpull(GPIO_KEY_HOME, S3C_GPIO_PULL_UP);

	enable_irq_wake(IRQ_KP_VOLUMEUP);
	enable_irq_wake(IRQ_KP_VOLUMEDOWN);

	if(!strcmp(g_selected_utmodel,"d720")){
		printk("enable D720 irq14/irq2 wake up\n");
		s3c_gpio_cfgpin(EXYNOS4_GPX1(6),S3C_GPIO_SFN(0xF));
		irq_set_irq_type(IRQ_EINT(14), IRQ_TYPE_EDGE_BOTH);
		enable_irq_wake(IRQ_EINT(14));
		s3c_gpio_cfgpin(EXYNOS4_GPX0(2),S3C_GPIO_SFN(0xF));
		irq_set_irq_type(IRQ_EINT(2), IRQ_TYPE_EDGE_BOTH);
		enable_irq_wake(IRQ_EINT(2));
	}
	
//	enable_irq_wake(IRQ_KEY_HOME);

//	if (g_gsm_power_state) {
//		s3c_gpio_cfgpin(GPIO_3G_WUP, S3C_GPIO_SFN(0xF));
//		s3c_gpio_setpull(GPIO_3G_WUP, S3C_GPIO_PULL_UP);
//		enable_irq_wake(IRQ_3G_WUP);
//	} else {
//		s3c_gpio_cfgpin(GPIO_3G_WUP, S3C_GPIO_INPUT);
////		disable_irq_wake(IRQ_3G_WUP);
//	}

	tmp = readl(S5P_EINT_WAKEUP_MASK);

       printk("%s(); #e-int wakup = %08x\n",__func__, tmp);

	return;
}

/******************************************************************************
 * ut4412_setup_wakeout_eint: 
 *
 * Parameter:
 * Input: 
 * Output: 
 * Returns: 
 * 
 * modification history
 * --------------------
 *    2014/04/23, Albert_lee create this function
 * 
 ******************************************************************************/
static void ut4412_setup_wakeout_eint(void)
{
	if (is_axp_pmu) {
		disable_irq_wake(IRQ_PMU_INT);
	} else {
		disable_irq_wake(IRQ_ON_OUT);
	}

	disable_irq_wake(IRQ_KP_VOLUMEUP);
	disable_irq_wake(IRQ_KP_VOLUMEDOWN);
	disable_irq_wake(IRQ_KEY_HOME);

	return;
}

#endif

int s3c_pm_is_key_wakeup(void)
{
	if(__raw_readl(S5P_WAKEUP_STAT) & 0x01) {
		if((wake_type0 & 0x2)
			|| (wake_type2 & 0x1)
			|| (wake_type2 & 0x2)
			)
		return 1;
	}

	return 0;
}

EXPORT_SYMBOL(s3c_pm_is_key_wakeup);


extern void ut7gm_pm_suspend(void);
extern void ut7gm_pm_wakeup(void);

static int s3c_pm_enter(suspend_state_t state)
{
	/* ensure the debug is initialised (if enabled) */

	s3c_pm_debug_init();

	S3C_PMDBG("%s(%d)\n", __func__, state);

#if 1	//Urbest+ for ut7gm
	ut7gm_pm_suspend();
#endif
	ut4412_setup_sleepin_eint();

	if (pm_cpu_prep == NULL || pm_cpu_sleep == NULL) {
		printk(KERN_ERR "%s: error: no cpu sleep function\n", __func__);
		return -EINVAL;
	}
	/* check if we have anything to wake-up with... bad things seem
	 * to happen if you suspend with no wakeup (system will often
	 * require a full power-cycle)
	*/

	if (!any_allowed(s3c_irqwake_intmask, s3c_irqwake_intallow) &&
	    !any_allowed(s3c_irqwake_eintmask, s3c_irqwake_eintallow)) {
		printk(KERN_ERR "%s: No wake-up sources!\n", __func__);
		printk(KERN_ERR "%s: Aborting sleep\n", __func__);
		return -EINVAL;
	}

	/* save all necessary core registers not covered by the drivers */

	s3c_pm_save_gpios();
	s3c_pm_saved_gpios();
	s3c_pm_save_uarts();
	s3c_pm_save_core();

	/* set the irq configuration for wake */

	s3c_pm_configure_extint();

	S3C_PMDBG("sleep: irq wakeup masks: %08lx,%08lx\n",
	    s3c_irqwake_intmask, s3c_irqwake_eintmask);

	s3c_pm_arch_prepare_irqs();

	/* call cpu specific preparation */

	pm_cpu_prep();

	/* flush cache back to ram */

	flush_cache_all();

	s3c_pm_check_store();

	/* send the cpu to sleep... */

	s3c_pm_arch_stop_clocks();

	printk(KERN_ALERT "PM: SLEEP\n");

	/* s3c_cpu_save will also act as our return point from when
	 * we resume as it saves its own register state and restores it
	 * during the resume.  */

	printk(KERN_ALERT "ARM_COREx_STATUS CORE1[0x%08x], CORE2[0x%08x], CORE3[0x%08x]\n",
			__raw_readl(S5P_VA_PMU + 0x2084),
			__raw_readl(S5P_VA_PMU + 0x2104),
			__raw_readl(S5P_VA_PMU + 0x2184));

	s3c_cpu_save(0, PLAT_PHYS_OFFSET - PAGE_OFFSET);

	/* restore the cpu state using the kernel's cpu init code. */

	cpu_init();

	s3c_pm_restore_core();
	s3c_pm_restore_uarts();
	s3c_pm_restore_gpios();
	s3c_pm_restored_gpios();

	s3c_pm_debug_init();

	/* restore the system state */

	if (pm_cpu_restore)
		pm_cpu_restore();

	/* check what irq (if any) restored the system */

	s3c_pm_arch_show_resume_irqs();

	S3C_PMDBG("%s: post sleep, preparing to return\n", __func__);

	/* LEDs should now be 1110 */
	s3c_pm_debug_smdkled(1 << 1, 0);

	s3c_pm_check_restore();

	/* ok, let's return from sleep */
#if 1	//urbetter+ for ut7gm
	ut4412_setup_wakeout_eint();
	ut7gm_pm_wakeup();
#endif

	S3C_PMDBG("S3C PM Resume (post-restore)\n");
	return 0;
}

static int s3c_pm_prepare(void)
{
	/* prepare check area if configured */

	s3c_pm_check_prepare();

	if (pm_prepare)
		pm_prepare();
	return 0;
}

static void s3c_pm_finish(void)
{
	s3c_pm_check_cleanup();
}

static const struct platform_suspend_ops s3c_pm_ops = {
	.enter		= s3c_pm_enter,
	.prepare	= s3c_pm_prepare,
	.finish		= s3c_pm_finish,
	.valid		= suspend_valid_only_mem,
};

/* s3c_pm_init
 *
 * Attach the power management functions. This should be called
 * from the board specific initialisation if the board supports
 * it.
*/

int __init s3c_pm_init(void)
{
	printk("S3C Power Management, Copyright 2004 Simtec Electronics\n");

	suspend_set_ops(&s3c_pm_ops);
	return 0;
}
