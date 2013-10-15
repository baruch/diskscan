#ifndef ARCH_INTERNAL_LINUX_H
#define ARCH_INTERNAL_LINUX_H

struct disk_dev_t {
	int fd;
	uint32_t sector_size;
};

#endif
