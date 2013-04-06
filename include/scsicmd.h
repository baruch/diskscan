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

/* inquiry */
int cdb_inquiry(unsigned char *cdb, bool evpd, char page_code, uint16_t alloc_len);
static inline int cdb_inquiry_simple(unsigned char *cdb, uint16_t alloc_len) { return cdb_inquiry(cdb, 0, 0, alloc_len); }
bool parse_inquiry(char *buf, unsigned buf_len, int *device_type, scsi_vendor_t vendor,
                   scsi_model_t model, scsi_fw_revision_t rev, scsi_serial_t serial);

#endif
