#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "parse_log_sense.h"
#include "parse_mode_sense.h"
#include "parse_extended_inquiry.h"
#include "parse_read_defect_data.h"
#include "parse_receive_diagnostics.h"
#include "scsicmd.h"
#include "sense_dump.h"

#ifndef __AFL_LOOP
#define __AFL_LOOP(count) 1
#endif

static char *csvtok_last;

static void csvtok_reset(void)
{
	csvtok_last = NULL;
}

static char *csvtok(char *start)
{
	if (start)
		csvtok_last = start;

	if (!csvtok_last)
		return NULL;

	char *ret = csvtok_last;
	for (; *csvtok_last && *csvtok_last != ','; csvtok_last++)
		;
	if (*csvtok_last) {
		*csvtok_last = 0;
		csvtok_last++;
	}
	else
		csvtok_last = NULL;
	return ret;
}

static unsigned char char2val(unsigned char ch)
{
	if (ch >= '0' && ch <= '9')
		return ch - '0';
	else if (ch >= 'a' && ch <= 'f')
		return ch - 'a' + 10;
	else if (ch >= 'A' && ch <= 'F')
		return ch - 'A' + 10;
	else
		return 0;
}

static void print_hex(uint8_t *buf, unsigned buf_len)
{
	unsigned i;
	for (i = 0; i < buf_len; i++) {
		printf("%02x ", buf[i]);
	}
	printf("\n");
}

static unsigned char *parse_hex(char *str, int *len_out)
{
	char buf[64*1024];
	const unsigned buf_size = sizeof(buf);
	unsigned char ch;
	unsigned len = 0;
	bool top_char = true;

	*len_out = -1;

	for (; *str && len < buf_size; str++) {
		if (isspace(*str)) {
			if (!top_char) {
				printf("Leftover character\n");
				return NULL;
			}
		} else if (isxdigit(*str)) {
			if (top_char) {
				ch = char2val(*str);
				top_char = false;
			} else {
				buf[len++] = (ch<<4) | char2val(*str);
				top_char = true;
			}
		} else {
			printf("Unknown character '%c'\n", *str);
			return NULL;
		}
	}

	// For valgrind and AFL, copy the data to a malloc buffer to easily detect out of bounds accesses
	unsigned char *out = malloc(len);
	memcpy(out, buf, len);
	*len_out = len;
	return out;
}

static inline const char *yes_no(bool val)
{
	return val ? "yes" : "no";
}

static void unparsed_data(uint8_t *buf, unsigned buf_len, uint8_t *start, unsigned total_len)
{
	const unsigned len = safe_len(start, total_len, buf, buf_len);
	printf("Unparsed data: ");
	print_hex(buf, len);
}

static void parse_log_sense_param_informational_exceptions(uint16_t param_code, uint8_t *param, uint8_t param_len)
{
	switch (param_code) {
		case 0:
			printf("Information Exceptions ASC: %02X\n", param[0]);
			printf("Information Exceptions ASCQ: %02X\n", param[1]);
			printf("Temperature: %u\n", param[2]);
			if (param_len > 3)
				unparsed_data(param+3, param_len-3, param, param_len);
			break;
		default:
			unparsed_data(param, param_len, param, param_len);
	}
}

static void parse_log_sense_param_ascii(uint8_t *param, unsigned param_len)
{
	uint8_t *ascii = log_sense_param_data(param);
	unsigned ascii_len = log_sense_param_len(param);
	ascii_len = safe_len(param, param_len, ascii, ascii_len);

	printf("ASCII (%u): '", ascii_len);
	for (; ascii_len > 0; ascii_len--, ascii++)
		putchar(*ascii);
	printf("'\n");
}

