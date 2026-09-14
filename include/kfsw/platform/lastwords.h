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

/** Why the node was going down. */
enum kfsw_lastwords_reason {
	/** Nothing was written, or what was there did not validate. */
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

/** What the previous run left behind. */
struct kfsw_lastwords {
	/** Why it was going down. */
	enum kfsw_lastwords_reason reason;
	/** Caller-defined, meaningful only alongside the reason. */
	uint32_t detail;
	/** Milliseconds the previous run had been up when it wrote this. */
	uint32_t uptime_ms;
	/** How many restarts had happened before it, as the writer knew it. */
	uint32_t boot_count;
};

/**
 * @brief Write a note. Safe from an interrupt; a second call replaces the first.
 */
void kfsw_lastwords_write(enum kfsw_lastwords_reason reason, uint32_t detail,
			  uint32_t uptime_ms, uint32_t boot_count);

/**
 * @brief Read the note and clear it. Returns true when a valid note was found.
 */
bool kfsw_lastwords_take(struct kfsw_lastwords *record);

/**
 * @brief Clear the note if its reason matches. Returns true when it was cleared.
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
