#ifndef KFSW_PLATFORM_WALLCLOCK_H
#define KFSW_PLATFORM_WALLCLOCK_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Wall time that outlives a reset, when the board can keep it.
 *
 * Distinct from the monotonic clock in time.h, which measures elapsed time and
 * restarts at every boot. This is the time a sample is stamped with, and the
 * reason it is worth reading from hardware rather than from RAM: a node that
 * resets mid-pass otherwise comes back not knowing when it is, and everything
 * gated on a valid clock — scheduled collection, beacons — stays silent until
 * a ground station is there to tell it.
 *
 * Backed by the RTC named by the `kfsw,rtc` chosen node. A composition without
 * one answers -ENOTSUP and the caller keeps whatever it used before.
 *
 * How much of a reset it survives is the board's business, not this API's: the
 * counter lives in a backup domain that a software reset leaves alone, and
 * that a power cycle only preserves where VBAT is actually backed.
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
