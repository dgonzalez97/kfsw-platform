# K-FSW Platform

The layer between Zephyr and the rest of K-FSW. It only wraps Zephyr where
there is setup to manage or a board detail to hide; otherwise services call
Zephyr directly.

| Part | Contents |
| --- | --- |
| Time | Monotonic time |
| Reset cause | Why the board restarted, read at boot |
| Hardware ID | The chip's unique ID |
| Storage | LittleFS setup and mount |
| Watchdog | Arm, feed and stop feeding |
| Last words | A reset note in retained RAM |
| Wall clock | Time from the selected RTC |

The parameter tables for these values are in the application, because this
layer sits below the parameter service.

Full documentation is on the [K-FSW site](https://dgonzalez97.github.io/k-fsw/).

## Reset cause

`kfsw_platform_get_reset_cause()` reads the hardware flags and asks the driver
to clear them. The boot service caches the result; use
`kfsw_boot_get_reset_cause()` for later reads. Clearing support depends on the
driver.

## Hardware ID

`kfsw_platform_get_hardware_id()` returns the chip's unique ID as lowercase hex,
so boards running the same image can be told apart.

```text
  STM32L496   203037324d46500c0010001f   96 bits
  Kinetis K64 ffffffff4e454487500a0013   128 bits
  RP2040      5044340578af1b1c            64 bits
```

The width depends on the SoC. A buffer that is too small returns `-ENOSPC`
instead of a truncated ID, and an SoC without an ID returns `-ENOTSUP`. The
boot service reads it once, and the boot marker, shell and board table use that
value.

A ground node reaching three boards over two links, each reporting its ID:

![Three boards reporting their IDs](https://raw.githubusercontent.com/dgonzalez97/k-fsw/develop/docs/media/multi-board-can.gif)

## Last words

A reset note in RAM that startup does not clear. The boot service reads it
once on the next boot. A reset can retain an existing note; it does not create
one by itself.

```text
  commanded restart, watchdog, fault, reset button   the note survives
  brown-out                                          depends on how far the voltage fell
  power removed                                      lost
```

A node restarted on command, reporting why afterwards:

![A node restarted, and the note it left](https://raw.githubusercontent.com/dgonzalez97/k-fsw/main/docs/media/reboot-with-a-pin.gif)

Reads check the magic, version and CRC, then clear the note. A missing note
can mean power loss, an interrupted write, or a reset before any note was
written. Check the hardware reset cause too.

## Wall clock

`kfsw_wallclock_get()` and `kfsw_wallclock_set()` use the RTC selected by
`kfsw,rtc`. Without one they return `-ENOTSUP`. Retention across reset or power
loss depends on the RTC, board configuration and backup supply.

Use monotonic time for intervals and deadlines. Setting the RTC does not
change those timers.

## Storage

`kfsw_storage_init()`, `kfsw_storage_mount()` and `kfsw_storage_unmount()`
check the backend and mount `/kfsw`. `kfsw_storage_get_info()` reports whether
it is mounted and its capacity. The application chooses the flash partition
with the `kfsw,storage-partition` devicetree property.

There are no wrappers for `fs_open()`, `fs_read()`, `fs_write()`, `fs_seek()`
or `fs_close()`. Once storage is mounted, services use the Zephyr filesystem
API directly.

Mounting uses `FS_MOUNT_FLAG_NO_FORMAT`. A partition that fails to mount is only
formatted when every byte is still erased; otherwise the error is returned and
the data is left alone.

## License

Licensed under [Apache 2.0](LICENSE). Third-party dependencies retain their
own licences.