static void parse_log_sense_param_counter(uint8_t *param, unsigned param_len)
{
	uint8_t *data = log_sense_param_data(param);
	unsigned data_len = log_sense_param_len(param);

	switch (data_len) {
		case 2:
			printf("Counter 16bit: %u\n", get_uint16(data, 0));
			break;
		case 4:
			printf("Counter 32bit: %u\n", get_uint32(data, 0));
			break;
		case 8:
			printf("Counter 64bit: %lu\n", get_uint64(data, 0));
			break;
		default:
			printf("Counter %d bytes\n", data_len);
			unparsed_data(data, data_len, param, param_len);
			break;
	}
}

static void parse_log_sense_param(uint8_t page, uint8_t subpage, uint16_t param_code, uint8_t *param, uint8_t param_len)
{
	(void)subpage;

	switch (page) {
		case 0x2F: parse_log_sense_param_informational_exceptions(param_code, log_sense_param_data(param), log_sense_param_len(param)); break;
		/* TODO: parse more LOG SENSE pages */
		default:
				   switch (log_sense_param_fmt(param)) {
					   case LOG_PARAM_FMT_COUNTER_STOP:
					   case LOG_PARAM_FMT_COUNTER_ROLLOVER:
						   parse_log_sense_param_counter(param, param_len);
						   break;
					   case LOG_PARAM_FMT_ASCII:
						   parse_log_sense_param_ascii(param, param_len);
						   break;
					   default:
						   unparsed_data(param, param_len, param, param_len);
						   break;
				   }
				   break;
	}
}

static int parse_log_sense(unsigned char *data, unsigned data_len)
{
	printf("Log Sense\n");
	if (data_len < LOG_SENSE_MIN_LEN) {
		printf("Insufficient data in log sense to begin parsing\n");
		return 1;
	}
	printf("Log Sense Page Code: 0x%02x\n", log_sense_page_code(data));
	printf("Log Sense Subpage format: %s\n", yes_no(log_sense_subpage_format(data)));
	if (log_sense_subpage_format(data))
		printf("Log Sense Subpage: 0x%02x\n", log_sense_subpage_code(data));
	printf("Log Sense Data Saved: %s\n", yes_no(log_sense_data_saved(data)));
	printf("Log Sense Data Length: %u\n", log_sense_data_len(data));

	if (log_sense_page_code(data) == 0) {
		if (!log_sense_subpage_format(data)) {
			printf("Supported Log Pages:\n");
			uint8_t supported_page;
			for_all_log_sense_pg_0_supported_pages(data, data_len, supported_page) {
				printf("\t%02X\n", supported_page & 0x3F);
			}
		} else if (log_sense_subpage_code(data) == 0xFF) {
			printf("Supported Log Subpages:\n");
			uint8_t supported_page, supported_subpage;
			for_all_log_sense_pg_0_supported_subpages(data, data_len, supported_page, supported_subpage) {
				printf("\t%02X %02X\n", supported_page & 0x3F, supported_subpage);
			}
		} else {
			printf("Unknown supported log page combination");
			unparsed_data(log_sense_data(data), log_sense_data_len(data), data, data_len);
		}
	} else {
		uint8_t *param;
		for_all_log_sense_params(data, data_len, param) {
			putchar('\n');
			printf("Log Sense Param Code: 0x%04x\n", log_sense_param_code(param));
			printf("Log Sense Param Len: %u\n", log_sense_param_len(param));
			printf("Log Sense Param format: %u\n", log_sense_param_fmt(param));
			parse_log_sense_param(log_sense_page_code(data), log_sense_subpage_code(data), log_sense_param_code(param), param, log_sense_param_len(param) + 4);
		}
	}

	return 0;
}

static int parse_read_cap_10(unsigned char *data, unsigned data_len)
{
	uint32_t max_lba;
	uint32_t block_size;
	bool parsed = parse_read_capacity_10(data, data_len, &max_lba, &block_size);

	printf("Read Capacity 10\n");
	if (!parsed) {
		unparsed_data(data, data_len, data, data_len);
		return 1;
	}

	printf("Max LBA: %u\n", max_lba);
	printf("Block Size: %u\n", block_size);

	if (data_len > 8)
		unparsed_data(data+8, data_len-8, data, data_len);
	return 0;
}

