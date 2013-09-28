#include "arch.h"

#include <linux/fs.h>
#include <sys/ioctl.h>
#include <sys/types.h>

int get_block_device_size(int fd, uint64_t *size_bytes, uint64_t *sector_size)
{
	if (ioctl(fd, BLKGETSIZE64, size_bytes) < 0) {
		return -1;
	}

	if (ioctl(fd, BLKSSZGET, sector_size) < 0) {
		return -1;
	}

	return 0;
}
