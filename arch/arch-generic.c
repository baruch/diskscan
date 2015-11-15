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

int disk_dev_identify(disk_dev_t *dev, char *vendor, char *model, char *fw_rev, char *serial, bool *is_ata, unsigned char *ata_buf, unsigned *ata_buf_len)
{
	(void)dev;
	strcpy(vendor, "UNKNOWN");
	strcpy(model, "UNKNOWN");
	strcpy(fw_rev, "UNKN");
	strcpy(serial, "UNKNOWN");
	*is_ata = 0;
	*ata_buf_len = 0;
	*ata_buf = 0;
	return 0;
}

void mac_read(unsigned char *buf, int len)
{
	(void)len;
	*buf = 0;
}