static int parse_read_cap_16(unsigned char *data, unsigned data_len)
{
	uint64_t max_lba;
	uint32_t block_size;
	bool prot_enable, thin_provisioning_enabled, thin_provisioning_zero;
	unsigned p_type, p_i_exponent, logical_blocks_per_physical_block_exponent, lowest_aligned_lba;

	printf("Read Capacity 16\n");

	bool parsed = parse_read_capacity_16(data, data_len, &max_lba, &block_size, &prot_enable,
		&p_type, &p_i_exponent, &logical_blocks_per_physical_block_exponent,
		&thin_provisioning_enabled, &thin_provisioning_zero, &lowest_aligned_lba);

	if (!parsed) {
		unparsed_data(data, data_len, data, data_len);
		return 1;
	}

	printf("Max LBA: %lu\n", max_lba);
	printf("Block Size: %u\n", block_size);
	printf("Protection enabled: %s\n", yes_no(prot_enable));
	printf("Thin Provisioning enabled: %s\n", yes_no(thin_provisioning_enabled));
	printf("Thin Provisioning zero: %s\n", yes_no(thin_provisioning_zero));
	printf("P Type: %u\n", p_type);
	printf("Pi Exponent: %u\n", p_i_exponent);
	printf("Logical blocks per physical block exponent: %u\n", logical_blocks_per_physical_block_exponent);
	printf("Lowest aligned LBA: %u\n", lowest_aligned_lba);

	return 0;
}

static int parse_extended_inquiry_data(uint8_t *data, unsigned data_len)
{
	printf("Extended Inquiry\n");

	if (data_len < EVPD_MIN_LEN) {
		printf("Not enough data for EVPD header\n");
		unparsed_data(data, data_len, data, data_len);
		return 1;
	}

	printf("Peripheral Qualifier: %d\n", evpd_peripheral_qualifier(data));
	printf("Peripheral Device Type: %d\n", evpd_peripheral_device_type(data));
	printf("EVPD page code: 0x%02X\n", evpd_page_code(data));
	printf("EVPD data len: %u\n", evpd_page_len(data));

	if (!evpd_is_valid(data, data_len))
		return 0;

	uint8_t *page_data = evpd_page_data(data);

	if (evpd_is_ascii_page(evpd_page_code(data))) {
		printf("ASCII len: %u\n", evpd_ascii_len(page_data));
		printf("ASCII string: '%*s'\n", evpd_ascii_len(page_data), evpd_ascii_data(page_data));
		if (evpd_ascii_post_data_len(page_data, data_len) > 0)
			unparsed_data(evpd_ascii_post_data(page_data), evpd_ascii_post_data_len(page_data, data_len), data, data_len);
	} else {
		/* TODO: parse more of the extended inquiry pages */
		unparsed_data(page_data, evpd_page_len(data), data, data_len);
	}
	return 0;
}

static int parse_simple_inquiry_data(uint8_t *data, unsigned data_len)
{
	int device_type;
	scsi_vendor_t vendor;
	scsi_model_t model;
	scsi_fw_revision_t rev;
	scsi_serial_t serial;
	bool parsed = parse_inquiry(data, data_len, &device_type, vendor, model, rev, serial);

	printf("Simple Inquiry\n");

	if (!parsed) {
		unparsed_data(data, data_len, data, data_len);
		return 1;
	}

	printf("Device Type: %d\n", device_type);
	printf("Vendor: %s\n", vendor);
	printf("Model: %s\n", model);
	printf("FW Revision: %s\n", rev);
	printf("Serial: %s\n", serial);
	return 0;
}

static int parse_inquiry_data(uint8_t *cdb, unsigned cdb_len, uint8_t *data, unsigned data_len)
{
	if (cdb_len < 6)
		return 1;

	if (cdb[1] & 1)
		return parse_extended_inquiry_data(data, data_len);
	else
		return parse_simple_inquiry_data(data, data_len);
}

