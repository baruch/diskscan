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
#include "sense_dump.h"
#include "parse_extended_inquiry.h"
#include <stdio.h>
#include <memory.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <scsi/sg.h>
#include <inttypes.h>
#include <ctype.h>

static void dump_evpd_ascii(unsigned char *data, uint16_t data_len)
{
	const uint16_t ascii_len = (data[0] << 8) | data[1];
	bool not_in_string = true;
	uint16_t i;

	printf("ASCII len: %u\n", ascii_len);
	printf("ASCII data: \"");
	for (i = 2; i < ascii_len + 2; i++)
		putchar(data[i]);
	printf("\"\n");

	for (; i < data_len; i++) {
		if (isprint(data[i])) {
			if (not_in_string) {
				printf("Vendor string: \"");
				not_in_string = false;
			}
			putchar(data[i]);
		} else {
			if (!not_in_string) {
				printf("\"\n");
				not_in_string = true;
			}
		}
	}
	if (!not_in_string)
		printf("\"\n");
}

static void dump_evpd(int fd, uint8_t page)
{
	unsigned char cdb[32];
	unsigned char buf[512] ;
	unsigned cdb_len = cdb_inquiry(cdb, true, page, sizeof(buf));

	printf("\n\nExtended Inquiry Page %u (%02x):\n", page, page);
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

	if (buf_len < EVPD_MIN_LEN) {
		printf("Buffer returned is too small, got %d expected minimum of %d", buf_len, EVPD_MIN_LEN);
		return;
	}

	if (page >= 0x01 && page <= 0x7F) {
		dump_evpd_ascii(evpd_page_data(buf), evpd_page_len(buf));
	}
}

static void do_extended_inquiry(int fd)
{
	unsigned char cdb[32];
	unsigned char buf[512] ;
	unsigned cdb_len = cdb_inquiry(cdb, true, 0, sizeof(buf));

	printf("\n\nExtended Inquiry:\n");
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

	if (buf_len < EVPD_MIN_LEN) {
		printf("Buffer returned is too small, got %d expected minimum of %d", buf_len, EVPD_MIN_LEN);
		return;
	}

	printf("Peripheral Qualifier: %u\n", evpd_peripheral_qualifier(buf));
	printf("Peripheral Device Type: %u\n", evpd_peripheral_device_type(buf));

	uint16_t max_page_idx = evpd_page_len(buf) + 4;
	uint16_t i;
	for (i = 4; i < max_page_idx; i++)
		dump_evpd(fd, buf[i]);
}

static void do_simple_inquiry(int fd)
{
	unsigned char cdb[32];
	unsigned char buf[512] ;
	unsigned cdb_len = cdb_inquiry_simple(cdb, 96);

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

	int device_type;
	scsi_vendor_t vendor;
	scsi_model_t model;
	scsi_fw_revision_t rev;
	scsi_serial_t serial;
	bool parsed = parse_inquiry(buf, buf_len, &device_type, vendor, model, rev, serial);
	if (!parsed) {
		printf("Failed to parse INQUIRY buffer\n");
		return;
	}

	printf("Device Type: %d - %s\n", device_type, scsi_device_type_name(device_type));
	printf("Vendor: %s\n", vendor);
	printf("Model: %s\n", model);
	printf("Rev: %s\n", rev);
	printf("Serial: %s\n", serial);
} 

void do_command(int fd)
{
	do_simple_inquiry(fd);
	do_extended_inquiry(fd);
}
