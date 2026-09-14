#ifndef KFSW_PLATFORM_WALLCLOCK_H
#define KFSW_PLATFORM_WALLCLOCK_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Wall time from the RTC named by the `kfsw,rtc` chosen node.
 *
 * Unlike the monotonic clock in time.h, it survives a software reset, and a
 * power cycle when VBAT is backed. Without an RTC the calls return -ENOTSUP.
 */

/** Whether this composition has a wall clock at all. */
bool kfsw_wallclock_is_present(void);

/**
 * @brief Read the wall clock.
 *
 * @param[out] seconds Unix seconds.
 * @retval 0 Read, and the value had been set.
 * @retval -ENOTSUP No wall clock in this composition.
 * @retval -ENODATA The clock is running but was never set.
 * @retval <0 An errno from the driver.
 */
int kfsw_wallclock_get(int64_t *seconds);

/**
 * @brief Set the wall clock.
 *
 * @param seconds Unix seconds. Must be positive.
 * @retval 0 Set.
 * @retval -EINVAL A time before the epoch.
 * @retval -ENOTSUP No wall clock in this composition.
 * @retval <0 An errno from the driver.
 */
int kfsw_wallclock_set(int64_t seconds);

#ifdef __cplusplus
}
#endif

#endif /* KFSW_PLATFORM_WALLCLOCK_H */
