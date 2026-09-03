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

/* The board's own watchdog0 alias is not used. On the first target brought up
 * it selects the window watchdog, which resets when fed too early and whose
 * timeout is far shorter than a flight keep-alive wants. The device is chosen
 * explicitly instead, the same way the rest of K-FSW binds hardware.
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

static void watchdog_keepalive_handler(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(watchdog_keepalive, watchdog_keepalive_handler);
#endif

uint32_t kfsw_platform_watchdog_feed_interval_ms(uint32_t timeout_ms)
{
	uint32_t interval = timeout_ms / KFSW_WATCHDOG_FEED_DIVISOR;

	/* A zero interval would busy-feed and starve everything else, and a
	 * timeout below the divisor is a misconfiguration rather than a
	 * request for an infinitely fast keep-alive.
	 */
	return (interval == 0U) ? 1U : interval;
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
	bool running;
	uint32_t interval;

	ARG_UNUSED(work);

	key = k_spin_lock(&watchdog_lock);
	running = (watchdog_state.state == KFSW_PLATFORM_WATCHDOG_RUNNING);
	interval = watchdog_state.feed_interval_ms;
	if (running) {
		(void)watchdog_feed_locked();
	}
	k_spin_unlock(&watchdog_lock, key);

	/* A starved watchdog must not be rescheduled: that is the whole point
	 * of stopping the feed.
	 */
	if (running) {
		(void)k_work_reschedule(&watchdog_keepalive, K_MSEC(interval));
	}
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
		/* No callback: the independent watchdog on the first target
		 * supports one only behind an extra Kconfig, and a reset that
		 * depends on an interrupt firing is weaker than one that does
		 * not.
		 */
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

	/* Pausing while halted by the debugger keeps a breakpoint from looking
	 * like a hang. Not every driver supports it, and losing the option is
	 * not a reason to leave the system unguarded.
	 */
	result = wdt_setup(watchdog_device, WDT_OPT_PAUSE_HALTED_BY_DBG);
	if (result == -ENOTSUP) {
		result = wdt_setup(watchdog_device, 0);
	}
	if (result != 0) {
		return result;
	}

	key = k_spin_lock(&watchdog_lock);
	watchdog_state.state = KFSW_PLATFORM_WATCHDOG_RUNNING;
	(void)watchdog_feed_locked();
	k_spin_unlock(&watchdog_lock, key);

	(void)k_work_reschedule(&watchdog_keepalive, K_MSEC(interval));
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

	/* Cancel without waiting: this may be called from a context that must
	 * not block, and the handler checks the state before feeding anyway.
	 */
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
