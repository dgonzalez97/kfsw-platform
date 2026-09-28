#ifndef KFSW_PLATFORM_TIME_H
#define KFSW_PLATFORM_TIME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get monotonic elapsed time in milliseconds.
 *
 * Use for scheduling and timeouts. This clock is not synchronized to UTC.
 *
 * @return Milliseconds elapsed since the platform monotonic clock started.
 */
uint64_t kfsw_time_monotonic_ms(void);

/**
 * @brief Get monotonic elapsed time in microseconds.
 *
 * Use for intervals and measurements. This clock is not synchronized to UTC.
 *
 * @return Microseconds elapsed since the platform monotonic clock started.
 */
uint64_t kfsw_time_monotonic_us(void);

#ifdef __cplusplus
}
#endif

#endif
