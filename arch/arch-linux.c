#include "arch.h"
#include "libscsicmd/include/scsicmd.h"
#include "libscsicmd/include/ata.h"
#include "libscsicmd/include/ata_parse.h"
#include "verbose.h"

#include <linux/fs.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <scsi/sg.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <net/if.h>
#include <netinet/in.h>
#include <mntent.h>

#define LONG_TIMEOUT (60*1000) // 1 minutes
#define SHORT_TIMEOUT (5*1000) // 5 seconds

static void strtrim(char *s)
{
	char *t;

	// Skip initial spaces
	for (t = s; *t && isspace(*t); t++)
		;

	if (t != s) {
		// Copy content to start
		while (*t && !isspace(*t)) {
			*s++ = *t++;
		}
		*s = 0;
	} else {
		while (*t && !isspace(*t))
			t++;
		*t = 0;
	}
}

static enum result_error_e sense_to_error(sense_info_t *info)
{
	// TODO: May need a more granular decision based on asc/ascq

	switch (info->sense_key) {
		case SENSE_KEY_NO_SENSE:
			return ERROR_NONE;

		case SENSE_KEY_RECOVERED_ERROR:
			return ERROR_CORRECTED;

        case SENSE_KEY_MEDIUM_ERROR:
			return ERROR_UNCORRECTED;

        case SENSE_KEY_UNIT_ATTENTION:
        case SENSE_KEY_NOT_READY:
        case SENSE_KEY_ABORTED_COMMAND:
			return ERROR_NEED_RETRY;

        case SENSE_KEY_HARDWARE_ERROR:
        case SENSE_KEY_ILLEGAL_REQUEST:
        case SENSE_KEY_DATA_PROTECT:
        case SENSE_KEY_BLANK_CHECK:
        case SENSE_KEY_VENDOR_SPECIFIC:
        case SENSE_KEY_COPY_ABORTED:
        case SENSE_KEY_RESERVED_C:
        case SENSE_KEY_VOLUME_OVERFLOW:
        case SENSE_KEY_MISCOMPARE:
        case SENSE_KEY_COMPLETED:
			return ERROR_FATAL;
	}

	ERROR("BUG: Cannot translate sense 0x%02X to error code", info->sense_key);
	return ERROR_UNKNOWN;
}

static const char *host_status_to_str(int host_status)
{
	switch (host_status) {
		case 0x00: return "DID_OK: No error";
		case 0x01: return "DID_NO_CONNECT: Couldn't connect before timeout period";
		case 0x02: return "DID_BUS_BUSY: BUS stayed busy through time out period";
		case 0x03: return "DID_TIME_OUT: TIMED OUT for other reason";
		case 0x04: return "DID_BAD_TARGET: BAD target";
		case 0x05: return "DID_ABORT: Told to abort for some other reason";
		case 0x06: return "DID_PARITY: Parity error";
		case 0x07: return "DID_ERROR: internal error";
		case 0x08: return "DID_RESET: Reset by somebody";
		case 0x09: return "DID_BAD_INTR: Got an interrupt we weren't expecting";
		default: return "Unknown host status";
	}
}

static const char *driver_status_low_to_str(int driver_status)
{
	switch (driver_status) {
		case 0x00: return "DRIVER_OK: No error";
		case 0x01: return "DRIVER_BUSY: not used";
		case 0x02: return "DRIVER_SOFT: not used";
		case 0x03: return "DRIVER_MEDIA: not used";
		case 0x04: return "DRIVER_ERROR: internal driver error";
		case 0x05: return "DRIVER_INVALID: finished (DID_BAD_TARGET or DID_ABORT)";
		case 0x06: return "DRIVER_TIMEOUT: finished with timeout";
		case 0x07: return "DRIVER_HARD: finished with fatal error";
		case 0x08: return "DRIVER_SENSE: had sense information available";
		default: return "Unknown driver status";
	}
}

