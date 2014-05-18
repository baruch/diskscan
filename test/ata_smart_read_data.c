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
#include <stdio.h>
#include <scsi/sg.h>
#include <inttypes.h>

void do_command(int fd)
{
	unsigned char cdb[12];
	unsigned char buf[512] ;
	int cdb_len = cdb_ata_smart_read_data(cdb);

	bool ret = submit_cmd(fd, cdb, cdb_len, buf, sizeof(buf), SG_DXFER_FROM_DEV);
	if (!ret) {
		fprintf(stderr, "Failed to submit command\n");
		return;
	}

	unsigned char *sense;
	unsigned sense_len;
	ret = read_response(fd, &sense, &sense_len);
	if (!ret) {
		fprintf(stderr, "Error reading scsi response\n");
		return;
	}

/*
	ata_status_t status;
	if (ata_status_from_scsi_sense(sense, sense_len, &status)) {
		printf("extend: %d\n", status.extend);
		printf("error: %02x\n", status.error);
		printf("lba: %08llx\n", status.lba);
		printf("device: %02x\n", status.device);
		printf("status: %02x\n", status.status);
	}
*/
	
	printf("Response Dump:\n");
	response_dump(buf, sizeof(buf));


	printf("Page checksum read: %02X\n", ata_get_ata_smart_read_data_checksum(buf));
	printf("Page checksum calc: %02X\n", ata_calc_ata_smart_read_data_checksum(buf));
	printf("Page checksum matches: %s\n", ata_check_ata_smart_read_data_checksum(buf) ? "true" : "false");
	printf("Page version: %04Xh\n", ata_get_ata_smart_read_data_version(buf));
	if (ata_get_ata_smart_read_data_version(buf) != 0x10) {
		printf("Unknown page version, only known version is 10h\n");
		return;
	}

	ata_smart_attr_t attrs[MAX_SMART_ATTRS];
	int num_attrs = ata_parse_ata_smart_read_data(buf, attrs, MAX_SMART_ATTRS);
	int i;
	for (i = 0; i < num_attrs; i++) {
		const ata_smart_attr_t *attr = &attrs[i];
		printf("Attribute #%2d: id %3u status %04X val %3u min %3u raw %"PRIu64"\n", i, attr->id, attr->status, attr->value, attr->min, attr->raw);
	}
}
