#ifndef _DISKSCAN_ARCH_H
#define _DISKSCAN_ARCH_H

#include <stdint.h>

int get_block_device_size(int fd, uint64_t *size_bytes, uint64_t *sector_size);

#endif