static const char *driver_status_high_to_str(int driver_status)
{
	switch (driver_status) {
		case 0: return "No suggestion";
		case 0x10: return "SUGGEST_RETRY: retry the SCSI request";
		case 0x20: return "SUGGEST_ABORT: abort the request";
		case 0x30: return "SUGGEST_REMAP: remap the block (not yet implemented)";
		case 0x40: return "SUGGEST_DIE: let the kernel panic";
		case 0x80: return "SUGGEST_SENSE: get sense information from the device";
		case 0xff: return "SUGGEST_IS_OK: nothing to be done";
		default: return "Unknown suggestion";
	}
}

static const char *status_code_to_str(int status)
{
	switch (status) {
		case 0x00: return "GOOD";
		case 0x01: return "CHECK_CONDITION";
		case 0x02: return "CONDITION_GOOD";
		case 0x04: return "BUSY";
		case 0x08: return "INTERMEDIATE_GOOD";
		case 0x0a: return "INTERMEDIATE_C_GOOD";
		case 0x0c: return "RESERVATION_CONFLICT";
		default: return "Unknown status";
	}
}

static const char *driver_status_to_str(int driver_status)
{
	static char buf[256];

	snprintf(buf, sizeof(buf), "%s %s",
			driver_status_low_to_str(driver_status & 0x0F),
			driver_status_high_to_str(driver_status & 0xF0));
	return buf;
}

static int sg_ioctl(int fd, unsigned char *cdb, unsigned cdb_len,
		unsigned char *buf, unsigned buf_len,
		int dxfer_direction, unsigned timeout,
		unsigned char *sense, unsigned sense_len,
		unsigned *buf_read, unsigned *sense_read,
		io_result_t *io_res)
{
	sg_io_hdr_t hdr;
	int ret;

	memset(&hdr, 0, sizeof(hdr));
	memset(io_res, 0, sizeof(*io_res));

	*sense_read = 0;
	*buf_read = 0;

	hdr.interface_id = 'S';
	hdr.dxfer_direction = dxfer_direction;
	hdr.cmd_len = cdb_len;
	hdr.mx_sb_len = sense_len;
	hdr.dxfer_len = buf_len;
	hdr.dxferp = buf;
	hdr.cmdp = cdb;
	hdr.sbp = sense;
	hdr.timeout = timeout; /* timeout in milliseconds */
	hdr.flags = SG_FLAG_LUN_INHIBIT;
	hdr.pack_id = 0;
	hdr.usr_ptr = 0;

	ret = ioctl(fd, SG_IO, &hdr);
	if (ret < 0) {
		ERROR("Failed to issue ioctl to device errno=%d: %s", errno, strerror(errno));
		io_res->error = ERROR_FATAL;
		io_res->data = DATA_NONE;
		return -1;
	}

#if 0
	if (hdr.status || hdr.driver_status || hdr.msg_status || hdr.host_status || hdr.sb_len_wr)
	{
		printf("status: %d %s\n", hdr.status, status_code_to_str(hdr.status));
		printf("masked status: %d\n", hdr.masked_status);
		printf("driver status: %d %s\n", hdr.driver_status, driver_status_to_str(hdr.driver_status));
		printf("msg status: %d\n", hdr.msg_status);
		printf("host status: %d = %s\n", hdr.host_status, host_status_to_str(hdr.host_status));
		printf("sense len: %d\n", hdr.sb_len_wr);
	}
#endif

	*buf_read = hdr.dxfer_len - hdr.resid;

	if (*buf_read == buf_len)
		io_res->data = DATA_FULL;
	else if (*buf_read == 0)
		io_res->data = DATA_NONE;
	else
		io_res->data = DATA_PARTIAL;

	if (hdr.sb_len_wr) {
		memcpy(io_res->sense, sense, hdr.sb_len_wr);
		io_res->sense_len = hdr.sb_len_wr;

		*sense_read = hdr.sb_len_wr;

		// Error with sense, parse the sense
		if (scsi_parse_sense(sense, hdr.sb_len_wr, &io_res->info)) {
			io_res->error = sense_to_error(&io_res->info);
		} else {
			// Parsing of the sense failed, assume the worst
			io_res->error = ERROR_UNKNOWN;
		}
		return 0;
	}

	if (hdr.status != 0) {
		// No sense but we have an error, consider it fatal if no data returned
		ERROR("IO failed with no sense: status=%d (%s) mask=%d driver=%d (%s) msg=%d host=%d (%s)",
				hdr.status, status_code_to_str(hdr.status),
				hdr.masked_status,
				hdr.driver_status, driver_status_to_str(hdr.driver_status),
				hdr.msg_status,
				hdr.host_status, host_status_to_str(hdr.host_status));

		if (*buf_read == 0)
			io_res->error = ERROR_UNKNOWN;
		return 0;
	}

	io_res->error = ERROR_NONE;
	return 0;
}

