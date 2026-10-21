#ifndef KFSW_STORAGE_ZMS_DISK_H
#define KFSW_STORAGE_ZMS_DISK_H

#include <stdbool.h>

#define KFSW_STORAGE_ZMS_DISK_NAME "KFSW"
#define KFSW_STORAGE_ZMS_SECTOR_SIZE 4096U

int kfsw_storage_zms_is_erased(bool *is_erased);

#endif
