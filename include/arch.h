#ifndef _DISKSCAN_ARCH_H
#define _DISKSCAN_ARCH_H

#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

typedef struct disk_dev_t disk_dev_t;

bool disk_dev_open(disk_dev_t *dev, const char *path);
void disk_dev_close(disk_dev_t *dev);
ssize_t disk_dev_read(disk_dev_t *dev, uint64_t offset_bytes, uint32_t len_bytes, void *buf);
ssize_t disk_dev_write(disk_dev_t *dev, uint64_t offset_bytes, uint32_t len_bytes, void *buf);
int disk_dev_read_cap(disk_dev_t *dev, uint64_t *size_bytes, uint64_t *sector_size);

#include "arch-internal.h"

#endif
