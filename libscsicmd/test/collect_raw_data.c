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
#include "ata.h"

#include "main.h"
#include "sense_dump.h"
#include "scsicmd_utils.h"
#include "parse_extended_inquiry.h"
#include "parse_receive_diagnostics.h"
#include "parse_log_sense.h"
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

static bool is_ata;

static void hex_dump(uint8_t *data, uint16_t len)
{
	uint16_t i;

	if (data == NULL || len == 0)
		return;

	printf("%02x", data[0]);
	for (i = 1; i < len; i++) {
		printf(" %02x", data[i]);
	}
}

static void emit_data_csv(uint8_t *cdb, uint8_t cdb_len, uint8_t *sense, uint8_t sense_len, uint8_t *buf, uint16_t buf_len)
{
	putchar(',');
	hex_dump(cdb, cdb_len);
	putchar(',');
	hex_dump(sense, sense_len);
	putchar(',');
	hex_dump(buf, buf_len);
	putchar('\n');
}

static int simple_command(int fd, uint8_t *cdb, unsigned cdb_len, uint8_t *buf, unsigned buf_len)
{
	memset(buf, 0, buf_len);

	bool ret = submit_cmd(fd, cdb, cdb_len, buf, buf_len, buf_len ? SG_DXFER_FROM_DEV : SG_DXFER_NONE);
	if (!ret) {
		printf("Failed to submit command,\n");
		return -1;
	}

	unsigned char *sense = NULL;
	unsigned sense_len = 0;
	ret = read_response_buf(fd, &sense, &sense_len, &buf_len);

	emit_data_csv(cdb, cdb_len, sense, sense_len, buf, buf_len);

	if (sense_len > 0) {
		sense_info_t sense_info;
		bool sense_parsed = scsi_parse_sense(sense, sense_len, &sense_info);
		if (sense_parsed && sense_info.sense_key == SENSE_KEY_RECOVERED_ERROR)
			return buf_len;
		return -1;
	}
	return buf_len;
}

static void do_simple_inquiry(int fd)
{
	unsigned char cdb[32];
	unsigned char buf[128];
	unsigned cdb_len = cdb_inquiry_simple(cdb, 96);
	int buf_len;

	buf_len = simple_command(fd, cdb, cdb_len, buf, sizeof(buf));

	int device_type;
	scsi_vendor_t vendor;
	scsi_model_t model;
	scsi_fw_revision_t rev;
	scsi_serial_t serial;
	if (parse_inquiry(buf, buf_len, &device_type, vendor, model, rev, serial) &&
		strncmp(vendor, "ATA", 3) == 0)
	{
		is_ata = true;
	}
} 

static void dump_evpd(int fd, uint8_t evpd_page)
{
	unsigned char cdb[32];
	unsigned char buf[512];
	unsigned cdb_len = cdb_inquiry(cdb, true, evpd_page, sizeof(buf));

	simple_command(fd, cdb, cdb_len, buf, sizeof(buf));
}

static void do_extended_inquiry(int fd)
{
	unsigned char cdb[32];
	unsigned char buf[512];
	unsigned cdb_len = cdb_inquiry(cdb, true, 0, sizeof(buf));
	int buf_len;

	buf_len = simple_command(fd, cdb, cdb_len, buf, sizeof(buf));

	if (buf_len > 0 && evpd_is_valid(buf, buf_len)) {
		uint16_t max_page_idx = evpd_page_len(buf) + 4;
		uint16_t i;
		for (i = 4; i < max_page_idx; i++)
			dump_evpd(fd, buf[i]);
	}
}

static void dump_log_sense(int fd, uint8_t page, uint8_t subpage)
{
	unsigned char cdb[32];
	unsigned char buf[16*1024];
	unsigned cdb_len = cdb_log_sense(cdb, page, subpage, sizeof(buf));

	simple_command(fd, cdb, cdb_len, buf, sizeof(buf));
}

