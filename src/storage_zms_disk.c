#include <errno.h>
#include <string.h>

#include <zephyr/drivers/disk.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/kvss/zms.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/util.h>

#include "storage_zms_disk.h"

#define PARTITION DT_CHOSEN(kfsw_storage_partition)
#define SECTOR_COUNT CONFIG_KFSW_STORAGE_ZMS_SECTOR_COUNT
#define PHYSICAL_SECTOR_SIZE (DT_REG_SIZE(PARTITION) / 2U)
#define ZMS_HEADER_SIZE 16U
#define ZMS_HEADER_METADATA 0x4201U

BUILD_ASSERT(IS_ENABLED(CONFIG_LITTLE_ENDIAN), "Pinned ZMS format uses native byte order");
BUILD_ASSERT(!IS_ENABLED(CONFIG_ZMS_ID_64BIT), "Recognition supports 32-bit ZMS IDs only");
BUILD_ASSERT(DT_REG_SIZE(PARTITION) % 2U == 0U);
BUILD_ASSERT(SECTOR_COUNT *(KFSW_STORAGE_ZMS_SECTOR_SIZE + 32U) + 160U < PHYSICAL_SECTOR_SIZE,
	     "One physical sector must hold the whole logical live set");

static struct zms_fs store;
K_MUTEX_DEFINE(disk_lock);
static uint8_t scan_buffer[KFSW_STORAGE_ZMS_SECTOR_SIZE];

/* Pinned Zephyr v4.4.0: empty ATE at the end of each physical sector.
 * Recognize only its framing, CRC and version; ZMS owns recovery and GC.
 */
static int recognize(const struct flash_area *area)
{
	const size_t write_size = flash_get_write_block_size(area->fa_dev);
	const size_t ate_size = ROUND_UP(ZMS_HEADER_SIZE, write_size);
	uint8_t header[ZMS_HEADER_SIZE];
	bool recognized = false;
	bool blank = true;
	int result;

	for (size_t offset = 0U; offset < area->fa_size; offset += sizeof(scan_buffer)) {
		const size_t size = MIN(sizeof(scan_buffer), area->fa_size - offset);

		result = flash_area_read(area, offset, scan_buffer, size);
		if (result != 0) {
			return result;
		}
		for (size_t index = 0U; index < size; index++) {
			blank &= scan_buffer[index] == flash_area_erased_val(area);
		}
	}
	if (blank) {
		return 0;
	}

	for (size_t sector = 0U; sector < 2U; sector++) {
		result = flash_area_read(area, (sector + 1U) * PHYSICAL_SECTOR_SIZE - ate_size,
					 header, sizeof(header));
		if (result != 0) {
			return result;
		}
		if (header[0] != crc8_ccitt(0xff, header + 1U, sizeof(header) - 1U) ||
		    sys_get_le16(header + 2U) != UINT16_MAX ||
		    sys_get_le32(header + 4U) != UINT32_MAX) {
			continue;
		}
		if (sys_get_le32(header + 12U) != ZMS_HEADER_METADATA) {
			return -EPROTONOSUPPORT;
		}
		recognized = true;
	}
	return recognized ? 0 : -EFAULT;
}

static int initialize(struct disk_info *disk)
{
	const struct flash_area *area;
	struct flash_pages_info page;
	int result;

	ARG_UNUSED(disk);
	k_mutex_lock(&disk_lock, K_FOREVER);
	if (store.ready) {
		result = 0;
		goto out;
	}
	result = flash_area_open(DT_FIXED_PARTITION_ID(PARTITION), &area);
	if (result != 0) {
		goto out;
	}
	if (!device_is_ready(area->fa_dev)) {
		result = -ENODEV;
		goto close;
	}
	const size_t write_size = flash_get_write_block_size(area->fa_dev);

	if (write_size == 0U || write_size > 32U || flash_area_erased_val(area) != 0xff) {
		result = -ENOTSUP;
		goto close;
	}
	/* Check every erase page, including the partition end, before mounting. */
	for (size_t offset = 0U; offset < area->fa_size; offset += page.size) {
		result = flash_get_page_info_by_offs(area->fa_dev, area->fa_off + offset, &page);
		if (result != 0) {
			goto close;
		}
		if (page.start_offset != area->fa_off + offset ||
		    PHYSICAL_SECTOR_SIZE % page.size != 0U) {
			result = -EINVAL;
			goto close;
		}
	}
	result = recognize(area);
	if (result == 0) {
		memset(&store, 0, sizeof(store));
		store.flash_device = area->fa_dev;
		store.offset = area->fa_off;
		store.sector_size = PHYSICAL_SECTOR_SIZE;
		store.sector_count = 2U;
		result = zms_mount(&store);
	}
close:
	flash_area_close(area);
out:
	k_mutex_unlock(&disk_lock);
	return result;
}

static int status(struct disk_info *disk)
{
	int result;

	ARG_UNUSED(disk);
	k_mutex_lock(&disk_lock, K_FOREVER);
	result = store.ready ? DISK_STATUS_OK : DISK_STATUS_UNINIT;
	k_mutex_unlock(&disk_lock);
	return result;
}

