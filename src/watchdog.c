#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <kfsw/platform/time.h>
#include <kfsw/platform/watchdog.h>

/* The board's watchdog0 alias is the window watchdog, so the device comes
 * from the kfsw,watchdog chosen property instead.
 */
#define KFSW_WATCHDOG_NODE DT_CHOSEN(kfsw_watchdog)
#define KFSW_WATCHDOG_PRESENT DT_NODE_HAS_STATUS(KFSW_WATCHDOG_NODE, okay)

/* Feeding at a third of the timeout leaves room for two consecutive misses. */
#define KFSW_WATCHDOG_FEED_DIVISOR 3U

static struct k_spinlock watchdog_lock;
static struct kfsw_platform_watchdog_info watchdog_state = {
	.state = KFSW_PLATFORM_WATCHDOG_UNCONFIGURED,
};
static uint64_t watchdog_last_feed_ms;

#if KFSW_WATCHDOG_PRESENT
static const struct device *const watchdog_device = DEVICE_DT_GET(KFSW_WATCHDOG_NODE);
static int watchdog_channel = -1;
K_MUTEX_DEFINE(watchdog_handover_lock);
static struct k_work_sync watchdog_release_sync;

static void watchdog_keepalive_handler(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(watchdog_keepalive, watchdog_keepalive_handler);
#endif

uint32_t kfsw_platform_watchdog_feed_interval_ms(uint32_t timeout_ms)
{
	uint32_t interval = timeout_ms / KFSW_WATCHDOG_FEED_DIVISOR;

	/* A timeout below the divisor would give a zero feed interval. */
	return (interval == 0U) ? 1U : interval;
}

uint32_t kfsw_platform_watchdog_configured_timeout_ms(void)
{
#if KFSW_WATCHDOG_PRESENT
	return CONFIG_KFSW_WATCHDOG_TIMEOUT_MS;
#else
	return 0U;
#endif
}

#if KFSW_WATCHDOG_PRESENT
/* Caller holds watchdog_lock. */
static int watchdog_feed_locked(void)
{
	int result;

	result = wdt_feed(watchdog_device, watchdog_channel);
	if (result != 0) {
		return result;
	}

	if (watchdog_state.feeds < UINT32_MAX) {
		watchdog_state.feeds++;
	}
	watchdog_last_feed_ms = kfsw_time_monotonic_ms();
	return 0;
}

static void watchdog_keepalive_handler(struct k_work *work)
{
	k_spinlock_key_t key;

	ARG_UNUSED(work);

	key = k_spin_lock(&watchdog_lock);
	if (watchdog_state.keepalive_owned &&
	    (watchdog_state.state == KFSW_PLATFORM_WATCHDOG_RUNNING)) {
		(void)watchdog_feed_locked();
		(void)k_work_reschedule(&watchdog_keepalive,
					K_MSEC(watchdog_state.feed_interval_ms));
	}
	k_spin_unlock(&watchdog_lock, key);
}
#endif /* KFSW_WATCHDOG_PRESENT */

int kfsw_platform_watchdog_init(void)
{
#if KFSW_WATCHDOG_PRESENT
	const struct wdt_timeout_cfg timeout = {
		.window =
			{
				.min = 0U,
				.max = CONFIG_KFSW_WATCHDOG_TIMEOUT_MS,
			},
		/* No callback, so the reset doesn't depend on an interrupt. */
		.callback = NULL,
		.flags = WDT_FLAG_RESET_SOC,
	};
	k_spinlock_key_t key;
	int result;

	if (!device_is_ready(watchdog_device)) {
		return -ENODEV;
	}

	key = k_spin_lock(&watchdog_lock);
	if (watchdog_state.state != KFSW_PLATFORM_WATCHDOG_UNCONFIGURED) {
		k_spin_unlock(&watchdog_lock, key);
		return -EALREADY;
	}
	k_spin_unlock(&watchdog_lock, key);

	result = wdt_install_timeout(watchdog_device, &timeout);
	if (result < 0) {
		return result;
	}

	key = k_spin_lock(&watchdog_lock);
	watchdog_channel = result;
	watchdog_state.timeout_ms = CONFIG_KFSW_WATCHDOG_TIMEOUT_MS;
	watchdog_state.feed_interval_ms =
		kfsw_platform_watchdog_feed_interval_ms(CONFIG_KFSW_WATCHDOG_TIMEOUT_MS);
	watchdog_state.device_bound = true;
	watchdog_state.state = KFSW_PLATFORM_WATCHDOG_CONFIGURED;
	k_spin_unlock(&watchdog_lock, key);

	return 0;
#else
	return -ENODEV;
#endif
}

int kfsw_platform_watchdog_start(void)
{
#if KFSW_WATCHDOG_PRESENT
	k_spinlock_key_t key;
	uint32_t interval;
	int result;

	key = k_spin_lock(&watchdog_lock);
	if (watchdog_state.state != KFSW_PLATFORM_WATCHDOG_CONFIGURED) {
		k_spin_unlock(&watchdog_lock, key);
		return -EINVAL;
	}
	interval = watchdog_state.feed_interval_ms;
	k_spin_unlock(&watchdog_lock, key);

	/* Pause while halted in the debugger, where the driver supports it. */
	result = wdt_setup(watchdog_device, WDT_OPT_PAUSE_HALTED_BY_DBG);
	if (result == -ENOTSUP) {
		result = wdt_setup(watchdog_device, 0);
	}
	if (result != 0) {
		return result;
	}

	key = k_spin_lock(&watchdog_lock);
	watchdog_state.state = KFSW_PLATFORM_WATCHDOG_RUNNING;
	watchdog_state.keepalive_owned = true;
	(void)watchdog_feed_locked();
	(void)k_work_reschedule(&watchdog_keepalive, K_MSEC(interval));
	k_spin_unlock(&watchdog_lock, key);

	return 0;
#else
	return -ENODEV;
#endif
}

int kfsw_platform_watchdog_feed(void)
{
#if KFSW_WATCHDOG_PRESENT
	k_spinlock_key_t key;
	int result;

	key = k_spin_lock(&watchdog_lock);
	if (watchdog_state.state == KFSW_PLATFORM_WATCHDOG_STARVED) {
		k_spin_unlock(&watchdog_lock, key);
		return -EPERM;
	}
	if (watchdog_state.state != KFSW_PLATFORM_WATCHDOG_RUNNING) {
		k_spin_unlock(&watchdog_lock, key);
		return -EINVAL;
	}
	result = watchdog_feed_locked();
	k_spin_unlock(&watchdog_lock, key);

	return result;
#else
	return -ENODEV;
#endif
}

int kfsw_platform_watchdog_release(void)
{
#if KFSW_WATCHDOG_PRESENT
	k_spinlock_key_t key;

	if (!k_is_preempt_thread() || (k_current_get() == k_work_queue_thread_get(&k_sys_work_q))) {
		return -EWOULDBLOCK;
	}
	k_mutex_lock(&watchdog_handover_lock, K_FOREVER);
	key = k_spin_lock(&watchdog_lock);
	if (watchdog_state.state != KFSW_PLATFORM_WATCHDOG_RUNNING) {
		k_spin_unlock(&watchdog_lock, key);
		k_mutex_unlock(&watchdog_handover_lock);
		return -EINVAL;
	}
	watchdog_state.keepalive_owned = false;
	k_spin_unlock(&watchdog_lock, key);

	/* Wait for a handler that was already running before the handover. */
	(void)k_work_cancel_delayable_sync(&watchdog_keepalive, &watchdog_release_sync);
	k_mutex_unlock(&watchdog_handover_lock);
	return 0;
#else
	return -ENODEV;
#endif
}

int kfsw_platform_watchdog_stop_feeding(void)
{
#if KFSW_WATCHDOG_PRESENT
	k_spinlock_key_t key;

	key = k_spin_lock(&watchdog_lock);
	if (watchdog_state.state != KFSW_PLATFORM_WATCHDOG_RUNNING) {
		k_spin_unlock(&watchdog_lock, key);
		return -EINVAL;
	}
	watchdog_state.state = KFSW_PLATFORM_WATCHDOG_STARVED;
	k_spin_unlock(&watchdog_lock, key);

	/* Don't wait here; the handler checks the state before feeding. */
	(void)k_work_cancel_delayable(&watchdog_keepalive);
	return 0;
#else
	return -ENODEV;
#endif
}

int kfsw_platform_watchdog_get_info(struct kfsw_platform_watchdog_info *info)
{
	k_spinlock_key_t key;
	uint64_t now;

	if (info == NULL) {
		return -EINVAL;
	}

	now = kfsw_time_monotonic_ms();

	key = k_spin_lock(&watchdog_lock);
	*info = watchdog_state;
	if (watchdog_state.feeds == 0U) {
		info->since_feed_ms = 0U;
	} else {
		info->since_feed_ms = (uint32_t)(now - watchdog_last_feed_ms);
	}
	k_spin_unlock(&watchdog_lock, key);

	return 0;
}