static void do_log_sense(int fd)
{
	unsigned char cdb[32];
	unsigned char buf[16*1024];
	unsigned cdb_len = cdb_log_sense(cdb, 0, 0, sizeof(buf));
	int buf_len;

	buf_len = simple_command(fd, cdb, cdb_len, buf, sizeof(buf));

	if (buf_len < 0) {
		printf("error while reading log sense list, nothing to show\n");
		return;
	}

	if (!log_sense_is_valid(buf, buf_len)) {
		printf("log sense page is invalid\n");
		return;
	}

	if (buf[0] != 0 || buf[1] != 0) {
		printf("expected to receive log page 0 subpage 0\n");
		return;
	}

	uint16_t num_pages = get_uint16(buf, 2);
	uint16_t i;
	for (i = 0; i < num_pages; i++) {
		dump_log_sense(fd, buf[4 + i], 0);
	}

	cdb_len = cdb_log_sense(cdb, 0, 0xff, sizeof(buf));
	buf_len = simple_command(fd, cdb, cdb_len, buf, sizeof(buf));

	if (buf_len < 0) {
		printf("error while reading list of log subpages, nothing to show\n");
		return;
	}

	if (!log_sense_is_valid(buf, buf_len)) {
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
		dump_log_sense(fd, page, subpage);
	}
}

static void do_mode_sense_10_type(int fd, bool long_lba, bool disable_block_desc, page_control_e page_control)
{
	unsigned char cdb[32];
	unsigned char buf[4096];
	unsigned cdb_len = cdb_mode_sense_10(cdb, long_lba, disable_block_desc, page_control, 0x3F, 0xFF, sizeof(buf));

	simple_command(fd, cdb, cdb_len, buf, sizeof(buf));
}

static void do_mode_sense_10(int fd)
{
	do_mode_sense_10_type(fd, true, true, PAGE_CONTROL_CURRENT);
	do_mode_sense_10_type(fd, true, true, PAGE_CONTROL_CHANGEABLE);
	do_mode_sense_10_type(fd, true, true, PAGE_CONTROL_DEFAULT);
	do_mode_sense_10_type(fd, true, true, PAGE_CONTROL_SAVED);

	do_mode_sense_10_type(fd, false, true, PAGE_CONTROL_CURRENT);
	do_mode_sense_10_type(fd, false, true, PAGE_CONTROL_CHANGEABLE);
	do_mode_sense_10_type(fd, false, true, PAGE_CONTROL_DEFAULT);
	do_mode_sense_10_type(fd, false, true, PAGE_CONTROL_SAVED);

	do_mode_sense_10_type(fd, false, false, PAGE_CONTROL_CURRENT);
	do_mode_sense_10_type(fd, false, false, PAGE_CONTROL_CHANGEABLE);
	do_mode_sense_10_type(fd, false, false, PAGE_CONTROL_DEFAULT);
	do_mode_sense_10_type(fd, false, false, PAGE_CONTROL_SAVED);

	do_mode_sense_10_type(fd, true, false, PAGE_CONTROL_CURRENT);
	do_mode_sense_10_type(fd, true, false, PAGE_CONTROL_CHANGEABLE);
	do_mode_sense_10_type(fd, true, false, PAGE_CONTROL_DEFAULT);
	do_mode_sense_10_type(fd, true, false, PAGE_CONTROL_SAVED);
}

static void do_mode_sense_6_type(int fd, bool disable_block_desc, page_control_e page_control)
{
	unsigned char cdb[32];
	unsigned char buf[255];
	unsigned cdb_len = cdb_mode_sense_6(cdb, disable_block_desc, page_control, 0x3F, 0xFF, sizeof(buf));

	simple_command(fd, cdb, cdb_len, buf, sizeof(buf));
}

static void do_mode_sense_6(int fd)
{
	do_mode_sense_6_type(fd, true, PAGE_CONTROL_CURRENT);
	do_mode_sense_6_type(fd, true, PAGE_CONTROL_CHANGEABLE);
	do_mode_sense_6_type(fd, true, PAGE_CONTROL_DEFAULT);
	do_mode_sense_6_type(fd, true, PAGE_CONTROL_SAVED);

	do_mode_sense_6_type(fd, false, PAGE_CONTROL_CURRENT);
	do_mode_sense_6_type(fd, false, PAGE_CONTROL_CHANGEABLE);
	do_mode_sense_6_type(fd, false, PAGE_CONTROL_DEFAULT);
	do_mode_sense_6_type(fd, false, PAGE_CONTROL_SAVED);
}

