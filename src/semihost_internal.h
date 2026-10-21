#ifndef KFSW_SEMIHOST_INTERNAL_H
#define KFSW_SEMIHOST_INTERNAL_H

#include <stddef.h>

int kfsw_semihost_context(void);
long kfsw_semihost_target_write(const char *text, size_t size);

#endif