static void parse_mode_sense_block_descriptor(uint8_t *data, unsigned data_len)
{
	if (data_len != BLOCK_DESCRIPTOR_LENGTH) {
		printf("Unknown block descriptor\n");
		unparsed_data(data, data_len, data, data_len);
		return;
	}

	printf("Density code: %u\n", block_descriptor_density_code(data));
	printf("Num blocks: %u\n", block_descriptor_num_blocks(data));
	printf("Block length: %u\n", block_descriptor_block_length(data));
}

static void parse_mode_sense_data_page(uint8_t *data, unsigned data_len)
{
	bool subpage_format = mode_sense_data_subpage_format(data);
	printf("\nPage code: 0x%02x\n", mode_sense_data_page_code(data));

	if (subpage_format)
		printf("Subpage code: 0x%02x\n", mode_sense_data_subpage_code(data));
	printf("Page Saveable: %s\n", yes_no(mode_sense_data_parameter_saveable(data)));

	printf("Page len: %u\n", mode_sense_data_param_len(data));
	/* TODO: Parse the mode sense data */
	unparsed_data(mode_sense_data_param(data), mode_sense_data_param_len(data), data, data_len);
}

static int parse_mode_sense_10(uint8_t *data, unsigned data_len)
{
	printf("Mode Sense 10\n");

	if (data_len < MODE_SENSE_10_MIN_LEN) {
		printf("Not enough data for MODE SENSE header\n");
		unparsed_data(data, data_len, data, data_len);
		return 1;
	}

	printf("Mode Sense 10 data length: %u\n", mode_sense_10_data_len(data));
	printf("Mode Sense 10 medium type: %u\n", mode_sense_10_medium_type(data));
	printf("Mode Sense 10 Device specific param: %u\n", mode_sense_10_device_specific_param(data));
	printf("Mode Sense 10 Long LBA: %s\n", yes_no(mode_sense_10_long_lba(data)));
	printf("Mode Sense 10 Block descriptor length: %u\n", mode_sense_10_block_descriptor_length(data));

	if (data_len < mode_sense_10_expected_length(data)) {
		printf("Not enough data to parse full data\n");
		unparsed_data(data + MODE_SENSE_10_MIN_LEN, data_len - MODE_SENSE_10_MIN_LEN, data, data_len);
		return 1;
	}

	if (mode_sense_10_block_descriptor_length(data) > 0) {
		const unsigned safe_desc_len = safe_len(data, data_len, mode_sense_10_block_descriptor_data(data), mode_sense_10_block_descriptor_length(data)); 
		parse_mode_sense_block_descriptor(mode_sense_10_block_descriptor_data(data), safe_desc_len);
	}

	unsigned remaining_len;
	uint8_t *mode_page;
	for_all_mode_sense_10_pages(data, data_len, mode_page, remaining_len) {
		printf("\nRemaining len: %u\n", remaining_len);
		parse_mode_sense_data_page(mode_page, remaining_len);
	}
	return 0;
}

