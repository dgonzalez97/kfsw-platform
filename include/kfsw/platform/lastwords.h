#ifndef KFSW_PLATFORM_LASTWORDS_H
#define KFSW_PLATFORM_LASTWORDS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * A note a node leaves for itself before it restarts.
 *
 * The event ring is RAM and does not survive a reset, so the record an
 * operator most wants — what the node was doing in the moment before it went
 * away — is exactly the one that is lost. This keeps one small record in
 * memory the reset does not clear, written on the way down and read on the way
 * back up.
 *
 * **What survives what.** The record lives in RAM that is not zeroed at
 * start-up, so it survives any reset while the supply holds: a commanded
 * restart, a watchdog bite, a fault, the reset button. A real power loss
 * clears RAM and takes the record with it. A brown-out sits in between: the
 * voltage detector fires while the supply is still high enough to run, so the
 * note is written, and whether it is still there afterwards depends on how far
 * the rail actually fell. Surviving a dip is the case this is for; surviving a
 * disconnected power lead is not, and would need backup-domain registers or
 * flash.
 *
 * The record is validated by magic and CRC, so uninitialised memory is
 * reported as "nothing was left" rather than decoded as a record.
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
	/** Health withheld the watchdog feed on purpose. */
	KFSW_LASTWORDS_STARVED = 4,
	/** Something is going down and could not say more than that. */
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
 * @brief Leave a note, as late as the caller can manage.
 *
 * Safe from an interrupt: it writes a handful of words and a checksum, takes
 * no lock and allocates nothing. Called twice, the second call wins, because
 * whatever is closest to the restart is the more useful account.
 */
void kfsw_lastwords_write(enum kfsw_lastwords_reason reason, uint32_t detail,
			  uint32_t uptime_ms, uint32_t boot_count);

/**
 * @brief Read the note and clear it, so it is reported once.
 *
 * Returns true when a valid record was found. Clearing on read is what keeps
 * a reason from being attributed to a later restart that had nothing to do
 * with it.
 */
bool kfsw_lastwords_take(struct kfsw_lastwords *record);

/** Human-readable name for a reason, for the shell and the log. */
const char *kfsw_lastwords_reason_name(enum kfsw_lastwords_reason reason);

/**
 * @brief Start watching the supply, where the platform can.
 *
 * Returns 0 when a detector is now armed, -ENOTSUP where the SoC has none.
 * A composition that does not call this still gets every other reason.
 */
int kfsw_lastwords_watch_supply(void);

#ifdef __cplusplus
}
#endif

#endif /* KFSW_PLATFORM_LASTWORDS_H */
