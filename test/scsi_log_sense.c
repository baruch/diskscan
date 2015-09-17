/* Copyright 2014 Baruch Even
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

#include "scsicmd.h"
#include "main.h"
#include <stdio.h>
#include <memory.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <scsi/sg.h>
#include <inttypes.h>

static inline uint16_t get_uint16(unsigned char *buf, int start)
{
	return (uint16_t)buf[start] << 8 |
		   (uint16_t)buf[start+1];
}

static void dump_page(int fd, uint8_t page, uint8_t subpage)
{
	unsigned char cdb[32];
	unsigned char buf[16*1024];
	unsigned cdb_len = cdb_log_sense(cdb, page, subpage, sizeof(buf));

	printf("List page %02X subpage %02X\n", page, subpage);
	bool ret = submit_cmd(fd, cdb, cdb_len, buf, sizeof(buf), SG_DXFER_FROM_DEV);
	if (!ret) {
		fprintf(stderr, "Failed to submit command\n");
		return;
	}

	unsigned char *sense = NULL;
	unsigned sense_len = 0;
	unsigned buf_len = 0;
	ret = read_response_buf(fd, &sense, &sense_len, &buf_len);

	if (sense) {
		printf("error while reading response buffer, nothing to show\n");
		return;
	}

	printf("Read %u bytes\n", buf_len);
	response_dump(buf, buf_len);
	printf("\n");
}

void do_command(int fd)
{
	unsigned char cdb[32];
	unsigned char buf[16*1024];
	unsigned cdb_len = cdb_log_sense(cdb, 0, 0, sizeof(buf));

	printf("List all supported pages\n");
	bool ret = submit_cmd(fd, cdb, cdb_len, buf, sizeof(buf), SG_DXFER_FROM_DEV);
	if (!ret) {
		fprintf(stderr, "Failed to submit command\n");
		return;
	}

	unsigned char *sense = NULL;
	unsigned sense_len = 0;
	unsigned buf_len = 0;
	ret = read_response_buf(fd, &sense, &sense_len, &buf_len);

	if (sense) {
		printf("error while reading response buffer, nothing to show\n");
		return;
	}

	printf("Read %u bytes\n", buf_len);
	response_dump(buf, buf_len);

	if (buf_len < 4) {
		printf("log sense list must have at least 4 bytes\n");
		return;
	}

	if (buf[0] != 0 || buf[1] != 0) {
		printf("expected to receive log page 0 subpage 0\n");
		return;
	}

	uint16_t num_pages = get_uint16(buf, 2);
	uint16_t i;
	for (i = 0; i < num_pages; i++) {
		dump_page(fd, buf[4 + i], 0);
	}

	printf("List all pages and subpages\n");
	cdb_len = cdb_log_sense(cdb, 0, 0xff, sizeof(buf));

	ret = submit_cmd(fd, cdb, cdb_len, buf, sizeof(buf), SG_DXFER_FROM_DEV);
	if (!ret) {
		fprintf(stderr, "Failed to submit command\n");
		return;
	}

	sense = NULL;
	sense_len = 0;
	buf_len = 0;
	ret = read_response_buf(fd, &sense, &sense_len, &buf_len);

	if (sense) {
		printf("error while reading response buffer, nothing to show\n");
		return;
	}

	printf("Read %u bytes\n", buf_len);

	response_dump(buf, buf_len);

	if (buf_len < 4) {
		printf("log sense list must have at least 4 bytes\n");
		return;
	}

	if (buf[0] != 0x40 || buf[1] != 0xFF) {
		printf("expected to receive log page 0 (spf=1) subpage 0xFF\n");
		return;
	}

	num_pages = get_uint16(buf, 2);
	for (i = 0; i < num_pages; i++) {
		uint8_t page = buf[4 + i*2] & 0x3F;
		uint8_t subpage = buf[4 + i*2 + 1];
		if (subpage == 0) {
			printf("Skipping page %02X subpage %02X since subpage is 00 it was already retrieved above\n", page, subpage);
			continue;
		}
		dump_page(fd, page, subpage);
	}
} 
