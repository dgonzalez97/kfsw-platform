# K-FSW Platform

The layer that touches Zephyr, so nothing above it has to.

This is not a replacement HAL. It wraps a mechanism only where the wrapping
buys something: a lifecycle to own, a decision to make, or a Zephyr detail that
would otherwise leak into every caller. Where Zephyr's own API is already the
right one, services call it directly.

| Mechanism | What it is |
| --- | --- |
| Time | Monotonic elapsed time |
| Reset cause | Why the board restarted, latched at boot |
| Storage | The LittleFS lifecycle and its mount |
| Watchdog | Arm, feed, stop feeding — no policy |
| Last words | A note that survives a restart, written on the way down |

Everything here sits below the parameter service, which is why the tables that
publish these values live in the composition rather than here.

Full documentation is on the
[K-FSW site](https://dgonzalez97.github.io/k-fsw/); what follows is the
reasoning behind the parts that are easy to get wrong.

## Reset cause

Reading the cause clears the latched hardware flags, so the first reader is the
only reader. The boot service reads it once at startup and hands the value out;
anything that calls the platform again gets an empty register. That is the
whole reason `kfsw_boot_get_reset_cause()` exists.

## Last words

The event ring is RAM, so what a node was doing in the moment before it went
away is exactly the record a reset destroys. One small record lives in memory
that start-up does not clear, written on the way down and read on the way back
up by the boot service.

```text
  commanded restart, watchdog, fault, reset button   the note survives
  a brown-out dip                                    depends how far the rail fell
  the power lead pulled                              gone, and RAM with it
```

A node restarted on command, saying afterwards why it went down:

![A node restarted, and the note it left](https://raw.githubusercontent.com/dgonzalez97/k-fsw/main/docs/media/reboot-with-a-pin.gif)

Surviving a dip is the case this is for. It is validated by magic and CRC with
the checksum written last, so a reset landing mid-write leaves something that
reports as *nothing was left* rather than as a wrong answer. Reading consumes
it, because attributing one restart's reason to the next is worse than silence.

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
