#include <stdint.h>

#include <zephyr/kernel.h>

#include <kfsw/platform/time.h>

uint64_t kfsw_time_monotonic_ms(void)
{
    return (uint64_t)k_uptime_get();
}

uint64_t kfsw_time_monotonic_us(void)
{
    if (IS_ENABLED(CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER)) {
        return k_cyc_to_us_floor64(k_cycle_get_64());
    }

    return k_ticks_to_us_floor64(k_uptime_ticks());
}
