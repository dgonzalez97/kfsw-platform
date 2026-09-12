#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/timeutil.h>

#include <kfsw/platform/wallclock.h>

#if DT_HAS_CHOSEN(kfsw_rtc) && defined(CONFIG_RTC)

#include <zephyr/drivers/rtc.h>

/*
 * A wall clock the board keeps for itself.
 *
 * The counter lives in the RTC's backup domain, which a software reset does
 * not touch. That is the whole point: a node that reboots on a watchdog or on
 * command comes back knowing when it is, so scheduled collection resumes and a
 * beacon starts again without a ground station in view to set the time.
 *
 * A power cycle is a different question and belongs to the board rather than
 * to this file: the domain only survives one where VBAT is actually backed,
 * which on a development board usually means fitting the cell the footprint is
 * there for.
 */
static const struct device *const rtc_device = DEVICE_DT_GET(DT_CHOSEN(kfsw_rtc));

bool kfsw_wallclock_is_present(void)
{
	return device_is_ready(rtc_device);
}

int kfsw_wallclock_get(int64_t *seconds)
{
	struct rtc_time value;
	int64_t unix_seconds;
	int result;

	if (seconds == NULL) {
		return -EINVAL;
	}
	if (!device_is_ready(rtc_device)) {
		return -ENOTSUP;
	}

	result = rtc_get_time(rtc_device, &value);
	if (result != 0) {
		/* The driver reports a clock that is running but was never set
		 * as -ENODATA, which is not a fault: it is a node that has
		 * never been told the time. Passed through unchanged so the
		 * caller can tell the two apart.
		 */
		return result;
	}

	/* struct rtc_time is layout-compatible with struct tm, which is what
	 * the conversion takes; the trailing nanoseconds field is ignored.
	 */
	unix_seconds = timeutil_timegm64((const struct tm *)&value);
	if (unix_seconds < 0) {
		return -ERANGE;
	}
	*seconds = unix_seconds;
	return 0;
}

int kfsw_wallclock_set(int64_t seconds)
{
	struct rtc_time value = {0};
	struct tm broken_down;
	time_t as_time;

	if (seconds < 0) {
		return -EINVAL;
	}
	if (!device_is_ready(rtc_device)) {
		return -ENOTSUP;
	}

	as_time = (time_t)seconds;
	if (gmtime_r(&as_time, &broken_down) == NULL) {
		return -EINVAL;
	}

	value.tm_sec = broken_down.tm_sec;
	value.tm_min = broken_down.tm_min;
	value.tm_hour = broken_down.tm_hour;
	value.tm_mday = broken_down.tm_mday;
	value.tm_mon = broken_down.tm_mon;
	value.tm_year = broken_down.tm_year;
	value.tm_wday = broken_down.tm_wday;
	value.tm_yday = broken_down.tm_yday;
	value.tm_isdst = -1;
	value.tm_nsec = 0;

	return rtc_set_time(rtc_device, &value);
}

#else /* no wall clock in this composition */

bool kfsw_wallclock_is_present(void)
{
	return false;
}

int kfsw_wallclock_get(int64_t *seconds)
{
	ARG_UNUSED(seconds);
	return -ENOTSUP;
}

int kfsw_wallclock_set(int64_t seconds)
{
	ARG_UNUSED(seconds);
	return -ENOTSUP;
}

#endif
