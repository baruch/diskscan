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
#include "parse_receive_diagnostics.h"
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
#include <inttypes.h>

static void dump_page(int fd, uint8_t page)
{
	unsigned char cdb[32];
	unsigned char buf[16*1024];
	unsigned cdb_len = cdb_receive_diagnostics(cdb, true, page, sizeof(buf));

	printf("List page %02X\n", page);
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
	unsigned cdb_len = cdb_receive_diagnostics(cdb, true, 0, sizeof(buf));

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

	if (buf_len < RECV_DIAG_MIN_LEN) {
		printf("receive diagnostics list must have at least 4 bytes\n");
		return;
	}

	if (recv_diag_get_page_code(buf) != 0) {
		printf("expected to receive receive diagnostics page 0\n");
		return;
	}

	uint16_t num_pages = recv_diag_get_len(buf);
	uint16_t i;
	for (i = 0; i < num_pages; i++) {
		dump_page(fd, buf[4 + i]);
	}
} 