static int read_sector(uint8_t *data, uint32_t sector)
{
	ssize_t result = zms_get_data_length(&store, sector + 1U);

	if (result == -ENOENT) {
		memset(data, 0xff, KFSW_STORAGE_ZMS_SECTOR_SIZE);
		return 0;
	}
	if (result < 0) {
		return result;
	}
	if (result != KFSW_STORAGE_ZMS_SECTOR_SIZE) {
		return -EIO;
	}
	result = zms_read(&store, sector + 1U, data, KFSW_STORAGE_ZMS_SECTOR_SIZE);
	return result == KFSW_STORAGE_ZMS_SECTOR_SIZE ? 0 : (result < 0 ? result : -EIO);
}

static bool valid_request(const void *data, uint32_t start, uint32_t count)
{
	return data != NULL && count != 0U && start < SECTOR_COUNT && count <= SECTOR_COUNT - start;
}

static int read_disk(struct disk_info *disk, uint8_t *data, uint32_t start, uint32_t count)
{
	int result = 0;

	ARG_UNUSED(disk);
	if (!valid_request(data, start, count)) {
		return -EINVAL;
	}
	k_mutex_lock(&disk_lock, K_FOREVER);
	if (!store.ready) {
		result = -EACCES;
		goto out;
	}
	for (uint32_t index = 0U; index < count; index++) {
		result = read_sector(data + index * KFSW_STORAGE_ZMS_SECTOR_SIZE, start + index);
		if (result != 0) {
			break;
		}
	}
out:
	k_mutex_unlock(&disk_lock);
	return result;
}

static int write_disk(struct disk_info *disk, const uint8_t *data, uint32_t start, uint32_t count)
{
	int result = 0;

	ARG_UNUSED(disk);
	if (!valid_request(data, start, count)) {
		return -EINVAL;
	}
	k_mutex_lock(&disk_lock, K_FOREVER);
	if (!store.ready) {
		result = -EACCES;
		goto out;
	}
	for (uint32_t index = 0U; index < count; index++) {
		ssize_t written = zms_write(&store, start + index + 1U,
					    data + index * KFSW_STORAGE_ZMS_SECTOR_SIZE,
					    KFSW_STORAGE_ZMS_SECTOR_SIZE);

		if (written != 0 && written != KFSW_STORAGE_ZMS_SECTOR_SIZE) {
			result = written < 0 ? written : -EIO;
			/* A failed append may have partially programmed flash. Recover
			 * through mount before accepting any further disk operations.
			 */
			store.ready = false;
			break;
		}
	}
out:
	k_mutex_unlock(&disk_lock);
	return result;
}

static int ioctl_disk(struct disk_info *disk, uint8_t command, void *buffer)
{
	int result = 0;

	if (command == DISK_IOCTL_CTRL_INIT) {
		return initialize(disk);
	}
	k_mutex_lock(&disk_lock, K_FOREVER);
	switch (command) {
	case DISK_IOCTL_CTRL_DEINIT:
		store.ready = false;
		break;
	case DISK_IOCTL_CTRL_SYNC:
		result = store.ready ? 0 : -EACCES;
		break;
	case DISK_IOCTL_GET_SECTOR_COUNT:
	case DISK_IOCTL_GET_SECTOR_SIZE:
	case DISK_IOCTL_GET_ERASE_BLOCK_SZ:
		if (buffer == NULL) {
			result = -EINVAL;
			break;
		}
		*(uint32_t *)buffer = command == DISK_IOCTL_GET_SECTOR_COUNT ? SECTOR_COUNT
				      : command == DISK_IOCTL_GET_SECTOR_SIZE
					      ? KFSW_STORAGE_ZMS_SECTOR_SIZE
					      : 1U;
		break;
	default:
		result = -ENOTSUP;
	}
	k_mutex_unlock(&disk_lock);
	return result;
}

int kfsw_storage_zms_is_erased(bool *is_erased)
{
	int result = 0;

	*is_erased = false;
	k_mutex_lock(&disk_lock, K_FOREVER);
	if (!store.ready) {
		result = -EACCES;
		goto out;
	}
	for (uint32_t sector = 0U; sector < SECTOR_COUNT; sector++) {
		result = read_sector(scan_buffer, sector);
		if (result != 0) {
			goto out;
		}
		for (size_t index = 0U; index < sizeof(scan_buffer); index++) {
			if (scan_buffer[index] != 0xff) {
				goto out;
			}
		}
	}
	*is_erased = true;
out:
	k_mutex_unlock(&disk_lock);
	return result;
}

static const struct disk_operations operations = {
	.init = initialize,
	.status = status,
	.read = read_disk,
	.write = write_disk,
	.ioctl = ioctl_disk,
};

static struct disk_info disk = {
	.name = KFSW_STORAGE_ZMS_DISK_NAME,
	.ops = &operations,
};

static int register_disk(void)
{
	return disk_access_register(&disk);
}

SYS_INIT(register_disk, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
