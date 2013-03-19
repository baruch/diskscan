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

/* inquiry */
int cdb_inquiry(unsigned char *cdb, bool evpd, char page_code, uint16_t alloc_len);
static inline int cdb_inquiry_simple(unsigned char *cdb, uint16_t alloc_len) { return cdb_inquiry(cdb, 0, 0, alloc_len); }
bool parse_inquiry(char *buf, unsigned buf_len, int *device_type, scsi_vendor_t vendor,
                   scsi_model_t model, scsi_fw_revision_t rev, scsi_serial_t serial);

#endif
