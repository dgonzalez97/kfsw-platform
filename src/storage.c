#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/kernel.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/util.h>

#include <kfsw/platform/storage.h>

#define KFSW_STORAGE_PARTITION_NODE DT_CHOSEN(kfsw_storage_partition)
#define KFSW_STORAGE_PARTITION_ID DT_FIXED_PARTITION_ID(KFSW_STORAGE_PARTITION_NODE)
#define KFSW_STORAGE_ERASE_SCAN_CHUNK_SIZE 64U

BUILD_ASSERT(DT_NODE_EXISTS(KFSW_STORAGE_PARTITION_NODE),
	     "kfsw,storage-partition must select a fixed partition");

FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(kfsw_storage_littlefs);

static struct fs_mount_t kfsw_storage_mount_point = {
	.type = FS_LITTLEFS,
	.fs_data = &kfsw_storage_littlefs,
	.storage_dev = (void *)KFSW_STORAGE_PARTITION_ID,
	.mnt_point = KFSW_STORAGE_MOUNT_POINT,
	.flags = FS_MOUNT_FLAG_NO_FORMAT,
};

K_MUTEX_DEFINE(kfsw_storage_lock);

static bool kfsw_storage_initialized;
static bool kfsw_storage_ready;
static const char *kfsw_storage_backend = "unavailable";

static int storage_init_locked(void)
{
	const struct flash_area *area;
	const struct device *device;
	int result;

	if (kfsw_storage_initialized) {
		return 0;
	}

	result = flash_area_open(KFSW_STORAGE_PARTITION_ID, &area);
	if (result != 0) {
		return result;
	}

	device = flash_area_get_device(area);
	if ((device == NULL) || !device_is_ready(device)) {
		flash_area_close(area);
		return -ENODEV;
	}

	kfsw_storage_backend = device->name;
	kfsw_storage_initialized = true;
	flash_area_close(area);
	return 0;
}

static int storage_partition_is_erased(bool *is_erased)
{
	const struct flash_area *area;
	uint8_t data[KFSW_STORAGE_ERASE_SCAN_CHUNK_SIZE];
	size_t offset = 0U;
	int result;

	*is_erased = false;
	result = flash_area_open(KFSW_STORAGE_PARTITION_ID, &area);
	if (result != 0) {
		return result;
	}

	const uint8_t erased_value = flash_area_erased_val(area);

	while (offset < area->fa_size) {
		const size_t read_size = MIN(sizeof(data), area->fa_size - offset);

		result = flash_area_read(area, (off_t)offset, data, read_size);
		if (result != 0) {
			flash_area_close(area);
			return result;
		}

		for (size_t index = 0U; index < read_size; index++) {
			if (data[index] != erased_value) {
				flash_area_close(area);
				return 0;
			}
		}
		offset += read_size;
	}

	*is_erased = true;
	flash_area_close(area);
	return 0;
}

int kfsw_storage_init(void)
{
	int result;

	k_mutex_lock(&kfsw_storage_lock, K_FOREVER);
	result = storage_init_locked();
	k_mutex_unlock(&kfsw_storage_lock);
	return result;
}

int kfsw_storage_mount(void)
{
	bool is_erased;
	int result;

	k_mutex_lock(&kfsw_storage_lock, K_FOREVER);

	if (kfsw_storage_ready) {
		result = 0;
		goto out;
	}

	result = storage_init_locked();
	if (result != 0) {
		goto out;
	}

	kfsw_storage_mount_point.flags = FS_MOUNT_FLAG_NO_FORMAT;
	result = fs_mount(&kfsw_storage_mount_point);
	if (result == 0) {
		kfsw_storage_ready = true;
		goto out;
	}

	if (result != -EFAULT) {
		goto out;
	}

	result = storage_partition_is_erased(&is_erased);
	if ((result != 0) || !is_erased) {
		if (result == 0) {
			result = -EFAULT;
		}
		goto out;
	}

	kfsw_storage_mount_point.flags = 0U;
	result = fs_mount(&kfsw_storage_mount_point);
	kfsw_storage_mount_point.flags = FS_MOUNT_FLAG_NO_FORMAT;
	if (result == 0) {
		kfsw_storage_ready = true;
	}

out:
	k_mutex_unlock(&kfsw_storage_lock);
	return result;
}

int kfsw_storage_unmount(void)
{
	int result = 0;

	k_mutex_lock(&kfsw_storage_lock, K_FOREVER);
	if (kfsw_storage_ready) {
		result = fs_unmount(&kfsw_storage_mount_point);
		if (result == 0) {
			kfsw_storage_ready = false;
		}
	}
	k_mutex_unlock(&kfsw_storage_lock);
	return result;
}

bool kfsw_storage_is_ready(void)
{
	bool ready;

	k_mutex_lock(&kfsw_storage_lock, K_FOREVER);
	ready = kfsw_storage_ready;
	k_mutex_unlock(&kfsw_storage_lock);
	return ready;
}

int kfsw_storage_get_info(struct kfsw_storage_info *info)
{
	struct fs_statvfs statistics;
	int result = 0;

	if (info == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&kfsw_storage_lock, K_FOREVER);
	memset(info, 0, sizeof(*info));
	info->filesystem = "LittleFS";
	info->backend = kfsw_storage_backend;
	info->mount_point = KFSW_STORAGE_MOUNT_POINT;
	info->ready = kfsw_storage_ready;

	if (kfsw_storage_ready) {
		result = fs_statvfs(KFSW_STORAGE_MOUNT_POINT, &statistics);
		if (result == 0) {
			info->total_bytes =
				(uint64_t)statistics.f_frsize * (uint64_t)statistics.f_blocks;
			info->free_bytes =
				(uint64_t)statistics.f_frsize * (uint64_t)statistics.f_bfree;
		}
	}

	k_mutex_unlock(&kfsw_storage_lock);
	return result;
}
