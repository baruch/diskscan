#include "config.h"
#include "arch.h"

#include <sys/disk.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include "arch-posix.c"

int disk_dev_read_cap(disk_dev_t *dev, uint64_t *size_bytes, uint64_t *sector_size)
{
	if (ioctl(dev->fd, DIOCGMEDIASIZE, size_bytes) < 0) {
		return -1;
	}

	if (ioctl(dev->fd, DIOCGSECTORSIZE, sector_size) < 0) {
		return -1;
	}

	return 0;
}