static int parse_mode_sense_6(uint8_t *data, unsigned data_len)
{
	printf("Mode Sense 6\n");

	if (data_len < MODE_SENSE_6_MIN_LEN) {
		printf("Not enough data for MODE SENSE 6 header\n");
		unparsed_data(data, data_len, data, data_len);
		return 1;
	}

	printf("Mode Sense 6 data length: %u\n", mode_sense_6_data_len(data));
	printf("Mode Sense 6 medium type: %u\n", mode_sense_6_medium_type(data));
	printf("Mode Sense 6 Device specific param: %u\n", mode_sense_6_device_specific_param(data));
	printf("Mode Sense 6 Block descriptor length: %u\n", mode_sense_6_block_descriptor_length(data));

	if (data_len < mode_sense_6_expected_length(data)) {
		printf("Not enough data to parse full data\n");
		unparsed_data(data + MODE_SENSE_6_MIN_LEN, data_len - MODE_SENSE_6_MIN_LEN, data, data_len);
		return 1;
	}

	if (!mode_sense_6_is_valid_header(data, data_len)) {
		printf("Bad data in mode sense header\n");
		return 1;
	}

	if (mode_sense_6_block_descriptor_length(data) > 0) {
		unsigned safe_desc_len = safe_len(data, data_len, mode_sense_6_block_descriptor_data(data), mode_sense_6_block_descriptor_length(data)); 
		parse_mode_sense_block_descriptor(mode_sense_6_block_descriptor_data(data), safe_desc_len);
	}

	unsigned remaining_len;
	uint8_t *mode_page;
	for_all_mode_sense_6_pages(data, data_len, mode_page, remaining_len) {
		printf("Remaining len: %u\n", remaining_len);
		parse_mode_sense_data_page(mode_page, remaining_len);
	}
	return 0;
}

static void read_defect_data_format(address_desc_format_e fmt, uint8_t *data, unsigned len)
{
	const unsigned fmt_len = read_defect_data_fmt_len(fmt);
	if (fmt_len == 0) {
		printf("Unknown format to decode\n");
		unparsed_data(data, len, data, len);
		return;
	}
	for (; len > fmt_len; data += fmt_len, len -= fmt_len) {
		switch (fmt) {
			case ADDRESS_FORMAT_SHORT:
				printf("\t%u\n", get_uint32(data, 0));
				break;
			case ADDRESS_FORMAT_LONG:
				printf("\t%lu\n", get_uint64(data, 0));
				break;
			case ADDRESS_FORMAT_INDEX_OFFSET:
				printf("\tC=%u H=%u B=%u\n",
						format_address_byte_from_index_cylinder(data),
						format_address_byte_from_index_head(data),
						format_address_byte_from_index_bytes(data));
				break;
			case ADDRESS_FORMAT_PHYSICAL:
				printf("\tC=%u H=%u S=%u\n",
						format_address_physical_cylinder(data),
						format_address_physical_head(data),
						format_address_physical_sector(data));
				break;
			case ADDRESS_FORMAT_VENDOR:
				printf("\t%08x\n", get_uint32(data, 0));
				break;
			default:
				break;
		}
	}
}

static int parse_read_defect_data_10(uint8_t *data, unsigned data_len)
{
	printf("Read Defect Data 10\n");

	if (!read_defect_data_10_hdr_is_valid(data, data_len)) {
		printf("Header is not valid\n");
		unparsed_data(data, data_len, data, data_len);
		return 1;
}

	printf("Plist: %s\n", yes_no(read_defect_data_10_is_plist_valid(data)));
	printf("Glist: %s\n", yes_no(read_defect_data_10_is_glist_valid(data)));
	printf("Format: %s\n", read_defect_data_format_to_str(read_defect_data_10_list_format(data)));
	printf("Len: %u\n", read_defect_data_10_len(data));

	if (!read_defect_data_10_is_valid(data, data_len))
		return 0;

	if (data_len > 0) {
		const unsigned len = safe_len(data, data_len, read_defect_data_10_data(data), read_defect_data_10_len(data));
		read_defect_data_format(read_defect_data_10_list_format(data), read_defect_data_10_data(data), len);
	}
	return 0;
}

static int parse_read_defect_data_12(uint8_t *data, unsigned data_len)
{
	printf("Read Defect Data 12\n");

	if (!read_defect_data_12_hdr_is_valid(data, data_len)) {
		printf("Header is not valid\n");
		unparsed_data(data, data_len, data, data_len);
		return 1;
	}

	printf("Plist: %s\n", yes_no(read_defect_data_12_is_plist_valid(data)));
	printf("Glist: %s\n", yes_no(read_defect_data_12_is_glist_valid(data)));
	printf("Format: %s\n", read_defect_data_format_to_str(read_defect_data_12_list_format(data)));
	printf("Len: %u\n", read_defect_data_12_len(data));

	if (!read_defect_data_12_is_valid(data, data_len))
		return 0;

	if (data_len > 0) {
		const unsigned len = safe_len(data, data_len, read_defect_data_10_data(data), read_defect_data_10_len(data));
		read_defect_data_format(read_defect_data_12_list_format(data), read_defect_data_12_data(data), len);
	}
	return 0;
}

