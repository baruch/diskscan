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
#include <assert.h>
#include <memory.h>


bool ata_inquiry_checksum_verify(const unsigned char *buf, int buf_len)
{
	if (buf_len != 512)
		return false;

	if (buf[511] != 0xA5) {
		// Checksum isn't claimed to be valid, nothing to check here
		return true;
	}

	return ata_checksum_verify(buf);
}

bool ata_status_from_scsi_sense(unsigned char *sense, int sense_len, ata_status_t *status)
{
        sense_info_t sense_info;

	assert(status);

        bool parsed = scsi_parse_sense(sense, sense_len, &sense_info);

        if (parsed && sense_info.ata_status_valid) {
                *status = sense_info.ata_status;
                return true;
        }
        if (sense_info.is_fixed) {
                // Fixed format parsing for ATA passthrough can't be known automatically
                memset(status, 0, sizeof(*status));
                status->error = sense_info.information & 0xFF;
                status->status = (sense_info.information >> 8) & 0xFF;
                status->device = (sense_info.information >> 16) & 0xFF;

                status->sector_count = (sense_info.information >> 24) & 0xFF;

                status->extend = sense_info.cmd_specific & 0x80;
                uint32_t lba_high = (sense_info.cmd_specific >> 8) & 0xFF;
                uint32_t lba_mid = (sense_info.cmd_specific >> 16) & 0xFF;
                uint32_t lba_low = (sense_info.cmd_specific >> 24) & 0xFF;
                status->lba = (lba_high << 16) | (lba_mid << 8) | lba_low;

                // TODO: sector_count_upper_non_zero: sense_info.cmd_specific & 0x40
                // TODO: lba upper non zero: sense_info.cmd_specific & 0x20
                // TODO: log index: sense_info.cmd_specific & 0x07
        }

        return false;
}

/* ATA SMART READ DATA */

uint16_t ata_get_ata_smart_read_data_version(const unsigned char *buf)
{
	return ata_get_word(buf, 0);
}

int ata_parse_ata_smart_read_data(const unsigned char *buf, ata_smart_attr_t *attrs, int max_attrs)
{
	if (!ata_check_ata_smart_read_data_checksum(buf))
		return -1;

	if (ata_get_ata_smart_read_data_version(buf) != 0x0010)
		return -1;

	int i, j;

	for (i = 0, j = 0; i < MAX_SMART_ATTRS && i < max_attrs; i++) {
		const unsigned char *raw_attr = buf + 2 + 12*i;
		ata_smart_attr_t *attr = &attrs[j];

		attr->id = raw_attr[0];
		if (attr->id == 0) // Skip an invalid attribute
			continue;
		attr->status = raw_attr[1] | raw_attr[2]<<8;
		attr->value = raw_attr[3];
		attr->min = raw_attr[4];
		attr->raw = (raw_attr[5]) |
			        (raw_attr[6] << 8) |
					(raw_attr[7] << 16) |
					(raw_attr[8] << 24) |
					((uint64_t)raw_attr[9] << 32) |
					((uint64_t)raw_attr[10] << 40);

		j++;
	}

	return j;
}

int ata_parse_ata_smart_read_thresh(const unsigned char *buf, ata_smart_thresh_t *attrs, int max_attrs)
{
	if (!ata_check_ata_smart_read_data_checksum(buf))
		return -1;

	if (ata_get_ata_smart_read_data_version(buf) != 0x0010)
		return -1;

	int i, j;

	for (i = 0, j = 0; i < MAX_SMART_ATTRS && i < max_attrs; i++) {
		const unsigned char *raw_attr = buf + 2 + 12*i;
		ata_smart_thresh_t *attr = &attrs[j];

		attr->id = raw_attr[0];
		if (attr->id == 0) // Skip an invalid attribute
			continue;
		attr->threshold = raw_attr[1];

		j++;
	}

	return j;
}