static disk_mount_e mount_point_check(struct mntent *mnt)
{
	char *next = mnt->mnt_opts;
	char *opt;

	/* Device is mounted, check it */
	while ((opt = strtok(next, ", \t\r\n")) != NULL) {
		next = NULL; // continue scanning for this string
		if (strcmp(opt, "rw") == 0)
			return DISK_MOUNTED_RW;
	}

	return DISK_MOUNTED_RO;
}

disk_mount_e disk_dev_mount_state(const char *path)
{
	struct stat dev_st_buf;
	struct stat st_buf;
	FILE *f = NULL;
	struct mntent *mnt;
	disk_mount_e state = DISK_MOUNTED_RW; // assume the worst

	f = setmntent("/proc/mounts", "r");
	if (f == NULL) {
		ERROR("Failed to open /proc/mounts to know the state, errno=%d", errno);
		goto Exit;
	}

	if (stat(path, &dev_st_buf) != 0) {
		ERROR("Failed to stat the path %s, errno=%d", path, errno);
		goto Exit;
	}

	if (!S_ISBLK(dev_st_buf.st_mode)) {
		ERROR("Device %s is not a block device", path);
		goto Exit; // We only want block devices
	}

	// From here we assume the disk is not mounted
	state = DISK_NOT_MOUNTED;

	while ((mnt = getmntent(f)) != NULL) {
		disk_mount_e cur_state = DISK_NOT_MOUNTED;

		/* Ignore non-full-path entries */
		if (mnt->mnt_fsname[0] != '/')
			continue;
		/* Check for a name prefix match, we may check a full block device and a partition is mounted */
		if (strncmp(path, mnt->mnt_fsname, strlen(path)) == 0) {
			cur_state = mount_point_check(mnt);
			if (cur_state > state)
				state = cur_state;
			continue;
		}
		/* Check for an underlying device match (name may have changed in between actions) */
		if (stat(mnt->mnt_fsname, &st_buf) == 0) {
			if (!S_ISBLK(st_buf.st_mode))
				continue;
			if (dev_st_buf.st_rdev == st_buf.st_rdev) {
				cur_state = mount_point_check(mnt);
				if (cur_state > state)
					state = cur_state;
			}
		}
	}

Exit:
	if (f)
		endmntent(f);
	return state;
}

bool disk_dev_open(disk_dev_t *dev, const char *path)
{
	dev->fd = open(path, O_RDWR|O_DIRECT);
	return dev->fd >= 0;
}

void disk_dev_close(disk_dev_t *dev)
{
	close(dev->fd);
	dev->fd = -1;
}

void disk_dev_cdb_out(disk_dev_t *dev, unsigned char *cdb, unsigned cdb_len, unsigned char *buf, unsigned buf_size, unsigned *buf_read, unsigned char *sense, unsigned sense_size, unsigned *sense_read, io_result_t *io_res)
{
	sg_ioctl(dev->fd, cdb, cdb_len, buf, buf_size, SG_DXFER_TO_DEV, LONG_TIMEOUT, sense, sense_size, buf_read, sense_read, io_res);
}