static void parse_receive_diagnostic_results_pg_0(uint8_t *data, unsigned data_len)
{
	printf("Supported Receive Diagnostic Results pages:\n");
	for (; data_len > 0; data_len--, data++)
		printf("\t0x%02x\n", data[0]);
}

static unsigned parse_enclosure_descriptor(uint8_t *data, unsigned data_len)
{
	char name[16];

	if (!ses_config_enclosure_descriptor_is_valid(data, data_len))
		return data_len;

	printf("\nProcess identifier: %u\n", ses_config_enclosure_descriptor_process_identifier(data));
	printf("Num processes: %u\n", ses_config_enclosure_descriptor_num_processes(data));
	printf("Subenclosure identifier: %u\n", ses_config_enclosure_descriptor_subenclosure_identifier(data));
	printf("Num Type Descriptors: %u\n", ses_config_enclosure_descriptor_num_type_descriptors(data));
	printf("Enclosure descriptor len: %u\n", ses_config_enclosure_descriptor_len(data));
	printf("Logical identified: %016lx\n", ses_config_enclosure_descriptor_logical_identifier(data));

	ses_config_enclosure_descriptor_vendor_identifier(data, name, sizeof(name));
	printf("Vendor identifier: %s\n", name);

	ses_config_enclosure_descriptor_product_identifier(data, name, sizeof(name));
	printf("Product identifier: %s\n", name);

	ses_config_enclosure_descriptor_revision_level(data, name, sizeof(name));
	printf("Revision level: %s\n", name);

	printf("Vendor info len: %u\n", ses_config_enclosure_descriptor_vendor_len(data));
	if (ses_config_enclosure_descriptor_vendor_len(data) > 0)
		unparsed_data(ses_config_enclosure_descriptor_vendor_info(data), ses_config_enclosure_descriptor_vendor_len(data), data, data_len);

	return ses_config_enclosure_descriptor_len(data) + 4;
}

static void parse_receive_diagnostic_results_pg_1(uint8_t *data, unsigned data_len)
{
	unsigned parsed_len = 8;
	unsigned num_enclosures;

	if (!ses_config_is_valid(data, data_len))
		return;

	printf("SES config page:\n");
	num_enclosures = ses_config_num_sub_enclosures(data);
	printf("Num subenclosures: %u\n", num_enclosures);
	printf("Generation code: %u\n", ses_config_generation(data));

	for (; num_enclosures > 0 && parsed_len < data_len; num_enclosures--)
		parsed_len += parse_enclosure_descriptor(ses_config_sub_enclosure(data), data_len-parsed_len);

	/* TODO: There can be additional enclosures and type descriptors and strings */
	unparsed_data(data + parsed_len, data_len - parsed_len, data, data_len);
}

static int parse_receive_diagnostic_results(uint8_t *data, unsigned data_len)
{
	printf("Receive Diagnostic Results\n");

	if (!recv_diag_is_valid(data, data_len)) {
		printf("Data is not valid\n");
		return 1;
	}

	printf("Page code: 0x%02X\n", recv_diag_get_page_code(data));
	printf("Page code specific: 0x%02x\n", recv_diag_get_page_code_specific(data));
	printf("Len: %u\n", recv_diag_get_len(data));

	switch (recv_diag_get_page_code(data)) {
		case 0:
			parse_receive_diagnostic_results_pg_0(recv_diag_data(data), safe_len(data, data_len, recv_diag_data(data), recv_diag_get_len(data)));
			break;
		case 1:
			parse_receive_diagnostic_results_pg_1(data, data_len);
			break;
		default:
			unparsed_data(recv_diag_data(data), recv_diag_get_len(data), data, data_len); /* TODO: parse SES pages */
			break;
	}
	return 0;
}

