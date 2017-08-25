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
#include "sense_dump.h"
#include "parse_mode_sense.h"
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
	unsigned char buf[4096];
	unsigned cdb_len = cdb_mode_sense_10(cdb, true, false, PAGE_CONTROL_CURRENT, 0x3F, 0xFF, sizeof(buf));

	memset(buf, 0, sizeof(buf));
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

	if (buf_len < MODE_SENSE_10_MIN_LEN) {
		printf("Returned data is too short, expected a minimum of %u bytes and got only %u\n", MODE_SENSE_10_MIN_LEN, buf_len);
		return;
	}

	printf("Mode data len: %u\n", mode_sense_10_data_len(buf));
	printf("Medium Type: %u\n", mode_sense_10_medium_type(buf));
	printf("Device specific param: %u\n", mode_sense_10_device_specific_param(buf));
	printf("Long LBA: %s\n", mode_sense_10_long_lba(buf) ? "yes" : "no");
	printf("Block Descriptor length: %u\n", mode_sense_10_block_descriptor_length(buf));

	if (buf_len < MODE_SENSE_10_MIN_LEN + mode_sense_10_block_descriptor_length(buf))
	{
		printf("Not enough data for the block descriptor length\n");
		return;
	}

	if (mode_sense_10_long_lba(buf)) {
		printf("Don't know how to parse the block descriptor for a long lba yet\n");
	} else {
		uint8_t *bdb = mode_sense_10_block_descriptor_data(buf);

		putchar('\n');
		printf("Density code: %u\n", block_descriptor_density_code(bdb));
		printf("Num blocks: %u\n", block_descriptor_num_blocks(bdb));
		printf("Block length: %u\n", block_descriptor_block_length(bdb));
	}

	if (buf_len < MODE_SENSE_10_MIN_LEN + mode_sense_10_block_descriptor_length(buf) + mode_sense_10_data_len(buf))
	{
		printf("Not enough data for the mode data length\n");
		return;
	}

	putchar('\n');
	//uint8_t *mode_data = mode_sense_10_mode_data(buf);

} 
