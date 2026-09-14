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
 * @brief Whether the watchdog is one of the latched reset causes.
 *
 * @param cause Reset-cause flags from @ref kfsw_platform_get_reset_cause.
 *
 * @return True when the watchdog bit is set.
 */
bool kfsw_platform_reset_cause_is_watchdog(uint32_t cause);

/**
 * @brief Short name for a reset cause.
 *
 * When several causes are latched, the watchdog is reported first. Never
 * returns NULL.
 *
 * @param cause Reset-cause flags from @ref kfsw_platform_get_reset_cause.
 *
 * @return A lowercase name, or "unknown" when no known bit is set.
 */
const char *kfsw_platform_reset_cause_name(uint32_t cause);

#ifdef __cplusplus
}
#endif

#endif
