#ifndef KFSW_PLATFORM_BOOT_STORE_H
#define KFSW_PLATFORM_BOOT_STORE_H

#include <stdint.h>

/**
 * Open the dedicated early-boot flash record. No filesystem is required.
 * A store NVS refuses to mount as damaged is erased and mounted empty.
 */
int kfsw_boot_store_open(void);
/** Read both budget bytes atomically. Returns -ENOENT for an empty store. */
int kfsw_boot_store_read(uint8_t *left, uint8_t *maximum);
/** Persist both budget bytes atomically; negative errno on failure. */
int kfsw_boot_store_write(uint8_t left, uint8_t maximum);

#endif
