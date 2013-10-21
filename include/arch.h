#ifndef _DISKSCAN_ARCH_H
#define _DISKSCAN_ARCH_H

#include "libscsicmd/include/scsicmd.h"

#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

typedef struct disk_dev_t disk_dev_t;

typedef struct {
	enum result_data_e {
		DATA_FULL,         /* All data received for the request */
		DATA_PARTIAL,      /* Only some of the data was received/sent */
		DATA_NONE          /* No data was received/sent */
	} data;
	enum result_error_e {
		ERROR_NONE,        /* No error encountered */
		ERROR_CORRECTED,   /* A corrected error encountered */
		ERROR_UNCORRECTED, /* Unocrrected but non-fatal (i.e. local) error */
		ERROR_NEED_RETRY,  /* Temporary error that only merits a retry to complete */
		ERROR_FATAL,       /* A fatal error encountered, no reason to continue using disk */
	} error;

	sense_info_t info;
} io_result_t;

bool disk_dev_open(disk_dev_t *dev, const char *path);
void disk_dev_close(disk_dev_t *dev);
ssize_t disk_dev_read(disk_dev_t *dev, uint64_t offset_bytes, uint32_t len_bytes, void *buf, io_result_t *io_res);
ssize_t disk_dev_write(disk_dev_t *dev, uint64_t offset_bytes, uint32_t len_bytes, void *buf, io_result_t *io_res);
int disk_dev_read_cap(disk_dev_t *dev, uint64_t *size_bytes, uint64_t *sector_size);

#include "arch-internal.h"

#endif