void disk_dev_cdb_in(disk_dev_t *dev, unsigned char *cdb, unsigned cdb_len, unsigned char *buf, unsigned buf_size, unsigned *buf_read, unsigned char *sense, unsigned sense_size, unsigned *sense_read, io_result_t *io_res)
{
	sg_ioctl(dev->fd, cdb, cdb_len, buf, buf_size, SG_DXFER_FROM_DEV, LONG_TIMEOUT, sense, sense_size, buf_read, sense_read, io_res);
}

ssize_t disk_dev_read(disk_dev_t *dev, uint64_t offset_bytes, uint32_t len_bytes, void *buf, io_result_t *io_res)
{
	unsigned char cdb[32];
	unsigned char sense[128];
	int cdb_len;
	unsigned buf_read = 0;
	unsigned sense_read = 0;
	int ret;

	memset(buf, 0, len_bytes);
	memset(io_res, 0, sizeof(*io_res));

	cdb_len = cdb_read_10(cdb, false, offset_bytes / dev->sector_size, len_bytes / dev->sector_size);
	ret = sg_ioctl(dev->fd, cdb, cdb_len, buf, len_bytes, SG_DXFER_FROM_DEV, LONG_TIMEOUT, sense, sizeof(sense), &buf_read, &sense_read, io_res);
	if (ret < 0) {
		return -1;
	}

	if (buf_read < len_bytes && sense_read > 0) {
		VERBOSE("not all read: requested=%u read=%u sense=%u", len_bytes, buf_read, sense_read);
		return -1;
	}

	return buf_read;
}

ssize_t disk_dev_write(disk_dev_t *dev, uint64_t offset_bytes, uint32_t len_bytes, void *buf, io_result_t *io_res)
{
	unsigned char cdb[32];
	unsigned char sense[128];
	int cdb_len;
	unsigned buf_read = 0;
	unsigned sense_read = 0;
	int ret;

	memset(buf, 0, len_bytes);
	memset(io_res, 0, sizeof(*io_res));

	cdb_len = cdb_write_10(cdb, false, offset_bytes / dev->sector_size, len_bytes / dev->sector_size);
	ret = sg_ioctl(dev->fd, cdb, cdb_len, buf, len_bytes, SG_DXFER_TO_DEV, LONG_TIMEOUT, sense, sizeof(sense), &buf_read, &sense_read, io_res);
	if (ret < 0) {
		return -1;
	}

	if (buf_read < len_bytes && sense_read > 0) {
		VERBOSE("not all read: requested=%u read=%u sense=%u", len_bytes, buf_read, sense_read);
		return -1;
	}

	return buf_read;
}

int disk_dev_read_cap(disk_dev_t *dev, uint64_t *size_bytes, uint64_t *sector_size)
{
	unsigned char cdb[32];
	unsigned char buf[512];
	unsigned char sense[128];
	int cdb_len;
	unsigned buf_read = 0;
	unsigned sense_read = 0;
	int ret;
	io_result_t io_res;

	memset(buf, 0, sizeof(buf));

	cdb_len = cdb_read_capacity_10(cdb);
	ret = sg_ioctl(dev->fd, cdb, cdb_len, buf, sizeof(buf), SG_DXFER_FROM_DEV, SHORT_TIMEOUT, sense, sizeof(sense), &buf_read, &sense_read, &io_res);
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
		dev->sector_size = *sector_size = block_size;
		return 0;
	}

	// disk size is too large for READ CAPACITY 10, need to use READ CAPACITY 16
	cdb_len = cdb_read_capacity_16(cdb, sizeof(buf));
	ret = sg_ioctl(dev->fd, cdb, cdb_len, buf, sizeof(buf), SG_DXFER_FROM_DEV, SHORT_TIMEOUT, sense, sizeof(sense), &buf_read, &sense_read, &io_res);
	if (ret < 0)
		return -1;

	if (sense_read > 0) // TODO: Parse to see if real error or something we can ignore
		return -1;

	if (!parse_read_capacity_16_simple(buf, buf_read, size_bytes, &block_size))
		return -1;

	*size_bytes *= 512;
	dev->sector_size = *sector_size = block_size;
	return 0;
}