static void do_mode_sense(int fd)
{
	do_mode_sense_10(fd);
	do_mode_sense_6(fd);
}

static void dump_rcv_diag_page(int fd, uint8_t page)
{
	unsigned char cdb[32];
	unsigned char buf[16*1024];
	unsigned cdb_len = cdb_receive_diagnostics(cdb, true, page, sizeof(buf));

	simple_command(fd, cdb, cdb_len, buf, sizeof(buf));
}

static void do_receive_diagnostic(int fd)
{
	unsigned char cdb[32];
	unsigned char buf[16*1024];
	unsigned cdb_len = cdb_receive_diagnostics(cdb, true, 0, sizeof(buf));
	int buf_len;

	buf_len = simple_command(fd, cdb, cdb_len, buf, sizeof(buf));

	if (buf_len < 0) {
		printf("error while reading response buffer, nothing to show\n");
		return;
	}

	if (recv_diag_is_valid(buf, buf_len)) {
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
		dump_rcv_diag_page(fd, buf[4 + i]);
	}
}

static void do_read_capacity_10(int fd)
{
	unsigned char cdb[32];
	unsigned char buf[8];
	unsigned cdb_len = cdb_read_capacity_10(cdb);

	simple_command(fd, cdb, cdb_len, buf, sizeof(buf));
}

static void do_read_capacity_16(int fd)
{
	unsigned char cdb[32];
	unsigned char buf[512];
	unsigned cdb_len = cdb_read_capacity_16(cdb, sizeof(buf));

	simple_command(fd, cdb, cdb_len, buf, sizeof(buf));
}

static void do_read_capacity(int fd)
{
	do_read_capacity_10(fd);
	do_read_capacity_16(fd);
}

static void do_read_defect_data_10(int fd, bool plist, bool glist, uint8_t format, bool count_only)
{
	unsigned char cdb[32];
	unsigned char buf[512];
	unsigned cdb_len = cdb_read_defect_data_10(cdb, plist, glist, format, count_only ? 8 : sizeof(buf));

	simple_command(fd, cdb, cdb_len, buf, sizeof(buf));
}

static void do_read_defect_data_10_all(int fd, uint8_t format)
{
	do_read_defect_data_10(fd, true, false, format, true);
	do_read_defect_data_10(fd, true, false, format, false);
	do_read_defect_data_10(fd, false, true, format, true);
	do_read_defect_data_10(fd, false, true, format, false);
}

static void do_read_defect_data_12(int fd, bool plist, bool glist, uint8_t format, bool count_only)
{
	unsigned char cdb[32];
	unsigned char buf[512];
	unsigned cdb_len = cdb_read_defect_data_12(cdb, plist, glist, format, count_only ? 8 : sizeof(buf));

	simple_command(fd, cdb, cdb_len, buf, sizeof(buf));
}

static void do_read_defect_data_12_all(int fd, uint8_t format)
{
	do_read_defect_data_12(fd, true, false, format, true);
	do_read_defect_data_12(fd, true, false, format, false);
	do_read_defect_data_12(fd, false, true, format, true);
	do_read_defect_data_12(fd, false, true, format, false);
}

static void do_read_defect_data(int fd)
{
	uint8_t format;

	for (format = 0; format < 8; format++)
		do_read_defect_data_10_all(fd, format);

	for (format = 0; format < 8; format++)
		do_read_defect_data_12_all(fd, format);
}

static void do_ata_identify(int fd)
{
	uint8_t cdb[32];
	int cdb_len;
	uint8_t buf[512];

	cdb_len = cdb_ata_identify(cdb);

	simple_command(fd, cdb, cdb_len, buf, sizeof(buf));
}

static void do_ata_identify_16(int fd)
{
	uint8_t cdb[32];
	int cdb_len;
	uint8_t buf[512];

	cdb_len = cdb_ata_identify_16(cdb);

	simple_command(fd, cdb, cdb_len, buf, sizeof(buf));
}

static void do_ata_smart_return_status(int fd)
{
	uint8_t cdb[32];
	int cdb_len;
	uint8_t buf[512];

	cdb_len = cdb_ata_smart_return_status(cdb);

	simple_command(fd, cdb, cdb_len, buf, sizeof(buf));
}

