#ifndef KFSW_PLATFORM_RESET_H
#define KFSW_PLATFORM_RESET_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Read and clear the hardware reset cause.
 *
 * @param[out] cause Reset-cause flags.
 *
 * @retval 0 The reset cause was read successfully.
 * @retval -EINVAL @p cause is NULL.
 * @return A negative errno value on failure.
 */
int kfsw_platform_get_reset_cause(uint32_t *cause);

/**
 * @brief Report whether a reset cause names the watchdog.
 *
 * The raw cause is a bitmask and a reset can latch several bits at once, so
 * this asks only whether the watchdog is among them. It is pure and available
 * on every target, including those without watchdog hardware.
 *
 * @param cause Reset-cause flags from @ref kfsw_platform_get_reset_cause.
 *
 * @return True when the watchdog is one of the reported causes.
 */
bool kfsw_platform_reset_cause_is_watchdog(uint32_t cause);

/**
 * @brief Short human-readable name for a reset cause.
 *
 * Returns the most significant cause when several are latched, preferring the
 * watchdog because that is the one an operator is looking for after an
 * unattended restart. Never returns NULL.
 *
 * @param cause Reset-cause flags from @ref kfsw_platform_get_reset_cause.
 *
 * @return A stable lowercase name, or "unknown" when no known bit is set.
 */
const char *kfsw_platform_reset_cause_name(uint32_t cause);

#ifdef __cplusplus
}
#endif

#endif
