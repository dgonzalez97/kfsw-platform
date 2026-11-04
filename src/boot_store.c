#include <errno.h>

#include <zephyr/drivers/flash.h>
#include <zephyr/kvss/nvs.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>

#include <kfsw/platform/boot_store.h>

#define STORE_NODE DT_CHOSEN(kfsw_boot_budget_partition)
#define STORE_ID 1U
#define RECORD_SIZE 8U

static struct nvs_fs store;
static bool ready;

int kfsw_boot_store_open(void)
{
	const struct flash_area *area;
	struct flash_pages_info page;
	int result;

	if (ready) {
		return 0;
	}
	result = flash_area_open(DT_FIXED_PARTITION_ID(STORE_NODE), &area);
	if (result != 0) {
		return result;
	}
	store.flash_device = flash_area_get_device(area);
	result = flash_get_page_info_by_offs(store.flash_device, area->fa_off, &page);
	if (result == 0 && (area->fa_size != 2U * page.size || page.size > UINT16_MAX)) {
		result = -EINVAL;
	}
	if (result == 0) {
		store.offset = area->fa_off;
		store.sector_size = (uint16_t)page.size;
		store.sector_count = 2U;
		result = nvs_mount(&store);
	}
	/* NVS refuses both sectors closed and a damaged write area for good. An
	   empty store reads as a full budget, so erasing is the safe way out. */
	if ((result == -EDEADLK) || (result == -ESPIPE)) {
		result = flash_area_flatten(area, 0, area->fa_size);
		if (result == 0) {
			result = nvs_mount(&store);
		}
	}
	flash_area_close(area);
	ready = result == 0;
	return result;
}

int kfsw_boot_store_read(uint8_t *left, uint8_t *maximum)
{
	uint8_t record[RECORD_SIZE];
	ssize_t result;

	if (!ready) {
		return -ENODEV;
	}
	result = nvs_read(&store, STORE_ID, record, sizeof(record));
	if (result < 0) {
		return (int)result;
	}
	if (result != sizeof(record) || record[0] != 'B' || record[1] != 1U ||
	    sys_get_le32(&record[4]) != crc32_ieee(record, 4U)) {
		return -EBADMSG;
	}
	*left = record[2];
	*maximum = record[3];
	return 0;
}

int kfsw_boot_store_write(uint8_t left, uint8_t maximum)
{
	uint8_t record[RECORD_SIZE] = {'B', 1U, left, maximum};
	ssize_t result;

	if (!ready) {
		return -ENODEV;
	}
	sys_put_le32(crc32_ieee(record, 4U), &record[4]);
	result = nvs_write(&store, STORE_ID, record, sizeof(record));
	return result < 0 ? (int)result : 0;
}
