#ifndef KFSW_PLATFORM_WATCHDOG_H
#define KFSW_PLATFORM_WATCHDOG_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup kfsw_platform_watchdog K-FSW platform watchdog
 * @ingroup kfsw_platform
 *
 * Hardware watchdog mechanism. The platform owns arming the device, feeding it
 * and stopping the feed on request. It owns no health policy: nothing here
 * decides whether the system is well, only whether the watchdog has been told
 * that it is.
 *
 * The keep-alive is a default, not a supervisor. A later health component is
 * expected to take ownership of the decision to feed, at which point it should
 * stop the built-in keep-alive and call @ref kfsw_platform_watchdog_feed
 * itself.
 *
 * The watchdog cannot be disarmed on most parts once started, including the
 * STM32 independent watchdog this was first brought up against. Stopping the
 * feed therefore ends in a reset, by design.
 *
 * @{
 */

/** Watchdog lifecycle state. */
enum kfsw_platform_watchdog_state {
	/** No device bound, or initialization has not run. */
	KFSW_PLATFORM_WATCHDOG_UNCONFIGURED = 0,
	/** Timeout installed; hardware not yet armed. */
	KFSW_PLATFORM_WATCHDOG_CONFIGURED = 1,
	/** Hardware armed and the keep-alive is feeding it. */
	KFSW_PLATFORM_WATCHDOG_RUNNING = 2,
	/** Hardware armed and deliberately starved; a reset is expected. */
	KFSW_PLATFORM_WATCHDOG_STARVED = 3,
};

/** Consistent snapshot of watchdog configuration and activity. */
struct kfsw_platform_watchdog_info {
	/** Configured timeout in milliseconds. */
	uint32_t timeout_ms;
	/** Interval the keep-alive feeds at, in milliseconds. */
	uint32_t feed_interval_ms;
	/** Feeds issued since boot; saturates at UINT32_MAX. */
	uint32_t feeds;
	/** Milliseconds since the most recent feed, or zero if never fed. */
	uint32_t since_feed_ms;
	/** One of @ref kfsw_platform_watchdog_state. */
	uint8_t state;
	/** True when a watchdog device was bound at initialization. */
	bool device_bound;
	/** True while the built-in keep-alive is the one feeding. */
	bool keepalive_owned;
};

/**
 * @brief Feed interval for a given timeout.
 *
 * Returns the interval the keep-alive uses: a third of the timeout, so two
 * consecutive feeds can be missed before the hardware expires. The result is
 * clamped to at least one millisecond.
 *
 * This is pure arithmetic and is available on every target, including those
 * with no watchdog hardware, so the margin can be asserted in a unit test.
 *
 * @param timeout_ms Configured timeout in milliseconds.
 *
 * @return The feed interval in milliseconds.
 */
uint32_t kfsw_platform_watchdog_feed_interval_ms(uint32_t timeout_ms);

/**
 * @brief Bind the watchdog device and install the configured timeout.
 *
 * Does not arm the hardware; call @ref kfsw_platform_watchdog_start for that.
 * Splitting the two lets a composition install the timeout early and arm only
 * once the rest of the system is up, so a slow boot cannot be reset by its own
 * watchdog.
 *
 * @retval 0 The timeout was installed.
 * @retval -ENODEV No watchdog device is bound to the kfsw,watchdog chosen
 *                 property, or the device is not ready.
 * @retval -EALREADY Initialization has already run.
 * @return A negative errno value from the driver on failure.
 */
int kfsw_platform_watchdog_init(void);

/**
 * @brief Arm the hardware and start the keep-alive.
 *
 * @retval 0 The watchdog is running.
 * @retval -ENODEV No watchdog device is bound.
 * @retval -EINVAL Initialization has not run.
 * @return A negative errno value from the driver on failure.
 */
int kfsw_platform_watchdog_start(void);

/**
 * @brief Feed the watchdog once.
 *
 * Safe to call from any context that may block briefly. Feeding a starved
 * watchdog is rejected rather than silently rescuing it, so a deliberate reset
 * cannot be cancelled by an unrelated caller.
 *
 * @retval 0 The watchdog was fed.
 * @retval -ENODEV No watchdog device is bound.
 * @retval -EINVAL The watchdog is not running.
 * @retval -EPERM The feed was deliberately stopped.
 * @return A negative errno value from the driver on failure.
 */
int kfsw_platform_watchdog_feed(void);

/**
 * @brief Hand the feeding over to a caller.
 *
 * Stops the built-in keep-alive without starving the watchdog: it stays armed
 * and stays feedable, and from then on it is fed only when the caller says so.
 *
 * This is how health monitoring takes ownership. Stopping the feed outright
 * would be a decision to reset, which is a different thing from deciding who
 * makes that decision.
 *
 * The caller must feed at least as often as
 * @ref kfsw_platform_watchdog_feed_interval_ms, or the part resets.
 *
 * @retval 0 The keep-alive stopped and feeding is now the caller's.
 * @retval -ENODEV No watchdog device is bound.
 * @retval -EINVAL The watchdog is not running.
 */
int kfsw_platform_watchdog_release(void);

/**
 * @brief Stop feeding the watchdog so that it expires.
 *
 * Intended for the hardware acceptance test and for a future health component
 * that has decided the system is unrecoverable. The reset follows within one
 * timeout period. There is no way back: on parts whose watchdog cannot be
 * disarmed, the reset is now inevitable.
 *
 * @retval 0 The keep-alive was stopped.
 * @retval -ENODEV No watchdog device is bound.
 * @retval -EINVAL The watchdog is not running.
 */
int kfsw_platform_watchdog_stop_feeding(void);

/**
 * @brief Read a consistent snapshot of watchdog state.
 *
 * @param[out] info Destination snapshot.
 *
 * @retval 0 The snapshot was written.
 * @retval -EINVAL @p info is NULL.
 */
int kfsw_platform_watchdog_get_info(struct kfsw_platform_watchdog_info *info);

/** @} */

#ifdef __cplusplus
}
#endif

#endif
