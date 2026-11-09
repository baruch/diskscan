/* Exercise the real SMART command, Linux SG_IO and monitor paths without disks. */
#include <assert.h>
#include <errno.h>
#include <scsi/sg.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "lib/diskscan.c"

enum response_mode { GOOD, ABORTED, SHORT_DATA, BAD_CHECKSUM, EMPTY, ATA_ERROR, IOCTL_ERROR };
static enum response_mode mode;
static unsigned reads, health_checks;
static bool tripped;
static char messages[8192];
static size_t messages_len;
int verbose, verbose_extra_newline;

void verbose_out(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int n = vsnprintf(messages + messages_len, sizeof(messages) - messages_len, fmt, ap);
	va_end(ap);
	assert(n >= 0 && (size_t)n + messages_len + 1 < sizeof(messages));
	messages_len += n;
	messages[messages_len++] = '\n';
	messages[messages_len] = 0;
}

static void ata_descriptor(sg_io_hdr_t *hdr, unsigned char status)
{
	unsigned char *sense = hdr->sbp;
	memset(sense, 0, hdr->mx_sb_len);
	sense[0] = 0x72;
	sense[1] = SENSE_KEY_RECOVERED_ERROR;
	sense[3] = 0x1d;
	sense[7] = 14;
	sense[8] = 9;
	sense[9] = 12;
	sense[11] = status & 1 ? 4 : 0;
	sense[17] = tripped ? 0xf4 : 0x4f;
	sense[19] = tripped ? 0x2c : 0xc2;
	sense[21] = status;
	hdr->status = 2;
	hdr->driver_status = 8;
	hdr->sb_len_wr = 22;
}

int __wrap_ioctl(int fd, unsigned long request, ...)
{
	va_list ap;
	assert(fd == 7 && request == SG_IO);
	va_start(ap, request);
	sg_io_hdr_t *hdr = va_arg(ap, sg_io_hdr_t *);
	va_end(ap);
	assert(hdr->cmd_len == 12 && hdr->cmdp[0] == 0xa1 && hdr->cmdp[9] == 0xb0);
	assert(hdr->cmdp[6] == 0x4f && hdr->cmdp[7] == 0xc2);
	if (hdr->cmdp[3] == 0xda) {
		health_checks++;
		assert(hdr->cmdp[1] == 6 && hdr->cmdp[2] == 0x20 && !hdr->cmdp[4]);
		assert(!hdr->dxfer_len && !hdr->dxferp);
		ata_descriptor(hdr, 0x50);
		return 0;
	}
	assert(hdr->cmdp[3] == 0xd0 && hdr->cmdp[1] == 8 && hdr->cmdp[2] == 0x0e);
	assert(hdr->cmdp[4] == 1 && hdr->dxfer_len == 512);
	reads++;
	if (mode == IOCTL_ERROR) { errno = EOPNOTSUPP; return -1; }
	unsigned char *data = hdr->dxferp;
	memset(data, 0, 512);
	if (mode == ABORTED) {
		memset(hdr->sbp, 0, hdr->mx_sb_len);
		hdr->sbp[0] = 0x70;
		hdr->sbp[2] = SENSE_KEY_ABORTED_COMMAND;
		hdr->sbp[7] = 10;
		hdr->sb_len_wr = 18;
		hdr->status = 2;
		hdr->driver_status = 8;
		/* Like libata, report a full transfer despite the failed command. */
		return 0;
	}
	if (mode == SHORT_DATA) { hdr->resid = 256; return 0; }
	if (mode == EMPTY) return 0;
	data[0] = 0x10;
	data[2] = 5;
	data[5] = data[6] = 100;
	data[14] = 194;
	data[17] = data[18] = 100;
	data[19] = 30;
	data[26] = 197;
	data[29] = data[30] = 100;
	data[511] = ata_calc_checksum(data);
	if (mode == BAD_CHECKSUM) data[511] ^= 1;
	if (mode == ATA_ERROR) ata_descriptor(hdr, 0x51);
	return 0;
}

static void reset(disk_t *disk, enum response_mode next)
{
	memset(disk, 0, sizeof(*disk));
	disk->dev.fd = 7;
	mode = next;
	reads = health_checks = 0;
	tripped = false;
	messages[0] = 0;
	messages_len = 0;
}

static void test_success(void)
{
	disk_t disk;
	reset(&disk, GOOD);
	disk_ata_monitor_start(&disk);
	assert(disk.state.ata.smart_num == 3 && disk.state.ata.last_temp == 30);
	disk_ata_monitor(&disk);
	disk_ata_monitor(&disk);
	disk_ata_monitor_end(&disk);
	assert(reads == 4 && health_checks == 4);
	assert(disk.state.ata.smart_attributes_available);
	assert(!strstr(messages, "E: ") && !strstr(messages, "disabled"));
	tripped = true;
	assert(disk_smart_trip(&disk.dev) == 1);
	unsigned char cdb[12];
	cdb_ata_smart_read_threshold(cdb);
	assert(cdb[1] == 8 && cdb[2] == 0x0e && cdb[3] == 0xd1 && cdb[4] == 1);
}

static void test_failure(enum response_mode failure, const char *reason)
{
	disk_t disk;
	reset(&disk, failure);
	disk_ata_monitor_start(&disk);
	disk_ata_monitor(&disk);
	disk_ata_monitor(&disk);
	disk_ata_monitor_end(&disk);
	assert(reads == 1 && health_checks == 4);
	assert(!disk.state.ata.smart_attributes_available);
	assert(strstr(messages, reason));
	const char *notice = strstr(messages, "SMART attribute monitoring disabled");
	assert(notice && !strstr(notice + 1, "SMART attribute monitoring disabled"));
}

int main(void)
{
	test_success();
	test_failure(ABORTED, "key=0x0b ASC/ASCQ=00/00");
	test_failure(SHORT_DATA, "short response: 256/512");
	test_failure(BAD_CHECKSUM, "invalid checksum");
	test_failure(EMPTY, "no attributes");
	test_failure(ATA_ERROR, "ATA status=0x51 error=0x04");
	test_failure(IOCTL_ERROR, "errno=");
	disk_t disk;
	reset(&disk, GOOD);
	disk_ata_monitor_start(&disk);
	mode = BAD_CHECKSUM;
	disk_ata_monitor(&disk);
	disk_ata_monitor(&disk);
	disk_ata_monitor_end(&disk);
	assert(reads == 2 && !disk.state.ata.smart_attributes_available);
	test_success(); /* Failure state must not leak into another device/scan. */
	puts("SMART protocols, command errors, checksums and one-time failure reporting passed");
	return 0;
}
