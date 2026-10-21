#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include "semihost_internal.h"

#if CONFIG_CPU_CORTEX_M && CONFIG_ARMV7_M_ARMV8_M_MAINLINE && !CONFIG_ARMV8_M_SE
#include <cmsis_core.h>
#include <zephyr/arch/common/semihost.h>

#define SEMIHOST_STDOUT 1L

int kfsw_semihost_context(void)
{
	if (k_is_in_isr() || (__get_IPSR() != 0U) || (__get_PRIMASK() != 0U) ||
	    ((__get_CONTROL() & CONTROL_nPRIV_Msk) != 0U)) {
		return -EWOULDBLOCK;
	}
	if ((__get_BASEPRI() != 0U) || (__get_FAULTMASK() != 0U)) {
		return -EWOULDBLOCK;
	}
	return (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) != 0U ? 0 : -ENODEV;
}

long kfsw_semihost_target_write(const char *text, size_t size)
{
	int result = kfsw_semihost_context();

	if (result != 0) {
		return result;
	}
	/* Zephyr passes SYS_WRITE's count of unwritten bytes through unchanged. */
	return semihost_write(SEMIHOST_STDOUT, text, (long)size);
}
#else
int kfsw_semihost_context(void)
{
	return -ENOTSUP;
}

long kfsw_semihost_target_write(const char *text, size_t size)
{
	ARG_UNUSED(text);
	ARG_UNUSED(size);
	return -ENOTSUP;
}
#endif
