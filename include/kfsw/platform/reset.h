#ifndef KFSW_PLATFORM_RESET_H
#define KFSW_PLATFORM_RESET_H

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

#ifdef __cplusplus
}
#endif

#endif
