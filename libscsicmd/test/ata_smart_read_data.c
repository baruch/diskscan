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
#include "ata_smart.h"
#include "sense_dump.h"
#include "main.h"
#include "smartdb.h"
#include <stdio.h>
#include <scsi/sg.h>
#include <inttypes.h>

static bool read_data(int fd, unsigned char *buf, int buf_len)
{
	unsigned char cdb[12];

	int cdb_len = cdb_ata_smart_read_data(cdb);

	bool ret = submit_cmd(fd, cdb, cdb_len, buf, buf_len, SG_DXFER_FROM_DEV);
	if (!ret) {
		fprintf(stderr, "Failed to submit command\n");
		return false;
	}

	unsigned char *sense;
	unsigned sense_len;
	ret = read_response(fd, &sense, &sense_len);
	if (!ret) {
		fprintf(stderr, "Error reading scsi response\n");
		return false;
	}

	printf("Response Dump:\n");
	response_dump(buf, sizeof(buf));

	printf("Page checksum read: %02X\n", ata_get_ata_smart_read_data_checksum(buf));
	printf("Page checksum calc: %02X\n", ata_calc_checksum(buf));
	printf("Page checksum matches: %s\n", ata_check_ata_smart_read_data_checksum(buf) ? "true" : "false");
	printf("Page version: %04Xh\n", ata_get_ata_smart_read_data_version(buf));
	if (ata_get_ata_smart_read_data_version(buf) != 0x10) {
		printf("Unknown page version, only known version is 10h\n");
		return false;
	}

	return true;
}

static bool read_threshold(int fd, unsigned char *buf, int buf_len)
{
	unsigned char cdb[12];

	int cdb_len = cdb_ata_smart_read_threshold(cdb);

	bool ret = submit_cmd(fd, cdb, cdb_len, buf, buf_len, SG_DXFER_FROM_DEV);
	if (!ret) {
		fprintf(stderr, "Failed to submit command\n");
		return false;
	}

	unsigned char *sense;
	unsigned sense_len;
	ret = read_response(fd, &sense, &sense_len);
	if (!ret) {
		fprintf(stderr, "Error reading scsi response\n");
		return false;
	}

	printf("Response Dump:\n");
	response_dump(buf, sizeof(buf));

	printf("Page checksum read: %02X\n", ata_get_ata_smart_read_data_checksum(buf));
	printf("Page checksum calc: %02X\n", ata_calc_checksum(buf));
	printf("Page checksum matches: %s\n", ata_check_ata_smart_read_data_checksum(buf) ? "true" : "false");
	printf("Page version: %04Xh\n", ata_get_ata_smart_read_data_version(buf));
	if (ata_get_ata_smart_read_data_version(buf) != 0x10) {
		printf("Unknown page version, only known version is 10h\n");
		return false;
	}
	return true;
}

void do_command(int fd)
{
	unsigned char buf_thresh[512];
	unsigned char buf_data[512];

	if (!read_data(fd, buf_data, sizeof(buf_data)))
		return;

	if (!read_threshold(fd, buf_thresh, sizeof(buf_thresh)))
		return;

	const smart_table_t *table = smart_table_for_disk(NULL, NULL, NULL);

	ata_smart_attr_t attrs[MAX_SMART_ATTRS];
	int num_attrs1 = ata_parse_ata_smart_read_data(buf_data, attrs, MAX_SMART_ATTRS);

	ata_smart_thresh_t thresholds[MAX_SMART_ATTRS];
	int num_attrs2 = ata_parse_ata_smart_read_thresh(buf_thresh, thresholds, MAX_SMART_ATTRS);


	if (num_attrs1 != num_attrs2) {
		printf("Number of attributes in data (%d) and thresholds (%d) do not match\n", num_attrs1, num_attrs2);
		return;
	}

	printf("num attributes %d\n", num_attrs1);
	int i;
	for (i = 0; i < num_attrs1; i++) {
		const ata_smart_attr_t *attr = &attrs[i];
		const ata_smart_thresh_t *thresh = &thresholds[i];
		const smart_attr_t *attr_type = smart_attr_for_id(table, attr->id);
		printf("Attribute #%2d: id %3u %-40s status %04X val %3u min %3u thresh %3u raw %"PRIu64"\n", i, attr->id, attr_type ? attr_type->name : "Unknown", attr->status, attr->value, attr->min, thresh->threshold, attr->raw);
	}


	printf("\nKey attributes:\n");
	{
		int min_temp, max_temp, cur_temp;
		cur_temp = ata_smart_get_temperature(attrs, num_attrs1, table, &min_temp, &max_temp);
		printf("  Temperature: %d (min=%d max=%d)\n", cur_temp, min_temp, max_temp);
	}
	{
		int minutes = -1;
		int hours;
		hours = ata_smart_get_power_on_hours(attrs, num_attrs1, table, &minutes);
		printf("  POH: %d (minutes: %d)\n", hours, minutes);
	}
	printf("  # Reallocations: %d\n", ata_smart_get_num_reallocations(attrs, num_attrs1, table));
	printf("  # Pending Reallocations: %d\n", ata_smart_get_num_pending_reallocations(attrs, num_attrs1, table));
	printf("  # CRC Errors: %d\n", ata_smart_get_num_crc_errors(attrs, num_attrs1, table));
}
