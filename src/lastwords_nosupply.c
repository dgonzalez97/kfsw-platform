#include <errno.h>

#include <kfsw/platform/lastwords.h>

/* No supply detector on this SoC. Reported rather than pretended: a
 * composition that believes it is watching the rail when nothing is would draw
 * the wrong conclusion from a missing brown-out record.
 */
int kfsw_lastwords_watch_supply(void)
{
	return -ENOTSUP;
}
