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

void do_command(int fd)
{
	unsigned char cdb[32];
	unsigned char buf[512] ;
	unsigned cdb_len = cdb_read_capacity_10(cdb);

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

	uint32_t max_lba;
	uint32_t block_size;
	bool parsed = parse_read_capacity_10(buf, buf_len, &max_lba, &block_size);
	if (!parsed) {
		printf("Failed to parse read capacity buffer\n");
		return;
	}

	printf("Max LBA: %u\nBlock size: %u\n", max_lba, block_size);
	return;
} 
