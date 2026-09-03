#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <zephyr/drivers/hwinfo.h>

#include <kfsw/platform/reset.h>

int kfsw_platform_get_reset_cause(uint32_t *cause)
{
    int rc;

    if (cause == NULL) {
        return -EINVAL;
    }

    *cause = 0U;

    rc = hwinfo_get_reset_cause(cause);

    if (rc == 0) {
        /* Clear latched flags so the next boot reports a new reset event. */
        (void)hwinfo_clear_reset_cause();
    }

    return rc;
}

bool kfsw_platform_reset_cause_is_watchdog(uint32_t cause)
{
    return (cause & RESET_WATCHDOG) != 0U;
}

const char *kfsw_platform_reset_cause_name(uint32_t cause)
{
    /* Ordered by what an operator needs to know first after an unattended
     * restart, not by bit position. A watchdog reset alongside a brownout is
     * still a watchdog reset as far as the next boot report is concerned.
     */
    if ((cause & RESET_WATCHDOG) != 0U) {
        return "watchdog";
    }
    if ((cause & RESET_BROWNOUT) != 0U) {
        return "brownout";
    }
    if ((cause & RESET_POR) != 0U) {
        return "power-on";
    }
    if ((cause & RESET_SOFTWARE) != 0U) {
        return "software";
    }
    if ((cause & RESET_PIN) != 0U) {
        return "pin";
    }
    if ((cause & RESET_DEBUG) != 0U) {
        return "debug";
    }
    if ((cause & RESET_LOW_POWER_WAKE) != 0U) {
        return "low-power-wake";
    }

    return "unknown";
}
