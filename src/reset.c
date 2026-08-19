#include <errno.h>
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