static void do_ata_smart_read_data(int fd)
{
	uint8_t cdb[32];
	int cdb_len;
	uint8_t buf[512];

	cdb_len = cdb_ata_smart_read_data(cdb);

	simple_command(fd, cdb, cdb_len, buf, sizeof(buf));
}

static void do_ata_smart_read_threshold(int fd)
{
	uint8_t cdb[32];
	int cdb_len;
	uint8_t buf[512];

	cdb_len = cdb_ata_smart_read_threshold(cdb);

	simple_command(fd, cdb, cdb_len, buf, sizeof(buf));
}

static int do_ata_read_log_ext_page(int fd, uint8_t *buf, unsigned buf_sz, unsigned log_addr, unsigned page)
{
	uint8_t cdb[32];
	int cdb_len;

	cdb_len = cdb_ata_read_log_ext(cdb, 1, page, log_addr);
	return simple_command(fd, cdb, cdb_len, buf, buf_sz);
}

static void do_ata_read_log_ext(int fd)
{
	uint8_t  __attribute__((aligned(512))) buf[512];
	uint8_t  __attribute__((aligned(512))) buf_data[512];
	unsigned log_addr;

	do_ata_read_log_ext_page(fd, buf, sizeof(buf), 0, 0);

	// Validate the page is valid
	if (buf[0] != 1 || buf[1] != 0)
		return;

	for (log_addr = 1; log_addr < sizeof(buf)/2; log_addr++) {
		unsigned num_pages = ata_get_word(buf, log_addr);
		if (num_pages) {
			unsigned page;
			for (page = 0; page < num_pages; page++) {
				printf("READ LOG EXT log addr %02X page %u/%u\n", log_addr, page, num_pages);
				int ret = do_ata_read_log_ext_page(fd, buf_data, sizeof(buf_data), log_addr, page);
				if (ret < 0)
					break;
			}
		}
	}
}

static int do_ata_smart_read_log_addr(int fd, unsigned char *buf, unsigned buf_sz, uint8_t log_addr, uint8_t block_count)
{
	uint8_t cdb[32];
	int cdb_len;

	cdb_len = cdb_ata_smart_read_log(cdb, log_addr, block_count);
	return simple_command(fd, cdb, cdb_len, buf, buf_sz);
}

static void do_ata_smart_read_log(int fd)
{
	unsigned log_addr;
	uint8_t  __attribute__((aligned(512))) buf[512];
	uint8_t  __attribute__((aligned(512))) buf_data[512*256];

	int ret = do_ata_smart_read_log_addr(fd, buf, sizeof(buf), 0, 1);
	if (ret < (int)sizeof(buf))
		return;

	// Validate the page is valid
	if (buf[0] != 1 || buf[1] != 0)
		return;

	for (log_addr = 1; log_addr < 255; log_addr++) {
		unsigned num_pages = buf[log_addr*2];
		if (num_pages > 0) {
			printf("SMART READ LOG log addr %02X pages %u\n", log_addr, num_pages);
			fflush(stdout);
			do_ata_smart_read_log_addr(fd, buf_data, 512*num_pages, log_addr, num_pages);
		}
	}
}

static void do_ata_check_power_mode(int fd)
{
	uint8_t cdb[32];
	int cdb_len;

	printf("Check power mode\n");
	cdb_len = cdb_ata_check_power_mode(cdb);
	simple_command(fd, cdb, cdb_len, NULL, 0);
}

void do_command(int fd)
{
	debug = 0;
	is_ata = false;

	printf("msg,cdb,sense,data\n");
	do_read_capacity(fd);
	do_simple_inquiry(fd);
	do_extended_inquiry(fd);
	do_log_sense(fd);
	do_mode_sense(fd);
	do_receive_diagnostic(fd);
	do_read_defect_data(fd);

	if (is_ata) {
		do_ata_identify(fd);
		do_ata_identify_16(fd);
		do_ata_check_power_mode(fd);
		do_ata_smart_return_status(fd);
		do_ata_smart_read_data(fd);
		do_ata_smart_read_threshold(fd);
		do_ata_read_log_ext(fd);
		do_ata_smart_read_log(fd);
	}
}
