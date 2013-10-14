#include "config.h"
#include "arch.h"

#include <sys/types.h>
#include <unistd.h>

#include "arch-posix.c"

int disk_dev_read_cap(disk_dev_t *dev, uint64_t *size_bytes, uint64_t *sector_size)
{
	off_t end = lseek(dev->fd, 0, SEEK_END);

	if (end == (off_t)-1)
		return -1;

	*size_bytes = end;
	*sector_size = 512;

	return 0;
}
