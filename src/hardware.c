#include <errno.h>
#include <stdint.h>
#include <sys/types.h>

#include <zephyr/drivers/hwinfo.h>

#include <kfsw/platform/hardware.h>

int kfsw_platform_get_hardware_id(char *text, size_t size)
{
	static const char digits[] = "0123456789abcdef";
	uint8_t id[KFSW_HARDWARE_ID_MAX_BYTES];
	ssize_t length;
	size_t needed;
	size_t index;

	if (text == NULL) {
		return -EINVAL;
	}

	/* A driver that is absent answers -ENOSYS and one that is present but
	 * has nothing to report answers zero. Neither is a failure worth a
	 * distinct code here: both mean this SoC cannot name itself.
	 */
	length = hwinfo_get_device_id(id, sizeof(id));
	if (length <= 0) {
		return -ENOTSUP;
	}

	needed = ((size_t)length * 2U) + 1U;
	if (size < needed) {
		return -ENOSPC;
	}

	for (index = 0U; index < (size_t)length; index++) {
		text[index * 2U] = digits[id[index] >> 4];
		text[(index * 2U) + 1U] = digits[id[index] & 0x0fU];
	}
	text[needed - 1U] = '\0';

	return 0;
}
