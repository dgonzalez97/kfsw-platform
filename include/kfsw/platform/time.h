#ifndef KFSW_PLATFORM_TIME_H
#define KFSW_PLATFORM_TIME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get monotonic elapsed time in milliseconds.
 *
 * This local elapsed time is suitable for scheduling and timeouts. It is not
 * synchronized spacecraft time, UTC, TAI, or GNSS time.
 *
 * @return Milliseconds elapsed since the platform monotonic clock started.
 */
uint64_t kfsw_time_monotonic_ms(void);

/**
 * @brief Get monotonic elapsed time in microseconds.
 *
 * This local elapsed time is suitable for scheduling and measurements. It is
 * not synchronized spacecraft time, UTC, TAI, or GNSS time.
 *
 * @return Microseconds elapsed since the platform monotonic clock started.
 */
uint64_t kfsw_time_monotonic_us(void);

#ifdef __cplusplus
}
#endif

#endif
