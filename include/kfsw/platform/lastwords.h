#ifndef KFSW_PLATFORM_LASTWORDS_H
#define KFSW_PLATFORM_LASTWORDS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * A note written before a restart, kept in RAM that start-up does not clear.
 * It survives a reset but not a power loss, and a magic number and a CRC
 * validate it.
 */

/** Reset reason. */
enum kfsw_lastwords_reason {
	/** No valid reset note. */
	KFSW_LASTWORDS_NONE = 0,
	/** An operator or the ground asked for a restart. */
	KFSW_LASTWORDS_COMMANDED = 1,
	/** The supply fell far enough for the detector to fire. */
	KFSW_LASTWORDS_BROWNOUT = 2,
	/** A fatal error handler ran. */
	KFSW_LASTWORDS_FATAL = 3,
	/** Health stopped feeding the watchdog. */
	KFSW_LASTWORDS_STARVED = 4,
	/** Restart without a more specific reason. */
	KFSW_LASTWORDS_UNKNOWN = 5,
};

/** Retained reset note. */
struct kfsw_lastwords {
	/** Reset reason. */
	enum kfsw_lastwords_reason reason;
	/** Caller-defined, meaningful only alongside the reason. */
	uint32_t detail;
	/** Uptime in milliseconds when the note was written. */
	uint32_t uptime_ms;
	/** Boot count supplied by the writer. */
	uint32_t boot_count;
};

/**
 * @brief Write a note without waiting, including from an interrupt.
 * A concurrent or nested operation is ignored while another owns the record.
 * Sequential writes replace the previous note; there is no severity arbitration.
 */
void kfsw_lastwords_write(enum kfsw_lastwords_reason reason, uint32_t detail, uint32_t uptime_ms,
			  uint32_t boot_count);

/**
 * @brief Read the note and clear it. Returns true when a valid note was found.
 * Returns false without clearing the note if another operation owns it.
 */
bool kfsw_lastwords_take(struct kfsw_lastwords *record);

/**
 * @brief Clear the note if its reason matches. Returns true when it was cleared.
 * Returns false if another operation owns the record.
 */
bool kfsw_lastwords_withdraw(enum kfsw_lastwords_reason reason);

/** Human-readable name for a reason, for the shell and the log. */
const char *kfsw_lastwords_reason_name(enum kfsw_lastwords_reason reason);

/**
 * @brief Start the supply voltage detector.
 *
 * Returns 0 when a detector is armed, or -ENOTSUP when the SoC has none.
 */
int kfsw_lastwords_watch_supply(void);

#ifdef __cplusplus
}
#endif

#endif /* KFSW_PLATFORM_LASTWORDS_H */
