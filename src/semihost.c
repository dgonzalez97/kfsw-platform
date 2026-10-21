#include <errno.h>
#include <zephyr/sys/atomic.h>
#include <kfsw/platform/semihost.h>

#include "semihost_internal.h"

static atomic_t writing;

int kfsw_semihost_write(const char *text, size_t size)
{
	int result;
	long unwritten;

	if ((text == NULL) || (size == 0U) || (size > KFSW_SEMIHOST_MAX_WRITE)) {
		return -EINVAL;
	}
	result = kfsw_semihost_context();
	if (result != 0) {
		return result;
	}
	if (!atomic_cas(&writing, 0, 1)) {
		return -EBUSY;
	}
	unwritten = kfsw_semihost_target_write(text, size);
	atomic_clear(&writing);
	return unwritten == 0L ? 0 : -EIO;
}
