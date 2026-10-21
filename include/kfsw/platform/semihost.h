#ifndef KFSW_PLATFORM_SEMIHOST_H
#define KFSW_PLATFORM_SEMIHOST_H

#include <stddef.h>

#define KFSW_SEMIHOST_MAX_WRITE 256U

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Write one bounded text record to the debugger, without waiting for access.
 *
 * Requires KFSW_SEMIHOST and deliberate debugger semihost enablement before
 * execution. An attached probe does not establish handler readiness. The
 * debugger must remain attached throughout the call; detach can race the guard.
 * No ISR, interrupt-masked or unprivileged calls are accepted. Concurrent calls
 * are skipped. Host latency is unbounded. Does not log failures or retry writes.
 *
 * @param text Record bytes, owned by the caller until return.
 * @param size Number of bytes, from 1 through KFSW_SEMIHOST_MAX_WRITE.
 * @return 0 on complete write, -EINVAL for invalid input, -ENOTSUP on unsupported
 * targets, -ENODEV when detached, -EWOULDBLOCK for unsafe context, -EBUSY for
 * concurrent access, or -EIO for failed, partial or invalid host write results.
 */
int kfsw_semihost_write(const char *text, size_t size);

#ifdef __cplusplus
}
#endif

#endif
