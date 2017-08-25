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
	unsigned cdb_len = cdb_ata_identify(cdb);

	bool ret = submit_cmd(fd, cdb, cdb_len, buf, sizeof(buf), SG_DXFER_FROM_DEV);
	if (!ret) {
		fprintf(stderr, "Failed to submit command\n");
		return;
	}

	unsigned char *sense = NULL;
	unsigned sense_len = 0;
	ret = read_response(fd, &sense, &sense_len);

	ata_status_t status;
	if (sense && ata_status_from_scsi_sense(sense, sense_len, &status)) {
		printf("extend: %d\n", status.extend);
		printf("error: %02x\n", status.error);
		printf("lba: %08"PRIx64"\n", status.lba);
		printf("device: %02x\n", status.device);
		printf("status: %02x\n", status.status);
	}

	if (!sense)
		dump_ata_identify(buf);
	else
		printf("error while reading ATA IDENTIFY, nothing to show\n");
}
