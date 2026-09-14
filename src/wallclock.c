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

/* Wall clock on the RTC. The counter survives a software reset. */
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
		/* -ENODATA means the RTC runs but was never set. */
		return result;
	}

	/* struct rtc_time is layout-compatible with struct tm. */
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
