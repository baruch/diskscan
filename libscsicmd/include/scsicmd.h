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

#ifndef LIBSCSICMD_H
#define LIBSCSICMD_H

#include <stdbool.h>
#include <stdint.h>

#define SCSI_VENDOR_LEN 8
#define SCSI_MODEL_LEN 16
#define SCSI_FW_REVISION_LEN 4
#define SCSI_SERIAL_LEN 8

typedef char scsi_vendor_t[SCSI_VENDOR_LEN+1];
typedef char scsi_model_t[SCSI_MODEL_LEN+1];
typedef char scsi_fw_revision_t[SCSI_FW_REVISION_LEN+1];
typedef char scsi_serial_t[SCSI_SERIAL_LEN+1];

enum sense_key_e {
        SENSE_KEY_NO_SENSE = 0x0,
        SENSE_KEY_RECOVERED_ERROR = 0x1,
        SENSE_KEY_NOT_READY = 0x2,
        SENSE_KEY_MEDIUM_ERROR = 0x3,
        SENSE_KEY_HARDWARE_ERROR = 0x4,
        SENSE_KEY_ILLEGAL_REQUEST = 0x5,
        SENSE_KEY_UNIT_ATTENTION = 0x6,
        SENSE_KEY_DATA_PROTECT = 0x7,
        SENSE_KEY_BLANK_CHECK = 0x8,
        SENSE_KEY_VENDOR_SPECIFIC = 0x9,
        SENSE_KEY_COPY_ABORTED = 0xA,
        SENSE_KEY_ABORTED_COMMAND = 0xB,
        SENSE_KEY_RESERVED_C = 0xC,
        SENSE_KEY_VOLUME_OVERFLOW = 0xD,
        SENSE_KEY_MISCOMPARE = 0xE,
        SENSE_KEY_COMPLETED = 0xF,
};

int cdb_tur(unsigned char *cdb);

typedef enum scsi_device_type_e {
	SCSI_DEV_TYPE_BLOCK = 0,
	SCSI_DEV_TYPE_SEQ = 1,
	SCSI_DEV_TYPE_PRINTER = 2,
	SCSI_DEV_TYPE_PROCESSOR = 3,
	SCSI_DEV_TYPE_WRITE_ONCE = 4,
	SCSI_DEV_TYPE_CD = 5,
	SCSI_DEV_TYPE_SCANNER = 6,
	SCSI_DEV_TYPE_OPTICAL = 7,
	SCSI_DEV_TYPE_MEDIA_CHANGER = 8,
	SCSI_DEV_TYPE_COMMUNICATION = 9,
	SCSI_DEV_TYPE_OBSOLETE_A = 0xA,
	SCSI_DEV_TYPE_OBSOLETE_B = 0xB,
	SCSI_DEV_TYPE_RAID = 0xC,
	SCSI_DEV_TYPE_SES = 0xD,
	SCSI_DEV_TYPE_RBC = 0xE,
	SCSI_DEV_TYPE_OCRW = 0xF,
	SCSI_DEV_TYPE_BCC = 0x10,
	SCSI_DEV_TYPE_OSD = 0x11,
	SCSI_DEV_TYPE_ADC2 = 0x12,
	SCSI_DEV_TYPE_SECURITY_MGR = 0x13,
	SCSI_DEV_TYPE_RESERVED_14 = 0x14,
	SCSI_DEV_TYPE_RESERVED_15 = 0x15,
	SCSI_DEV_TYPE_RESERVED_16 = 0x16,
	SCSI_DEV_TYPE_RESERVED_17 = 0x17,
	SCSI_DEV_TYPE_RESERVED_18 = 0x18,
	SCSI_DEV_TYPE_RESERVED_19 = 0x19,
	SCSI_DEV_TYPE_RESERVED_1A = 0x1A,
	SCSI_DEV_TYPE_RESERVED_1B = 0x1B,
	SCSI_DEV_TYPE_RESERVED_1C = 0x1C,
	SCSI_DEV_TYPE_RESERVED_1D = 0x1D,
	SCSI_DEV_TYPE_WELL_KNOWN = 0x1E,
	SCSI_DEV_TYPE_UNKNOWN = 0x1F,
} scsi_device_type_e;

typedef struct ata_status_t {
	uint8_t extend;
	uint8_t error;
	uint8_t device;
	uint8_t status;
	uint16_t sector_count;
	uint64_t lba;
} ata_status_t;

/* sense parser */
typedef struct sense_info_t {
        bool is_fixed; // Else descriptor based
        bool is_current; // Else for previous request
        uint8_t sense_key;
        uint8_t asc;
        uint8_t ascq;
        bool information_valid;
        uint64_t information;
        bool cmd_specific_valid;
        uint64_t cmd_specific;
        bool sense_key_specific_valid;
        union {
                struct {
                        bool command_error;
                        bool bit_pointer_valid;
                        uint8_t bit_pointer;
                        uint16_t field_pointer;
                } illegal_request;
                struct {
                        uint16_t actual_retry_count;
                } hardware_medium_recovered_error;
                struct {
                        double progress;
                } not_ready;
                struct {
                        bool segment_descriptor;
                        bool bit_pointer_valid;
                        uint8_t bit_pointer;
                        uint16_t field_pointer;
                } copy_aborted;
                struct {
                        bool overflow;
                } unit_attention;
        } sense_key_specific;
        bool fru_code_valid;
        uint8_t fru_code;
        bool ata_status_valid;
        ata_status_t ata_status;
        bool incorrect_len_indicator;
} sense_info_t;
bool scsi_parse_sense(unsigned char *sense, int sense_len, sense_info_t *info);

/* inquiry */
int cdb_inquiry(unsigned char *cdb, bool evpd, char page_code, uint16_t alloc_len);
static inline int cdb_inquiry_simple(unsigned char *cdb, uint16_t alloc_len) { return cdb_inquiry(cdb, 0, 0, alloc_len); }
bool parse_inquiry(char *buf, unsigned buf_len, int *device_type, scsi_vendor_t vendor,
                   scsi_model_t model, scsi_fw_revision_t rev, scsi_serial_t serial);

#endif
