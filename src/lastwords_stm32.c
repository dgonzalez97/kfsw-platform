#include <errno.h>

#include <zephyr/irq.h>
#include <zephyr/kernel.h>

#include <stm32l4xx_ll_exti.h>
#include <stm32l4xx_ll_pwr.h>

#include <kfsw/platform/lastwords.h>
#include <kfsw/platform/time.h>

/* The supply watcher, where the SoC has one.
 *
 * The programmable voltage detector fires while the part is still running, at a
 * threshold above the level that resets it. That gap is the only chance to
 * write anything: once the brown-out reset asserts there is no software left to
 * run. Whether the note is still readable afterwards depends on how far the
 * rail actually fell, which is the honest limit of this approach.
 */

#define KFSW_PVD_EXTI_LINE LL_EXTI_LINE_16

static void pvd_isr(const void *argument)
{
	ARG_UNUSED(argument);

	if (!LL_EXTI_IsActiveFlag_0_31(KFSW_PVD_EXTI_LINE)) {
		return;
	}
	LL_EXTI_ClearFlag_0_31(KFSW_PVD_EXTI_LINE);

	/* Nothing else here. This runs while the supply is falling, so the only
	 * safe thing to do is leave the note and let whatever happens happen.
	 * No logging, no reset, no work item: each of those can outlive the
	 * voltage that would let it finish.
	 */
	kfsw_lastwords_write(KFSW_LASTWORDS_BROWNOUT, 0U, (uint32_t)kfsw_time_monotonic_ms(), 0U);
}

int kfsw_lastwords_watch_supply(void)
{
	/* Rising edge only. The interesting event is the supply crossing down
	 * through the threshold; the recovery edge would overwrite the note
	 * with a second, less useful one.
	 */
	LL_EXTI_EnableIT_0_31(KFSW_PVD_EXTI_LINE);
	LL_EXTI_EnableRisingTrig_0_31(KFSW_PVD_EXTI_LINE);
	LL_EXTI_DisableFallingTrig_0_31(KFSW_PVD_EXTI_LINE);

	LL_PWR_SetPVDLevel(CONFIG_KFSW_LASTWORDS_PVD_LEVEL);
	LL_PWR_EnablePVD();

	IRQ_CONNECT(PVD_PVM_IRQn, 0, pvd_isr, NULL, 0);
	irq_enable(PVD_PVM_IRQn);
	return 0;
}
