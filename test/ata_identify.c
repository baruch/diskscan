/* Copyright 2013 Baruch Even
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "ata.h"
#include "ata_parse.h"
#include "ata_identify_dump.h"
#include <stdio.h>
#include <memory.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <scsi/sg.h>

static bool submit_cmd(int fd, unsigned char *cdb, unsigned cdb_len, unsigned char *sense, unsigned sense_len, unsigned char *buf, unsigned buf_len, int dxfer_dir)
{
	sg_io_hdr_t hdr;

	memset(&hdr, 0, sizeof(hdr));

	hdr.interface_id = 'S';
	hdr.dxfer_direction = dxfer_dir;
	hdr.cmd_len = cdb_len;
	hdr.mx_sb_len = sense_len;
	hdr.dxfer_len = buf_len;
	hdr.dxferp = buf;
	hdr.cmdp = cdb;
	hdr.sbp = sense;
	hdr.timeout = 30*1000;
	hdr.flags = SG_FLAG_LUN_INHIBIT;
	hdr.pack_id = 0;
	hdr.usr_ptr = 0;

	printf("sbp: %p\n", hdr.sbp);

	ssize_t ret = write(fd, &hdr, sizeof(hdr));
	return ret == sizeof(hdr);
}

static void ata_identify(int fd)
{
	sg_io_hdr_t hdr;
	unsigned char sense[64];
	unsigned char cdb[32];
	unsigned char buf[512] ;
	int cdb_len = cdb_ata_identify(cdb);

	int j;
	for (j = 0 ; j < cdb_len; j++)
		printf("%02X ", cdb[j]);
		printf("\n");
	memset(sense, 0, sizeof(sense));

	bool ret = submit_cmd(fd, cdb, cdb_len, sense, sizeof(sense), buf, sizeof(buf), SG_DXFER_FROM_DEV);
	if (!ret) {
		fprintf(stderr, "Failed to submit command\n");
		return;
	}

	int retval = read(fd, &hdr, sizeof(hdr));
	if (retval != sizeof(hdr)) {
		fprintf(stderr, "Error reading from fd, retval=%d errno=%d (%m)\n", retval, errno);
		return;
	}

	printf("status: %d\n", hdr.status);
	printf("masked status: %d\n", hdr.masked_status);
	printf("driver status: %d\n", hdr.driver_status);
	printf("msg status: %d\n", hdr.msg_status);
	printf("host status: %d\n", hdr.host_status);
	printf("sense len: %d\n", hdr.sb_len_wr);

	int i;
	if (hdr.sb_len_wr) {
		printf("sense data:\n");
		for (i = 0; i < hdr.sb_len_wr; i++) {
			if (i % 16 == 0)
				printf("\n%02x  ", i);
			printf("%02x ", sense[i]);
		}
		printf("\n");
	}

	ata_status_t status;
	if (hdr.status && ata_status_from_scsi_sense(sense, hdr.sb_len_wr, &status)) {
		printf("extend: %d\n", status.extend);
		printf("error: %02x\n", status.error);
		printf("lba: %08llx\n", status.lba);
		printf("device: %02x\n", status.device);
		printf("status: %02x\n", status.status);
	}

	if (hdr.status == 0)
		dump_ata_identify((char *)buf);
	else
		printf("error while reading ATA IDENTIFY, nothing to show\n");
}

static void test(const char *devname)
{
	int fd = open(devname, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "Error opening device '%s': %m\n", devname);
		return;
	}

	ata_identify(fd);

	close(fd);
}

static int usage(int argc, char *name)
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "\t%s disk_device\n", name);
	return 1;
}

int main(int argc, char **argv)
{
	if (argc != 2 || strstr(argv[1], "/sd") != NULL)
		return usage(argc, argv[0]);

	test(argv[1]);
	return 0;
}
