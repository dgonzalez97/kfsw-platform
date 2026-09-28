#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/crc.h>

#include <kfsw/platform/lastwords.h>

#define KFSW_LASTWORDS_MAGIC 0x4B464C57UL /* "KFLW" */
#define KFSW_LASTWORDS_VERSION 1U

struct lastwords_record {
	volatile uint32_t magic;
	uint8_t version;
	uint8_t reason;
	uint16_t reserved;
	uint32_t detail;
	uint32_t uptime_ms;
	uint32_t boot_count;
	uint32_t crc;
};

/* Not in .bss, so start-up doesn't clear it. */
static __noinit struct lastwords_record record;
/* Never wait here: a fault may interrupt the current owner. */
static atomic_t record_busy;

static uint32_t record_crc(const struct lastwords_record *value)
{
	/* CRC over everything except the CRC field. */
	return crc32_ieee((const uint8_t *)value, offsetof(struct lastwords_record, crc));
}

void kfsw_lastwords_write(enum kfsw_lastwords_reason reason, uint32_t detail, uint32_t uptime_ms,
			  uint32_t boot_count)
{
	struct lastwords_record next = {
		.magic = KFSW_LASTWORDS_MAGIC,
		.version = KFSW_LASTWORDS_VERSION,
		.reason = (uint8_t)reason,
		.detail = detail,
		.uptime_ms = uptime_ms,
		.boot_count = boot_count,
	};

	if (!atomic_cas(&record_busy, 0, 1)) {
		return;
	}
	next.crc = record_crc(&next);
	/* Invalidate before copying; publish the magic after all other fields. */
	record.magic = 0U;
	barrier_dmem_fence_full();
	next.magic = 0U;
	memcpy(&record, &next, sizeof(record));
	barrier_dmem_fence_full();
	record.magic = KFSW_LASTWORDS_MAGIC;
	atomic_clear(&record_busy);
}

bool kfsw_lastwords_take(struct kfsw_lastwords *value)
{
	bool valid;

	if (value == NULL) {
		return false;
	}
	memset(value, 0, sizeof(*value));
	if (!atomic_cas(&record_busy, 0, 1)) {
		return false;
	}

	valid = (record.magic == KFSW_LASTWORDS_MAGIC) &&
		(record.version == KFSW_LASTWORDS_VERSION) && (record.crc == record_crc(&record)) &&
		(record.reason > (uint8_t)KFSW_LASTWORDS_NONE) &&
		(record.reason <= (uint8_t)KFSW_LASTWORDS_UNKNOWN);

	if (valid) {
		value->reason = (enum kfsw_lastwords_reason)record.reason;
		value->detail = record.detail;
		value->uptime_ms = record.uptime_ms;
		value->boot_count = record.boot_count;
	}

	/* Cleared even when invalid, so it is only reported once. */
	record.magic = 0U;
	record.crc = 0U;
	atomic_clear(&record_busy);
	return valid;
}

bool kfsw_lastwords_withdraw(enum kfsw_lastwords_reason reason)
{
	if (!atomic_cas(&record_busy, 0, 1)) {
		return false;
	}
	if ((record.magic != KFSW_LASTWORDS_MAGIC) || (record.version != KFSW_LASTWORDS_VERSION) ||
	    (record.crc != record_crc(&record)) || (record.reason != (uint8_t)reason)) {
		atomic_clear(&record_busy);
		return false;
	}
	/* Clearing the magic is enough to invalidate the note. */
	record.magic = 0U;
	atomic_clear(&record_busy);
	return true;
}

const char *kfsw_lastwords_reason_name(enum kfsw_lastwords_reason reason)
{
	switch (reason) {
	case KFSW_LASTWORDS_COMMANDED:
		return "commanded";
	case KFSW_LASTWORDS_BROWNOUT:
		return "brownout";
	case KFSW_LASTWORDS_FATAL:
		return "fatal";
	case KFSW_LASTWORDS_STARVED:
		return "starved";
	case KFSW_LASTWORDS_UNKNOWN:
		return "unknown";
	case KFSW_LASTWORDS_NONE:
	default:
		return "none";
	}
}
