#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/crc.h>

#include <kfsw/platform/lastwords.h>

#define KFSW_LASTWORDS_MAGIC 0x4B464C57UL /* "KFLW" */
#define KFSW_LASTWORDS_VERSION 1U

struct lastwords_record {
	uint32_t magic;
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

static uint32_t record_crc(const struct lastwords_record *value)
{
	/* CRC over everything except the CRC field. */
	return crc32_ieee((const uint8_t *)value, offsetof(struct lastwords_record, crc));
}

void kfsw_lastwords_write(enum kfsw_lastwords_reason reason, uint32_t detail, uint32_t uptime_ms,
			  uint32_t boot_count)
{
	record.magic = KFSW_LASTWORDS_MAGIC;
	record.version = KFSW_LASTWORDS_VERSION;
	record.reason = (uint8_t)reason;
	record.reserved = 0U;
	record.detail = detail;
	record.uptime_ms = uptime_ms;
	record.boot_count = boot_count;

	/* Written last, so an interrupted write doesn't validate. */
	record.crc = record_crc(&record);
}

bool kfsw_lastwords_take(struct kfsw_lastwords *value)
{
	bool valid;

	if (value == NULL) {
		return false;
	}
	memset(value, 0, sizeof(*value));

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
	return valid;
}

bool kfsw_lastwords_withdraw(enum kfsw_lastwords_reason reason)
{
	if ((record.magic != KFSW_LASTWORDS_MAGIC) ||
	    (record.version != KFSW_LASTWORDS_VERSION) || (record.crc != record_crc(&record)) ||
	    (record.reason != (uint8_t)reason)) {
		return false;
	}
	/* Clearing the magic is enough to invalidate the note. */
	record.magic = 0U;
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
