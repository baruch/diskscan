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
#include "main.h"
#include "sense_dump.h"
#include <stdio.h>
#include <memory.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <scsi/sg.h>

static unsigned char sense[128];

bool submit_cmd(int fd, unsigned char *cdb, unsigned cdb_len, unsigned char *buf, unsigned buf_len, int dxfer_dir)
{
	sg_io_hdr_t hdr;

	memset(&hdr, 0, sizeof(hdr));

	hdr.interface_id = 'S';
	hdr.dxfer_direction = dxfer_dir;
	hdr.cmd_len = cdb_len;
	hdr.mx_sb_len = sizeof(sense);
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

bool read_response_buf(int fd, unsigned char **sensep, unsigned *sense_len, unsigned *buf_read)
{
	*sensep = NULL;
	*sense_len = 0;

	sg_io_hdr_t hdr;
	int ret = read(fd, &hdr, sizeof(hdr));
	if (ret != sizeof(hdr)) {
		fprintf(stderr, "Error reading scsi response, ret=%d, expected=%d, %m\n", ret, (int)sizeof(hdr));
		return false;
	}

	printf("status: %d\n", hdr.status);
	printf("masked status: %d\n", hdr.masked_status);
	printf("driver status: %d\n", hdr.driver_status);
	printf("msg status: %d\n", hdr.msg_status);
	printf("host status: %d\n", hdr.host_status);
	printf("sense len: %d\n", hdr.sb_len_wr);

	if (hdr.sb_len_wr) {
		*sensep = sense;
		*sense_len = hdr.sb_len_wr;

		printf("sense data:\n");
                sense_dump(sense, hdr.sb_len_wr);
	}
	if (buf_read)
		*buf_read = hdr.dxfer_len - hdr.resid;
	return true;
}

static void test(const char *devname)
{
	int fd = open(devname, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "Error opening device '%s': %m\n", devname);
		return;
	}

	do_command(fd);

	close(fd);
}

static int usage(char *name)
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "\t%s disk_device\n", name);
	return 1;
}

int main(int argc, char **argv)
{
	if (argc != 2 || strstr(argv[1], "/sd") != NULL)
		return usage(argv[0]);

	test(argv[1]);
	return 0;
}