static void process_data(char *cdb_src, char *sense_src, char *data_src)
{
	unsigned char *cdb = NULL;
	unsigned char *sense = NULL;
	unsigned char *data = NULL;
	int cdb_len, sense_len, data_len;

	printf("CDB: %s\n", cdb_src);
	printf("Sense: %s\n", sense_src);
	printf("Data: %s\n", data_src);

	if (cdb_src == NULL || sense_src == NULL || data_src == NULL) {
		printf("Input csv is invalid\n");
		return;
	}

	cdb = parse_hex(cdb_src, &cdb_len);
	sense = parse_hex(sense_src, &sense_len);
	data = parse_hex(data_src, &data_len);

	printf("CDB Len: %d\n", cdb_len);
	printf("Sense Len: %d\n", sense_len);
	printf("Data Len: %d\n", data_len);

	if (cdb_len < 0) {
		printf("Failed to parse CDB\n");
		goto Exit;
	}
	if (sense_len < 0) {
		printf("Failed to parse SENSE\n");
		goto Exit;
	}
	if (data_len < 0) {
		printf("Failed to parse DATA\n");
		goto Exit;
	}

	if (sense_len > 0) {
		printf("Sense data indicates an error, not parsing data\n");
		sense_dump(sense, sense_len);
		goto Exit;
	}

	switch (cdb[0]) {
		case 0x4D: parse_log_sense(data, data_len); break;
		case 0x25: parse_read_cap_10(data, data_len); break;
		case 0x9E: parse_read_cap_16(data, data_len); break;
		case 0x12: parse_inquiry_data(cdb, cdb_len, data, data_len); break;
		case 0x5A: parse_mode_sense_10(data, data_len); break;
		case 0x1A: parse_mode_sense_6(data, data_len); break;
		case 0x1C: parse_receive_diagnostic_results(data, data_len); break;
		case 0x37: parse_read_defect_data_10(data, data_len); break;
		case 0xB7: parse_read_defect_data_12(data, data_len); break;
		default:
				   printf("Unsupported CDB opcode %02X\n", cdb[0]);
				   unparsed_data(data, data_len, data, data_len);
				   break;
	}

Exit:
	free(cdb);
	free(sense);
	free(data);
}

static ssize_t read_newline(char *buf, size_t buf_sz)
{
	ssize_t data_read = 0;

	while (data_read < (ssize_t)buf_sz) {
		int ch = getchar();
		if (ch == EOF)
			break;

		if (ch == '\n' || ch == '\r')
			return data_read;

		buf[data_read++] = ch;
	}

	return data_read;
}

int main(int argc, char **argv)
{
	char *cdb_src, *sense_src, *data_src;

	if (argc != 4 && argc != 1) {
		printf("Usage: %s \"cdb\" \"sense\" \"data\"\n", argv[0]);
		return 1;
	}

	if (argc == 1) {
		while (__AFL_LOOP(30000)) {
			char buf[64*1024];
			memset(buf, 0, sizeof(buf));
			int ret = read_newline(buf, sizeof(buf));
			if (ret <= 0) {
				printf("Insufficient input\n");
				return 1;
			}
			buf[ret] = 0;

			csvtok_reset();
			csvtok(buf);
			cdb_src = csvtok(NULL);
			sense_src = csvtok(NULL);
			data_src = csvtok(NULL);
			process_data(cdb_src, sense_src, data_src);
			printf("=================================================================================\n");
		}
	} else {
		cdb_src = argv[1];
		sense_src = argv[2];
		data_src = argv[3];
		process_data(cdb_src, sense_src, data_src);
	}

	return 0;
}
