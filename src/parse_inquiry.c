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

#include <stdio.h>
#include <string.h>

bool parse_inquiry(unsigned char *buf, unsigned buf_len, int *device_type, scsi_vendor_t vendor, scsi_model_t model,
                   scsi_fw_revision_t revision, scsi_serial_t serial)
{
        *device_type = -1;
        vendor[0] = 0;
        model[0] = 0;
        revision[0] = 0;
        serial[0] = 0;

        if (buf_len < 32)
                return false;

        unsigned char fmt = buf[3] & 0xf; 

        int valid_len = buf[4] + 4;

        *device_type = buf[0] & 0x1f;

        if (valid_len >= 8 + SCSI_VENDOR_LEN) {
                strncpy(vendor, (char*)buf+8, SCSI_VENDOR_LEN);
                vendor[SCSI_VENDOR_LEN] = 0;
        }

        if (valid_len >= 16 + SCSI_MODEL_LEN) {
                strncpy(model, (char*)buf+16, SCSI_MODEL_LEN);
                model[SCSI_MODEL_LEN] = 0;
        }

        if (valid_len >= 32 + SCSI_FW_REVISION_LEN) {
                strncpy(revision, (char*)buf+32, SCSI_FW_REVISION_LEN);
                revision[SCSI_FW_REVISION_LEN] = 0;
        }

        if (valid_len >= 44 && fmt == 2) {
                strncpy(serial, (char*)buf+36, SCSI_SERIAL_LEN);
                serial[SCSI_SERIAL_LEN] = 0;
        }

        return true;
}

#define STRINGIFY(name) # name

const char *scsi_device_type_name(scsi_device_type_e dev_type)
{
#define X(name) case SCSI_DEV_TYPE_##name: return STRINGIFY(name);
	switch (dev_type) {
	SCSI_DEVICE_TYPE_LIST
#undef X
	}

	return "Unknown device type";
}
