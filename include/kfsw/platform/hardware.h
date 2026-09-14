#ifndef KFSW_PLATFORM_HARDWARE_H
#define KFSW_PLATFORM_HARDWARE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Longest identifier read, in bytes. The Kinetis family reports 128 bits. */
#define KFSW_HARDWARE_ID_MAX_BYTES 16U

/** Buffer size that always holds a formatted identifier and its terminator. */
#define KFSW_HARDWARE_ID_TEXT_SIZE ((KFSW_HARDWARE_ID_MAX_BYTES * 2U) + 1U)

/**
 * @brief Read the chip's unique ID as lowercase hex, most significant byte first.
 *
 * @param[out] text Buffer receiving the NUL-terminated identifier.
 * @param size Bytes available in @p text; @ref KFSW_HARDWARE_ID_TEXT_SIZE
 *             is always enough.
 *
 * @retval 0 The identifier was read and formatted.
 * @retval -EINVAL @p text is NULL.
 * @retval -ENOSPC @p size is too small for this SoC's identifier. Nothing is
 *                 written.
 * @retval -ENOTSUP This SoC does not report one.
 */
int kfsw_platform_get_hardware_id(char *text, size_t size);

#ifdef __cplusplus
}
#endif

#endif
