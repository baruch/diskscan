#include "config.h"
#include "arch.h"

#include <sys/types.h>
#include <unistd.h>

int get_block_device_size(int fd, uint64_t *size_bytes, uint64_t *sector_size)
{
	off_t end = lseek(fd, 0, SEEK_END);

	if (end == (off_t)-1)
		return -1;

	*size_bytes = end;
	*sector_size = 512;

	return 0;
}
