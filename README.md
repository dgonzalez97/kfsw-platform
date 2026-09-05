# K-FSW Platform

The layer that touches Zephyr, so nothing above it has to.

This is not a replacement HAL. It wraps a mechanism only where the wrapping
buys something: a lifecycle to own, a decision to make, or a Zephyr detail that
would otherwise leak into every caller. Where Zephyr's own API is already the
right one, services call it directly.

Current scope:

- reset cause, latched
- monotonic elapsed time
- LittleFS storage lifecycle
- watchdog

Everything here sits below the parameter service, which is why the tables that
publish these values live in the composition rather than here.

## Reset cause

Reading the cause clears the latched hardware flags, so the first reader is the
only reader. The boot service reads it once at startup and hands the value out;
anything that calls the platform again gets an empty register. That is the
whole reason `kfsw_boot_get_reset_cause()` exists.

## Storage

`kfsw_storage_init()`, `kfsw_storage_mount()` and `kfsw_storage_unmount()` own
backend validation and the `/kfsw` mount. `kfsw_storage_get_info()` reports
readiness and real capacity. The application picks the flash partition through
the `kfsw,storage-partition` devicetree chosen property.

Deliberately absent are one-for-one wrappers around `fs_open()`, `fs_read()`,
`fs_write()`, `fs_seek()` and `fs_close()`. Once the lifecycle reports ready,
services use Zephyr's filesystem API as it is. Ownership of the backend stays
here; the semantics stay Zephyr's.

Mounting starts with `FS_MOUNT_FLAG_NO_FORMAT`. A failed mount is formatted
only when a raw scan proves the whole partition still holds its erase value. A
partition that has been written to and will not mount is reported as an error,
because formatting it would destroy data that might still be recoverable.
