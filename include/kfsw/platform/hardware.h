#ifndef KFSW_PLATFORM_HARDWARE_H
#define KFSW_PLATFORM_HARDWARE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Longest identifier this reads, in bytes.
 *
 * Sized for the widest in tree: the Kinetis family reports 128 bits. An SoC
 * that reported more would be truncated, and a truncated identity names the
 * wrong unit, so this is checked rather than assumed.
 */
#define KFSW_HARDWARE_ID_MAX_BYTES 16U

/** Buffer size that always holds a formatted identifier and its terminator. */
#define KFSW_HARDWARE_ID_TEXT_SIZE ((KFSW_HARDWARE_ID_MAX_BYTES * 2U) + 1U)

/**
 * @brief Read the identifier the silicon was manufactured with, as text.
 *
 * Two boards running the same image are otherwise indistinguishable: the
 * hostname, the model and the revision are all build options and are identical
 * across a bench. This is the one field that is not, which is what makes a
 * console session or a downlink name the unit it came from.
 *
 * Written as lowercase hexadecimal, most significant byte first, with no
 * separators, so the same unit reads the same on every target.
 *
 * @param[out] text Buffer receiving the NUL-terminated identifier.
 * @param size Bytes available in @p text; @ref KFSW_HARDWARE_ID_TEXT_SIZE
 *             always suffices.
 *
 * @retval 0 The identifier was read and formatted.
 * @retval -EINVAL @p text is NULL.
 * @retval -ENOSPC @p size is too small for this SoC's identifier. Nothing is
 *                 written, because a partial identifier names another unit.
 * @retval -ENOTSUP This SoC does not report one.
 */
int kfsw_platform_get_hardware_id(char *text, size_t size);

#ifdef __cplusplus
}
#endif

#endif
