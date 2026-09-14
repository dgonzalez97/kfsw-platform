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
 * Arm, feed and stop feeding a hardware watchdog. The health service can take
 * over the feeding. Most parts, including the STM32 independent watchdog, can't
 * be disarmed once started.
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
	/** Hardware armed and no longer fed; a reset is expected. */
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
	/** True while the built-in keep-alive is feeding. */
	bool keepalive_owned;
};

/**
 * @brief Feed interval for a timeout: a third of it, at least 1 ms.
 *
 * @param timeout_ms Configured timeout in milliseconds.
 *
 * @return The feed interval in milliseconds.
 */
uint32_t kfsw_platform_watchdog_feed_interval_ms(uint32_t timeout_ms);

/**
 * @brief Watchdog timeout selected for this board, available before init.
 * @return Configured milliseconds, or zero when no device is selected.
 */
uint32_t kfsw_platform_watchdog_configured_timeout_ms(void);

/**
 * @brief Bind the watchdog device and install the configured timeout.
 *
 * Does not arm the hardware; call @ref kfsw_platform_watchdog_start for that.
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
 * A starved watchdog can't be fed again.
 *
 * @retval 0 The watchdog was fed.
 * @retval -ENODEV No watchdog device is bound.
 * @retval -EINVAL The watchdog is not running.
 * @retval -EPERM Feeding was stopped.
 * @return A negative errno value from the driver on failure.
 */
int kfsw_platform_watchdog_feed(void);

/**
 * @brief Stop the built-in keep-alive and let the caller feed the watchdog.
 *
 * The watchdog stays armed. The caller must feed it at least every
 * @ref kfsw_platform_watchdog_feed_interval_ms or the part resets.
 *
 * @retval 0 The keep-alive stopped.
 * @retval -EWOULDBLOCK Called outside a preemptible thread or from the system
 *                     workqueue.
 * @retval -ENODEV No watchdog device is bound.
 * @retval -EINVAL The watchdog is not running.
 */
int kfsw_platform_watchdog_release(void);

/**
 * @brief Stop feeding the watchdog so that it expires.
 *
 * The part resets within one timeout, and this can't be undone.
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
