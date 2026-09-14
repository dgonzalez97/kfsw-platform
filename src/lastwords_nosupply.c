#include <errno.h>

#include <kfsw/platform/lastwords.h>

/* This SoC has no supply detector. */
int kfsw_lastwords_watch_supply(void)
{
	return -ENOTSUP;
}
