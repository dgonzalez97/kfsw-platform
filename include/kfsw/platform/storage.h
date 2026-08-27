#ifndef KFSW_PLATFORM_STORAGE_H
#define KFSW_PLATFORM_STORAGE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KFSW_STORAGE_MOUNT_POINT "/kfsw"

/** @brief Runtime information for the mounted K-FSW storage volume. */
struct kfsw_storage_info {
	/** Filesystem implementation name. */
	const char *filesystem;
	/** Zephyr flash device name, or "unavailable" before initialization. */
	const char *backend;
	/** Filesystem mount point. */
	const char *mount_point;
	/** Whether the storage volume is currently mounted and usable. */
	bool ready;
	/** Total filesystem capacity in bytes, or zero while unmounted. */
	uint64_t total_bytes;
	/** Available filesystem capacity in bytes, or zero while unmounted. */
	uint64_t free_bytes;
};

/**
 * @brief Initialize the configured flash storage backend.
 *
 * Repeated successful calls are harmless. This does not mount or format the
 * filesystem.
 *
 * @retval 0 The backend is initialized and available.
 * @return A negative errno value on failure.
 */
int kfsw_storage_init(void);

/**
 * @brief Mount the K-FSW storage volume.
 *
 * Repeated successful calls are harmless. If the initial no-format mount
 * reports a corrupt filesystem and every byte in the partition still has the
 * flash erase value, the partition is treated as first-boot media, formatted,
 * and mounted once more. Non-erased media is never automatically formatted.
 *
 * @retval 0 The volume is mounted and ready.
 * @return A negative errno value on failure.
 */
int kfsw_storage_mount(void);

/**
 * @brief Unmount the K-FSW storage volume.
 *
 * Repeated calls while unmounted are harmless.
 *
 * @retval 0 The volume is unmounted.
 * @return A negative errno value on failure.
 */
int kfsw_storage_unmount(void);

/** @return true when the K-FSW volume is mounted and usable. */
bool kfsw_storage_is_ready(void);

/**
 * @brief Read current K-FSW storage information.
 *
 * Capacity values are reported only while the volume is mounted.
 *
 * @param[out] info Storage information.
 *
 * @retval 0 Information was returned successfully.
 * @retval -EINVAL @p info is NULL.
 * @return A negative errno value if mounted volume statistics cannot be read.
 */
int kfsw_storage_get_info(struct kfsw_storage_info *info);

#ifdef __cplusplus
}
#endif

#endif
