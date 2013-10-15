#include "config.h"
#include "arch.h"
#include "libscsicmd/include/scsicmd.h"

#include <linux/fs.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <scsi/sg.h>
#include <memory.h>

#include "arch-posix.c"

static int sg_ioctl(int fd, unsigned char *cdb, unsigned cdb_len,
		unsigned char *buf, unsigned buf_len,
		unsigned char *sense, unsigned sense_len,
		unsigned *buf_read, unsigned *sense_read)
{
	sg_io_hdr_t hdr;
	int ret;

	memset(&hdr, 0, sizeof(hdr));
	*sense_read = 0;
	*buf_read = 0;

	hdr.interface_id = 'S';
	hdr.dxfer_direction = SG_DXFER_FROM_DEV;
	hdr.cmd_len = cdb_len;
	hdr.mx_sb_len = sense_len;
	hdr.dxfer_len = buf_len;
	hdr.dxferp = buf;
	hdr.cmdp = cdb;
	hdr.sbp = sense;
	hdr.timeout = 120*1000;
	hdr.flags = SG_FLAG_LUN_INHIBIT;
	hdr.pack_id = 0;
	hdr.usr_ptr = 0;

	ret = ioctl(fd, SG_IO, &hdr);
	if (ret < 0)
		return -1;

#if 0
	printf("status: %d\n", hdr.status);
	printf("masked status: %d\n", hdr.masked_status);
	printf("driver status: %d\n", hdr.driver_status);
	printf("msg status: %d\n", hdr.msg_status);
	printf("host status: %d\n", hdr.host_status);
	printf("sense len: %d\n", hdr.sb_len_wr);
#endif

	if (hdr.sb_len_wr) {
		*sense_read = hdr.sb_len_wr;
		return 0;
	}
	*buf_read = hdr.dxfer_len - hdr.resid;
	return 0;
}

int disk_dev_read_cap(disk_dev_t *dev, uint64_t *size_bytes, uint64_t *sector_size)
{
	unsigned char cdb[32];
	unsigned char buf[512];
	unsigned char sense[256];
	int cdb_len;
	unsigned buf_read = 0;
	unsigned sense_read = 0;
	int ret;

	memset(buf, 0, sizeof(buf));

	cdb_len = cdb_read_capacity_10(cdb);
	ret = sg_ioctl(dev->fd, cdb, cdb_len, buf, sizeof(buf), sense, sizeof(sense), &buf_read, &sense_read);
	if (ret < 0)
		return -1;

	uint32_t size_bytes_32;
	uint32_t block_size;
	if (!parse_read_capacity_10(buf, buf_read, &size_bytes_32, &block_size))
		return -1;

	if (sense_read > 0) // TODO: Parse to see if real error or something we can ignore
		return -1;

	if (size_bytes_32 < 0xFFFFFFFF) {
		*size_bytes = (uint64_t)size_bytes_32 * 512;
		*sector_size = block_size;
		return 0;
	}

	// disk size is too large for READ CAPACITY 10, need to use READ CAPACITY 16
	cdb_len = cdb_read_capacity_16(cdb, sizeof(buf));
	ret = sg_ioctl(dev->fd, cdb, cdb_len, buf, sizeof(buf), sense, sizeof(sense), &buf_read, &sense_read);
	if (ret < 0)
		return -1;

	if (sense_read > 0) // TODO: Parse to see if real error or something we can ignore
		return -1;

	if (!parse_read_capacity_16_simple(buf, buf_read, size_bytes, &block_size))
		return -1;

	*size_bytes *= 512;
	*sector_size = block_size;
	return 0;
}