int disk_dev_identify(disk_dev_t *dev, char *vendor, char *model, char *fw_rev, char *serial, bool *is_ata, unsigned char *ata_buf, unsigned *ata_buf_len)
{
	unsigned char cdb[32];
	unsigned char buf[512];
	unsigned char sense[128];
	int cdb_len;
	unsigned buf_read = 0;
	unsigned sense_read = 0;
	int ret;
	io_result_t io_res;

	*is_ata = false;
	*ata_buf_len = 0;
	memset(buf, 0, sizeof(buf));

	cdb_len = cdb_inquiry_simple(cdb, 96);
	ret = sg_ioctl(dev->fd, cdb, cdb_len, buf, sizeof(buf), SG_DXFER_FROM_DEV, SHORT_TIMEOUT, sense, sizeof(sense), &buf_read, &sense_read, &io_res);
	if (ret < 0)
		return -1;

	int device_type;
	if (!parse_inquiry(buf, buf_read, &device_type, vendor, model, fw_rev, serial))
	{
		INFO("Failed to parse the inquiry data");
		return -1;
	}
	strtrim(vendor);
	strtrim(model);
	strtrim(fw_rev);
	strtrim(serial);

	// If the vendor doesn't start with ATA it is a proper SCSI interface
	if (strncmp(vendor, "ATA", 3) != 0)
		return 0;

	*is_ata = true;

	// For an ATA disk we need to get the proper ATA IDENTIFY response
	memset(buf, 0, sizeof(buf));
	cdb_len = cdb_ata_identify(cdb);
	ret = sg_ioctl(dev->fd, cdb, cdb_len, buf, sizeof(buf), SG_DXFER_FROM_DEV, SHORT_TIMEOUT, sense, sizeof(sense), &buf_read, &sense_read, &io_res);
	if (ret < 0)
		return -1;

	ata_get_ata_identify_model(buf, vendor);
	strtrim(vendor);
	strcpy(model, vendor + strlen(vendor) + 1);
	strtrim(model);
	ata_get_ata_identify_fw_rev(buf, fw_rev);
	strtrim(fw_rev);
	ata_get_ata_identify_serial_number(buf, serial);
	strtrim(serial);

	memcpy(ata_buf, buf, buf_read);
	*ata_buf_len = buf_read;

	return 0;
}

void mac_read(unsigned char *buf, int len)
{
	struct ifreq ifr;
	struct ifconf ifc;
	char data[1024];
	int success = 0;

	buf[0] = 0;

	int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (sock == -1) {
		return;
	};

	ifc.ifc_len = sizeof(data);
	ifc.ifc_buf = data;
	if (ioctl(sock, SIOCGIFCONF, &ifc) == -1) {
		/* handle error */
		goto Exit;
	}

	struct ifreq* it = ifc.ifc_req;
	const struct ifreq* const end = it + (ifc.ifc_len / sizeof(struct ifreq));

	for (; it != end; ++it) {
		strcpy(ifr.ifr_name, it->ifr_name);
		if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0) {
			if (! (ifr.ifr_flags & IFF_LOOPBACK)) { // don't count loopback
				if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
					success = 1;
					break;
				}
			}
		}
		else { /* handle error */ }
	}

	if (success) {
		memcpy(buf, ifr.ifr_hwaddr.sa_data, len >= 6 ? 6 : len);
	} else {
		memset(buf, 0, len);
	}

Exit:
	close(sock);
}
