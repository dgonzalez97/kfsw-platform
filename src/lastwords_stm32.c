#include <errno.h>

#include <zephyr/irq.h>
#include <zephyr/kernel.h>

#include <stm32l4xx_ll_exti.h>
#include <stm32l4xx_ll_pwr.h>

#include <kfsw/platform/lastwords.h>
#include <kfsw/platform/time.h>

/* Programmable voltage detector. It fires before the brown-out reset. */

#define KFSW_PVD_EXTI_LINE LL_EXTI_LINE_16

static void pvd_isr(const void *argument)
{
	ARG_UNUSED(argument);

	if (!LL_EXTI_IsActiveFlag_0_31(KFSW_PVD_EXTI_LINE)) {
		return;
	}
	LL_EXTI_ClearFlag_0_31(KFSW_PVD_EXTI_LINE);

	/* The supply is falling: only write the note. */
	kfsw_lastwords_write(KFSW_LASTWORDS_BROWNOUT, 0U, (uint32_t)kfsw_time_monotonic_ms(), 0U);
}

int kfsw_lastwords_watch_supply(void)
{
	/* Rising edge: the supply went below the threshold. */
	LL_EXTI_EnableIT_0_31(KFSW_PVD_EXTI_LINE);
	LL_EXTI_EnableRisingTrig_0_31(KFSW_PVD_EXTI_LINE);
	LL_EXTI_DisableFallingTrig_0_31(KFSW_PVD_EXTI_LINE);

	LL_PWR_SetPVDLevel(CONFIG_KFSW_LASTWORDS_PVD_LEVEL);
	LL_PWR_EnablePVD();

	IRQ_CONNECT(PVD_PVM_IRQn, 0, pvd_isr, NULL, 0);
	irq_enable(PVD_PVM_IRQn);
	return 0;
}
