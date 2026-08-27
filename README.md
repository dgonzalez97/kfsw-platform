# K-FSW Platform

Small project-owned platform capability layer over Zephyr.

This is intentionally not a replacement vendor HAL.

Current scope:

- reset cause
- monotonic elapsed time
- LittleFS storage backend lifecycle

Planned scope:

- watchdog mechanism
- hardware identity/capabilities

## Storage boundary

`kfsw_storage_init()`, `kfsw_storage_mount()`, and
`kfsw_storage_unmount()` own backend validation and the `/kfsw` mount
lifecycle. `kfsw_storage_get_info()` exposes readiness and real filesystem
capacity. The application selects the fixed flash partition through the
project-owned `kfsw,storage-partition` devicetree chosen property.

The platform layer deliberately does not wrap Zephyr's `fs_open()`,
`fs_read()`, `fs_write()`, `fs_seek()`, or `fs_close()` one-for-one. Services
use the Zephyr filesystem API after the lifecycle reports ready, preserving
Zephyr semantics while keeping backend ownership in the platform layer.

Mounting starts with `FS_MOUNT_FLAG_NO_FORMAT`. A failed LittleFS mount is
formatted only when a raw flash scan proves the complete partition still has
its erase value. A non-erased partition that cannot mount is reported as an
error so potentially recoverable data is not silently destroyed.
